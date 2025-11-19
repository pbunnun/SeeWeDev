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
 * @file CVColorSpaceModel.hpp
 * @brief Node model for converting images between color spaces
 * 
 * This file defines a node that converts images from one color space to another
 * using OpenCV's cvtColor function. It supports various color space conversions
 * including RGB, BGR, HSV, LAB, grayscale, and many others.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVColorSpaceParameters
 * @brief Parameter structure for color space conversion
 * 
 * Encapsulates the configuration for color space conversion operations.
 * The input and output color space codes correspond to OpenCV's ColorConversionCodes
 * enum (e.g., COLOR_BGR2RGB, COLOR_RGB2HSV, COLOR_BGR2GRAY).
 * 
 * @note The kernel size field appears to be unused in current implementation
 */
typedef struct CVColorSpaceParameters{
    /** @brief Kernel size (currently unused) */
    cv::Size mCVSizeKernel;
    
    /** 
     * @brief Input color space code
     * @note Corresponds to the first part of OpenCV conversion codes
     * @see cv::ColorConversionCodes
     */
    int miCVColorSpaceInput;
    
    /** 
     * @brief Output color space code
     * @note Corresponds to the second part of OpenCV conversion codes
     * @see cv::ColorConversionCodes
     */
    int miCVColorSpaceOutput;
    
    /**
     * @brief Default constructor with typical BGR to HSV conversion
     */
    CVColorSpaceParameters()
    {
        miCVColorSpaceInput = 1;  // BGR
        miCVColorSpaceOutput = 2; // HSV
    }
} CVColorSpaceParameters;

/**
 * @class CVColorSpaceModel
 * @brief Node model for color space conversion operations
 * 
 * This model provides color space conversion functionality using OpenCV's cvtColor().
 * It enables transforming images between various color representations:
 * - RGB ↔ BGR (channel reordering)
 * - RGB/BGR → Grayscale (luminance extraction)
 * - RGB/BGR ↔ HSV (hue-saturation-value)
 * - RGB/BGR ↔ LAB (perceptually uniform color space)
 * - RGB/BGR ↔ YCrCb (luma-chroma)
 * - And many other OpenCV-supported conversions
 * 
 * Common use cases:
 * - Preprocessing for algorithms that expect specific color spaces (e.g., HSV for color-based segmentation)
 * - Channel reordering (BGR to RGB) for display or export
 * - Converting to grayscale for edge detection or feature extraction
 * - LAB color space for perceptually meaningful color operations
 * 
 * Input:
 * - Port 0: CVImageData - The source image to convert
 * 
 * Output:
 * - Port 0: CVImageData - The converted image
 * 
 * Design Note: The node validates that the selected conversion is compatible with
 * the input image format (e.g., can't convert single-channel to RGB).
 * 
 * @note Not all color space conversions are valid for all input formats
 * @see cv::cvtColor for the underlying OpenCV conversion function
 * @see cv::ColorConversionCodes for available conversion types
 */
class CVColorSpaceModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new color space conversion node
     * 
     * Initializes with default BGR to HSV conversion parameters.
     */
    CVColorSpaceModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVColorSpaceModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the current color space conversion settings.
     * 
     * @return QJsonObject containing input/output color space codes
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved color space conversion parameters.
     * 
     * @param p QJsonObject containing the saved node configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 1 input port (source image)
     * - 1 output port (converted image)
     * 
     * @param portType The type of port (In or Out)
     * @return Number of ports of the specified type
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port
     * 
     * All ports use CVImageData type.
     * 
     * @param portType The type of port (In or Out)
     * @param portIndex The index of the port
     * @return NodeDataType describing CVImageData
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Provides the converted image output
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the converted CVImageData
     * @note Returns nullptr if no input has been processed or conversion failed
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input image data
     * 
     * When image data arrives, this method:
     * 1. Validates the input data
     * 2. Determines the OpenCV conversion code from input/output color spaces
     * 3. Calls cv::cvtColor() to perform the conversion
     * 4. Stores the result for output
     * 5. Notifies connected nodes of new data
     * 
     * @param nodeData The input CVImageData
     * @param portIndex The input port index (must be 0)
     * @note Invalid conversions will result in an error state
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief No embedded widget for this node
     * 
     * @return nullptr - This node has no UI widget
     * @note All parameters are configured via the property browser
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets model properties from the property browser
     * 
     * Handles property changes for:
     * - "input_color_space": The source color space (enumeration)
     * - "output_color_space": The target color space (enumeration)
     * 
     * The property browser typically presents these as dropdown menus with
     * human-readable names (e.g., "BGR", "RGB", "HSV", "Grayscale").
     * 
     * When properties change, the node automatically reprocesses the current
     * input to reflect the new conversion settings.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property (color space code)
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Provides a thumbnail preview pixmap
     * 
     * @return QPixmap for node list/palette preview
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    /** @brief Category name for node organization */
    static const QString _category;

    /** @brief Display name for the node type */
    static const QString _model_name;

private:
    /**
     * @brief Internal helper to perform the color space conversion
     * 
     * Constructs the appropriate OpenCV ColorConversionCode from the parameter
     * structure and executes cv::cvtColor(). Handles error cases where the
     * conversion is incompatible with the input image format.
     * 
     * @param in The input CVImageData to convert
     * @param out The output CVImageData to populate with converted image
     * @param params The conversion parameters (input/output color spaces)
     * @note Uses OpenCV's cv::cvtColor() internally
     * @see cv::cvtColor
     * @see cv::ColorConversionCodes
     */
    void
    processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out,
                 const CVColorSpaceParameters & params );

    /** @brief Current conversion parameters */
    CVColorSpaceParameters mParams;
    
    /** @brief Cached output image data */
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    
    /** @brief Cached input image data */
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    
    /** @brief Preview pixmap for node palette */
    QPixmap _minPixmap;
};


