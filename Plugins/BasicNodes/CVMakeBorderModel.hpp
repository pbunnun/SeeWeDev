//Copyright © 2025, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

/**
 * @file CVMakeBorderModel.hpp
 * @brief Provides image border padding with various border types and colors.
 *
 * This file implements a node that adds borders around images using OpenCV's cv::copyCVMakeBorder
 * function. Border padding is essential for operations that require pixels outside the image
 * boundaries, such as convolution, filtering, and geometric transformations.
 *
 * Mathematical Context:
 * Many image processing operations use neighborhood information (e.g., 3×3, 5×5 kernels).
 * At image boundaries, some neighbors are outside the image. Border extrapolation fills
 * these missing values using various strategies.
 *
 * Border Types Supported:
 * 1. BORDER_CONSTANT: Fill with constant color (user-specified)
 *    - Use case: Add colored frame, prepare for rotation without black corners
 *    - Example: Black border (0,0,0) or white border (255,255,255)
 *
 * 2. BORDER_REPLICATE: Repeat edge pixels (aaa|abcd|ddd)
 *    - Use case: Natural extension for edge detection, filtering
 *    - Avoids discontinuities at boundaries
 *
 * 3. BORDER_REFLECT: Mirror reflection without repeating edge (cba|abcd|dcb)
 *    - Use case: Seamless tiling, avoiding edge artifacts
 *    - Preserves continuity in derivatives
 *
 * 4. BORDER_WRAP: Wrap around (tiling) (bcd|abcd|abc)
 *    - Use case: Periodic patterns, seamless textures
 *    - Treats image as tiled plane
 *
 * 5. BORDER_REFLECT_101: Mirror with edge repetition (dcb|abcd|cba)
 *    - Use case: Default for many OpenCV filters
 *    - Most common choice for general filtering
 *
 * Border Width Configuration:
 * - Independent control for each side (top, bottom, left, right)
 * - Asymmetric borders supported
 * - Useful for alignment, aspect ratio adjustment, or preparing for operations
 *
 * Common Use Cases:
 * - Pre-padding for convolution filters (avoid shrinking output)
 * - Preparing images for rotation (prevent corner clipping)
 * - Adding decorative frames
 * - Alignment padding (center small image in larger canvas)
 * - Extending images for seamless operations
 *
 * @see CVMakeBorderModel, cv::copyCVMakeBorder, cv::BorderTypes
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"


using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVMakeBorderParameters
 * @brief Configuration parameters for border padding operation.
 *
 * This structure defines all parameters controlling how borders are added to images.
 *
 * Border Dimensions (in pixels):
 * - miBorderTop: Padding added above image
 * - miBorderBottom: Padding added below image
 * - miBorderLeft: Padding added to left of image
 * - miBorderRight: Padding added to right of image
 *
 * Output size = (width + left + right) × (height + top + bottom)
 *
 * Border Type (miBorderType):
 * Determines how border pixels are filled. See cv::BorderTypes:
 * - BORDER_CONSTANT (0): Use constant color (mucBorderColor)
 * - BORDER_REPLICATE (1): Replicate edge pixels (aaa|abcd|ddd)
 * - BORDER_REFLECT (2): Mirror reflection (cba|abcd|dcb)
 * - BORDER_WRAP (3): Wrap around (bcd|abcd|abc)
 * - BORDER_REFLECT_101 (4): Mirror with edge (default, dcb|abcd|cba)
 *
 * Border Color (mucBorderColor):
 * - RGB color array for BORDER_CONSTANT type
 * - Range: [0-255] per channel
 * - Ignored for other border types
 * - Example: {255, 0, 0} = blue border (BGR order)
 *
 * Default Values:
 * - 1 pixel border on all sides
 * - BORDER_CONSTANT type
 * - Black color (0, 0, 0)
 */
typedef struct CVMakeBorderParameters{
    int miBorderTop;           ///< Top border width in pixels
    int miBorderBottom;        ///< Bottom border width in pixels
    int miBorderLeft;          ///< Left border width in pixels
    int miBorderRight;         ///< Right border width in pixels
    int miBorderType;          ///< Border extrapolation method (cv::BorderTypes)
    int mucBorderColor[3];     ///< RGB color for BORDER_CONSTANT (ignored otherwise)
    bool mbEnableGradient;     ///< Reserved for future gradient border feature
    CVMakeBorderParameters()
        : miBorderTop(1),
          miBorderBottom(1),
          miBorderLeft(1),
          miBorderRight(1),
          miBorderType(cv::BORDER_CONSTANT),
          mucBorderColor{0}
    {
    }
} CVMakeBorderParameters;

/**
 * @struct CVMakeBorderProperties
 * @brief Output properties tracking input and output image sizes.
 *
 * This structure stores the dimensions before and after border addition,
 * useful for validation and display purposes.
 *
 * Relationship:
 * mCVSizeOutput.width = mCVSizeInput.width + miBorderLeft + miBorderRight
 * mCVSizeOutput.height = mCVSizeInput.height + miBorderTop + miBorderBottom
 */
