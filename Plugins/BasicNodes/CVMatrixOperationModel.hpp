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
 * @file CVMatrixOperationModel.hpp
 * @brief Element-wise matrix/image arithmetic and comparison operations.
 *
 * This node performs pixel-wise operations between two images (cv::Mat), including
 * arithmetic (+, -, *, /), comparison (<, >, ≤, ≥), and min/max functions. It wraps
 * OpenCV's overloaded operators and functions for matrix operations.
 *
 * **Supported Operations:**
 * - **Arithmetic**: Addition, Subtraction, Multiplication, Division
 * - **Comparison**: Greater Than, Less Than, Greater/Equal, Less/Equal
 * - **Min/Max**: Element-wise minimum, Element-wise maximum
 *
 * **Key Features:**
 * - 10 different matrix operations
 * - Element-wise (pixel-by-pixel) processing
 * - Supports grayscale and color images
 * - Type conversion and saturation handled automatically
 * - Single operator selection via properties
 *
 * **Typical Applications:**
 * - Image blending and compositing
 * - Background subtraction
 * - Gain/offset adjustments
 * - Pixel-wise comparisons and masking
 * - Dynamic range operations
 *
 * @see cv::add, cv::subtract, cv::multiply, cv::divide
 * @see cv::max, cv::min, cv::compare
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
 * @struct MatOps
 * @brief Matrix operation identifiers.
 *
 * Enum defining all supported element-wise matrix operations.
 */
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
///

struct MatOps
{
    enum Operators
    {
        PLUS = 0,                   ///< Addition: out = img1 + img2 (with saturation)
        MINUS = 1,                  ///< Subtraction: out = img1 - img2 (with saturation)
        GREATER_THAN = 2,           ///< Comparison: out = (img1 > img2) ? 255 : 0
        GREATER_THAN_OR_EQUAL = 3,  ///< Comparison: out = (img1 ≥ img2) ? 255 : 0
        LESSER_THAN = 4,            ///< Comparison: out = (img1 < img2) ? 255 : 0
        LESSER_THAN_OR_EQUAL = 5,   ///< Comparison: out = (img1 ≤ img2) ? 255 : 0
        MULTIPLY = 6,               ///< Multiplication: out = img1 * img2 (with saturation)
        DIVIDE = 7,                 ///< Division: out = img1 / img2 (avoid divide-by-zero)
        MAXIMUM = 8,                ///< Element-wise max: out = max(img1, img2)
        MINIMUM = 9                 ///< Element-wise min: out = min(img1, img2)
    };
};

/**
 * @struct MatrixOperationParameters
 * @brief Configuration for matrix operation selection.
 */
typedef struct MatrixOperationParameters{
    int miOperator; ///< Selected operator (MatOps::Operators enum value)
    MatrixOperationParameters()
        : miOperator(MatOps::PLUS)
    {
    }
} MatrixOperationParameters;

