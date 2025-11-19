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
 * @file CVRGBsetValueModel.hpp
 * @brief Set specific channel values in RGB/BGR color images.
 *
 * This node allows manual setting of individual color channel values (Red, Green, Blue)
 * across an entire image or per-channel basis. It's used for color adjustment, channel
 * manipulation, and creating custom color effects.
 *
 * **Key Features:**
 * - Set R, G, or B channel to specific values
 * - Embedded widget for interactive channel selection
 * - Apply channel modifications selectively
 * - Supports both RGB and BGR color spaces
 *
 * **Typical Use Cases:**
 * - Remove specific color channels (set to 0)
 * - Create monochromatic effects (set two channels to 0)
 * - Color channel testing and debugging
 * - Custom color palette creation
 * - Channel-wise color correction
 *
 * @see CVRGBsetValueEmbeddedWidget for channel selection UI
 * @see ColorSpaceModel for color space conversions
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "CVRGBsetValueEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVRGBsetValueParameters
 * @brief RGB channel value settings.
 */
typedef struct CVRGBsetValueParameters
{
     int mucRvalue;     ///< Red channel value [0-255]
     int mucGvalue;     ///< Green channel value [0-255]
     int mucBvalue;     ///< Blue channel value [0-255]
     int miChannel;     ///< Selected channel to modify (0=R, 1=G, 2=B)
     CVRGBsetValueParameters()
         : mucRvalue(0),
           mucGvalue(0),
           mucBvalue(0)
     {
     }
} CVRGBsetValueParameters;

/**
 * @struct CVRGBsetValueProperties
 * @brief Qt properties for channel value controls in the embedded widget.
 *
 * Provides Qt property interface for the CVRGBsetValueEmbeddedWidget,
 * enabling data binding between the widget UI and the node's internal
 * channel value parameters.
 */
typedef struct CVRGBsetValueProperties
{
    int miChannel;     ///< Selected channel index (0=R, 1=G, 2=B)
    int mucValue;      ///< Value to set for selected channel [0-255]
    CVRGBsetValueProperties()
        : miChannel(0),
          mucValue(0)
    {
    }
} CVRGBsetValueProperties;

/**
 * @class CVRGBsetValueModel
 * @brief Sets specific RGB/BGR channel values in color images.
 *
 * ## Overview
 * This node allows selective modification of individual RGB or BGR channels
 * by setting them to specific values [0-255]. It provides both direct
 * parameter control and an embedded widget interface for interactive
 * channel manipulation.
 *
 * ## Channel Modification
 * The node can modify any individual channel (R, G, or B) while preserving
 * the other two channels:
 * - **Red channel**: Sets all red components to specified value
 * - **Green channel**: Sets all green components to specified value
 * - **Blue channel**: Sets all blue components to specified value
 *
 * For BGR images (OpenCV default):
 * - Channel 0 → B component
 * - Channel 1 → G component
 * - Channel 2 → R component
 *
 * ## Use Cases
 * 1. **Color Adjustment**: Remove or enhance specific color channels
 * 2. **Color Calibration**: Set known channel values for testing
 * 3. **Channel Isolation**: Zero out unwanted channels
 * 4. **Custom Color Effects**: Create artistic color modifications
 * 5. **White Balance Testing**: Set channels to neutral values
 *
 * ## Processing Behavior
 * - Operates on 3-channel color images (CV_8UC3)
 * - Preserves spatial dimensions
 * - Direct pixel value assignment (no blending)
 * - Fast operation (single-pass modification)
 *
 * Example:
 * @code
 * // Set red channel to maximum, preserving G and B
 * params.miChannel = 2;  // Red channel (BGR order)
 * params.mucRvalue = 255;
 * // Result: (B, G, 255) for all pixels
 * @endcode
 *
 * ## Performance
 * - Computational cost: O(width × height) - single-pass operation
 * - Memory: Single image buffer allocation
 * - Fast channel assignment using cv::Mat::setTo() with masking
 *
 * ## Limitations
 * - Requires 3-channel color input (grayscale not supported)
 * - Global operation (affects all pixels uniformly)
 * - No spatial or conditional masking
 *
 * @see RGBtoGrayModel For RGB to grayscale conversion
 * @see CVImageOperationModel For general image arithmetic operations
 */

class CVRGBsetValueModel : public PBNodeDelegateModel
{
    Q_OBJECT

public :
    CVRGBsetValueModel();

    virtual
    ~CVRGBsetValueModel() override {}

    QJsonObject save() const override;

    void load(const QJsonObject &p) override;

    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief Returns the embedded widget for interactive channel control.
     * @return Pointer to CVRGBsetValueEmbeddedWidget for channel/value selection
     */
    QWidget *embeddedWidget() override {return mpEmbeddedWidget;}

    void setModelProperty(QString &, const QVariant &) override;

    /**
     * @brief Returns the node's icon for visual identification.
     * @return QPixmap containing the RGB set value icon
     */
    QPixmap minPixmap() const override {return _minPixmap;}

    static const QString _category;     ///< Node category: "Image Operation"

    static const QString _model_name;   ///< Node display name: "RGB Set Value"

private Q_SLOTS:

    /**
     * @brief Handles channel/value changes from the embedded widget.
     * @param button Unused parameter (legacy interface)
     * 
     * Updates the internal properties when the user modifies channel
     * selection or value in the embedded widget, then triggers reprocessing.
     */
    void
    em_button_clicked( int );

private :
    CVRGBsetValueParameters mParams;                              ///< Channel value settings
    CVRGBsetValueProperties mProps;                               ///< Widget property bindings
    std::shared_ptr<CVImageData> mpCVImageData {nullptr};       ///< Output image data
    std::shared_ptr<CVImageData> mpCVImageInData {nullptr};     ///< Input image data
    CVRGBsetValueEmbeddedWidget* mpEmbeddedWidget;                ///< Interactive channel control widget
    QPixmap _minPixmap;                                         ///< Node icon

    /**
     * @brief Processes the image by setting the specified channel value.
     * @param[out] out Output image with modified channel
     * @param props Properties containing channel selection and value
     * 
     * Processing steps:
     * 1. Clone input image to preserve other channels
     * 2. Split into separate channel planes
     * 3. Set selected channel to specified value using cv::Mat::setTo()
     * 4. Merge channels back into output image
     * 
     * Example:
     * @code
     * // Input BGR image (100, 150, 200)
     * // Set Green channel to 255 (props.miChannel=1, mParams.mucGvalue=255)
     * // Output: (100, 255, 200)
     * @endcode
     */
    void processData(std::shared_ptr<CVImageData>& out, const CVRGBsetValueProperties& props);
};