typedef struct CVMakeBorderProperties
{
    cv::Size mCVSizeInput;     ///< Input image dimensions (before border)
    cv::Size mCVSizeOutput;    ///< Output image dimensions (after border)
    CVMakeBorderProperties()
        : mCVSizeInput(cv::Size(0,0)),
          mCVSizeOutput(cv::Size(0,0))
    {
    }
} CVMakeBorderProperties;

/**
 * @class CVMakeBorderModel
 * @brief Node for adding borders around images with configurable type and size.
 *
 * This model provides border padding functionality using cv::copyCVMakeBorder, essential
 * for operations requiring pixel access beyond image boundaries and for adding decorative
 * or functional frames to images.
 *
 * Core Functionality:
 * ```cpp
 * cv::copyCVMakeBorder(
 *     input,                    // Source image
 *     output,                   // Destination with border
 *     top, bottom, left, right, // Border widths
 *     borderType,               // Extrapolation method
 *     borderColor               // Color for BORDER_CONSTANT
 * );
 * ```
 *
 * Common Use Cases:
 *
 * 1. Pre-padding for Convolution:
 *    ```
 *    Image → CVMakeBorder(REFLECT_101, 2px all sides) → Filter2D(5×5)
 *    ```
 *    Prevents output shrinking, preserves image size through filtering
 *
 * 2. Rotation Preparation:
 *    ```
 *    Image → CVMakeBorder(CONSTANT, black, 50px) → Rotate(45°)
 *    ```
 *    Prevents corner clipping during rotation
 *
 * 3. Decorative Frame:
 *    ```
 *    Photo → CVMakeBorder(CONSTANT, white, 10px) → Display
 *    ```
 *    Add white frame around photo
 *
 * 4. Alignment Padding:
 *    ```
 *    SmallImage → CVMakeBorder(asymmetric) → StandardSize
 *    ```
 *    Center small image in larger canvas
 *
 * 5. Edge Artifact Prevention:
 *    ```
 *    Image → CVMakeBorder(REPLICATE, 1px) → EdgeDetection
 *    ```
 *    Avoid boundary artifacts in gradient-based operations
 *
 * Border Type Details:
 *
 * BORDER_CONSTANT (Solid Color):
 * ```
 * Input:  | a b c d |
 * Output: x x x | a b c d | x x x  (x = border color)
 * ```
 * Best for: Decorative frames, rotation preparation, explicit boundaries
 *
 * BORDER_REPLICATE (Edge Replication):
 * ```
 * Input:  | a b c d |
 * Output: a a a | a b c d | d d d
 * ```
 * Best for: Filtering, avoiding discontinuities, natural extension
 *
 * BORDER_REFLECT (Mirror Reflection):
 * ```
 * Input:  | a b c d |
 * Output: d c b | a b c d | c b a
 * ```
 * Best for: Seamless operations, periodic patterns
 *
 * BORDER_WRAP (Wrap Around):
 * ```
 * Input:  | a b c d |
 * Output: b c d | a b c d | a b c
 * ```
 * Best for: Tiling, periodic signals
 *
 * BORDER_REFLECT_101 (Default Reflection):
 * ```
 * Input:  | a b c d |
 * Output: c b a | a b c d | d c b  (edge pixel not repeated)
 * ```
 * Best for: General filtering (OpenCV default), derivative operations
 *
 * Performance: O(W×H) where output is (W+L+R) × (H+T+B), very fast (memory copy)
 *
 * @see cv::copyCVMakeBorder, cv::BorderTypes, CVMakeBorderParameters
 */
class CVMakeBorderModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVMakeBorderModel();

    virtual
    ~CVMakeBorderModel() override {}

    QJsonObject
    save() const override;

    void
    load(const QJsonObject &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:
    CVMakeBorderParameters mParams;                      ///< Current border configuration
    CVMakeBorderProperties mProps;                       ///< Input/output size tracking
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };    ///< Output image with border
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };  ///< Input image
    QPixmap _minPixmap;                                ///< Node icon

    /**
     * @brief Core border addition processing function.
     *
     * Applies cv::copyCVMakeBorder with specified parameters:
     * ```cpp
     * cv::Scalar borderColor(params.mucBorderColor[0],
     *                        params.mucBorderColor[1],
     *                        params.mucBorderColor[2]);
     * cv::copyCVMakeBorder(
     *     in->data(),
     *     output,
     *     params.miBorderTop,
     *     params.miBorderBottom,
     *     params.miBorderLeft,
     *     params.miBorderRight,
     *     params.miBorderType,
     *     borderColor
     * );
     * ```
     *
     * @param in Input image
     * @param out Output image with added border
     * @param params Border configuration (sizes, type, color)
     * @param props Properties to update (input/output sizes)
     */
    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & out,
                      const CVMakeBorderParameters & params, CVMakeBorderProperties &props);
};