/**
 * @class CVMatrixOperationModel
 * @brief Element-wise matrix operations for image arithmetic and comparison.
 *
 * CVMatrixOperationModel provides pixel-wise operations between two images, supporting
 * arithmetic, comparison, and min/max functions. All operations are element-wise
 * (applied independently to each pixel) with automatic saturation/clipping.
 *
 * **Port Configuration:**
 * - **Inputs:**
 *   - Port 0: CVImageData - First operand (image 1)
 *   - Port 1: CVImageData - Second operand (image 2)
 * - **Output:**
 *   - Port 0: CVImageData - Result image
 *
 * **Operation Details:**
 *
 * **1. PLUS (Addition):**
 * ```cpp
 * out = cv::add(img1, img2);  // Saturates at 255 for 8-bit images
 * // Example: 200 + 100 = 255 (not 300, clamped to max)
 * ```
 * Use cases: Image brightening, compositing, superimposition
 *
 * **2. MINUS (Subtraction):**
 * ```cpp
 * out = cv::subtract(img1, img2);  // Saturates at 0 for unsigned types
 * // Example: 50 - 100 = 0 (not -50, clamped to min)
 * ```
 * Use cases: Background subtraction, change detection, difference images
 *
 * **3. MULTIPLY (Multiplication):**
 * ```cpp
 * out = cv::multiply(img1, img2);  // Result scaled/saturated
 * // Example: img1 * (img2/255) for gain control
 * ```
 * Use cases: Gain adjustment, masking, alpha blending, contrast stretching
 *
 * **4. DIVIDE (Division):**
 * ```cpp
 * out = cv::divide(img1, img2);  // Handles divide-by-zero gracefully
 * // Division by zero typically results in 0 or saturation
 * ```
 * Use cases: Normalization, ratio images, flat-field correction
 *
 * **5. GREATER_THAN (>):**
 * ```cpp
 * out = (img1 > img2) ? 255 : 0;  // Binary mask output
 * // Creates white pixels where img1 > img2, black otherwise
 * ```
 * Use cases: Thresholding, pixel-wise comparison masks
 *
 * **6. GREATER_THAN_OR_EQUAL (≥):**
 * ```cpp
 * out = (img1 >= img2) ? 255 : 0;  // Binary mask output
 * ```
 * Use cases: Inclusive threshold masks
 *
 * **7. LESSER_THAN (<):**
 * ```cpp
 * out = (img1 < img2) ? 255 : 0;  // Binary mask output
 * ```
 * Use cases: Inverted threshold masks
 *
 * **8. LESSER_THAN_OR_EQUAL (≤):**
 * ```cpp
 * out = (img1 <= img2) ? 255 : 0;  // Binary mask output
 * ```
 * Use cases: Inclusive lower bound masks
 *
 * **9. MAXIMUM (Element-wise Max):**
 * ```cpp
 * out = cv::max(img1, img2);  // Per-pixel maximum
 * // out[i,j] = max(img1[i,j], img2[i,j])
 * ```
 * Use cases: Image compositing (choose brighter pixel), noise reduction
 *
 * **10. MINIMUM (Element-wise Min):**
 * ```cpp
 * out = cv::min(img1, img2);  // Per-pixel minimum
 * // out[i,j] = min(img1[i,j], img2[i,j])
 * ```
 * Use cases: Image compositing (choose darker pixel), intersection masks
 *
 * **Common Use Cases:**
 *
 * 1. **Background Subtraction:**
 *    ```
 *    CurrentFrame → MINUS ← BackgroundModel
 *                     ↓
 *              Foreground Mask
 *    ```
 *
 * 2. **Image Blending (50/50):**
 *    ```
 *    Image1 * 0.5 → PLUS ← Image2 * 0.5
 *                     ↓
 *                Blended Image
 *    ```
 *
 * 3. **Adaptive Thresholding:**
 *    ```
 *    Image → GREATER_THAN ← AdaptiveThreshold
 *                ↓
 *           Binary Mask
 *    ```
 *
 * 4. **Flat-Field Correction:**
 *    ```
 *    RawImage → DIVIDE ← FlatFieldReference
 *                  ↓
 *            Corrected Image
 *    ```
 *
 * 5. **Max Projection (Time Series):**
 *    ```
 *    Frame[t] → MAXIMUM ← Frame[t+1] → MAXIMUM ← Frame[t+2] → ...
 *                            ↓
 *                    Maximum Intensity Projection
 *    ```
 *
 * 6. **Region Highlighting:**
 *    ```
 *    BaseImage → MULTIPLY ← Mask (0.5 outside ROI, 1.0 inside)
 *                    ↓
 *              Highlighted ROI
 *    ```
 *
 * **Image Compatibility:**
 * - **Size Matching**: Both input images must have same dimensions (width × height)
 * - **Type Handling**:
 *   - Same types: Direct operation
 *   - Different types: Automatic conversion to common type
 *   - Color + Grayscale: Grayscale broadcast to all channels
 * - **Multi-Channel**: Operations applied independently to each channel (R, G, B)
 *
 * **Saturation Behavior (8-bit images):**
 * - Addition: Clips at 255 (no overflow to 0)
 * - Subtraction: Clips at 0 (no underflow to 255)
 * - Multiplication: Scales and clips at 255
 * - Division: Result scaled to [0, 255] range
 *
 * **Performance Characteristics:**
 * - Complexity: O(W × H × C) where W×H = image size, C = channels
 * - Typical Processing Time:
 *   - 640×480 grayscale: ~1ms
 *   - 1920×1080 color: ~5-10ms
 * - Memory: Allocates new cv::Mat for result
 * - Optimization: Uses OpenCV's vectorized implementations (SIMD)
 *
 * **Design Rationale:**
 * - Single operator per node keeps graph structure clear
 * - Saturation prevents unexpected wraparound artifacts
 * - Element-wise operations maintain spatial structure
 * - Supports both arithmetic and logical operations in one node
 *
 * **Troubleshooting:**
 * - **Size Mismatch Error**: Ensure both images have identical dimensions
 * - **Unexpected Results**: Check image types (8-bit vs float, signed vs unsigned)
 * - **Division by Zero**: OpenCV handles gracefully (typically outputs 0)
 * - **Overflow Artifacts**: Use appropriate image types (float for unbounded arithmetic)
 *
 * @note For weighted operations (e.g., 0.7*img1 + 0.3*img2), use cv::addWeighted or separate multiply nodes
 * @note Comparison operators output 8-bit binary images (255=true, 0=false)
 * @see cv::add, cv::subtract, cv::multiply, cv::divide, cv::max, cv::min, cv::compare
 */
class CVMatrixOperationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVMatrixOperationModel();

    virtual
    ~CVMatrixOperationModel() override {}

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
    MatrixOperationParameters mParams;                      ///< Selected operation
    std::shared_ptr<CVImageData> mpCVImageData { nullptr }; ///< Output result image
    std::shared_ptr<CVImageData> mapCVImageInData[2] {{ nullptr }}; ///< Input images [0]=img1, [1]=img2
    QPixmap _minPixmap;                                     ///< Node icon

    /**
     * @brief Executes selected matrix operation on two input images.
     *
     * **Processing Logic:**
     * ```cpp
     * cv::Mat img1 = in[0]->data();
     * cv::Mat img2 = in[1]->data();
     * cv::Mat result;
     *
     * switch (params.miOperator) {
     *     case MatOps::PLUS:
     *         cv::add(img1, img2, result);
     *         break;
     *     case MatOps::MINUS:
     *         cv::subtract(img1, img2, result);
     *         break;
     *     case MatOps::MULTIPLY:
     *         cv::multiply(img1, img2, result);
     *         break;
     *     case MatOps::DIVIDE:
     *         cv::divide(img1, img2, result);
     *         break;
     *     case MatOps::GREATER_THAN:
     *         cv::compare(img1, img2, result, cv::CMP_GT);
     *         break;
     *     case MatOps::GREATER_THAN_OR_EQUAL:
     *         cv::compare(img1, img2, result, cv::CMP_GE);
     *         break;
     *     case MatOps::LESSER_THAN:
     *         cv::compare(img1, img2, result, cv::CMP_LT);
     *         break;
     *     case MatOps::LESSER_THAN_OR_EQUAL:
     *         cv::compare(img1, img2, result, cv::CMP_LE);
     *         break;
     *     case MatOps::MAXIMUM:
     *         cv::max(img1, img2, result);
     *         break;
     *     case MatOps::MINIMUM:
     *         cv::min(img1, img2, result);
     *         break;
     * }
     *
     * out = std::make_shared<CVImageData>(result);
     * ```
     *
     * @param in Input images [0]=first operand, [1]=second operand
     * @param out Output result image
     * @param params Operation configuration (miOperator selects function)
     *
     * @note Images must have same dimensions (W×H)
     * @note Type conversion handled automatically by OpenCV
     */
    void processData( const std::shared_ptr<CVImageData> (&in)[2], std::shared_ptr< CVImageData > & out,
                      const MatrixOperationParameters & params );
};

