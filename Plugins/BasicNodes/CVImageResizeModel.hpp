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
 * @file CVImageResizeModel.hpp
 * @brief Node model for resizing images using OpenCV
 * 
 * This file defines a node that resizes images using either a scale factor
 * or explicit dimensions. It wraps OpenCV's cv::resize() function and provides
 * a convenient node interface for image scaling operations in processing pipelines.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"

#include <opencv2/opencv.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVImageResizeModel
 * @brief Node model for image resizing and scaling operations
 * 
 * This model provides image resizing functionality using OpenCV's cv::resize().
 * It supports two resize modes:
 * - Scale mode: Resize by a multiplicative factor (e.g., 0.5 for half size, 2.0 for double)
 * - Dimension mode: Resize to explicit width and height
 * 
 * The node processes images synchronously, performing the resize operation
 * whenever new input data arrives. Various interpolation methods can be
 * configured via the property browser (NEAREST, LINEAR, CUBIC, LANCZOS4).
 * 
 * Typical use cases:
 * - Downsampling for faster processing
 * - Upsampling for visualization
 * - Normalizing image sizes for batch processing
 * - Preparing images for neural network input
 * 
 * Input:
 * - Port 0: CVImageData - The image to resize
 * 
 * Output:
 * - Port 0: CVImageData - The resized image
 * 
 * @note Maintains the input image's channel count and depth
 * @note For large images or real-time processing, consider using lower interpolation quality
 * @see cv::resize for the underlying OpenCV operation
 */
class CVImageResizeModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new image resize node
     * 
     * Initializes default parameters:
     * - Scale factor: 1.0 (no change)
     * - Target size: 640x480
     * - Interpolation: LINEAR (default)
     */
    CVImageResizeModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVImageResizeModel() override {}

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the resize parameters for project persistence.
     * 
     * @return QJsonObject containing scale, width, height, and interpolation mode
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved resize parameters.
     * 
     * @param p QJsonObject containing the saved node configuration
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 1 input port (source image)
     * - 1 output port (resized image)
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
     * @brief Provides the resized image output
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the resized CVImageData
     * @note Returns nullptr if no input has been processed yet
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input image data
     * 
     * When image data arrives, this method:
     * 1. Validates the input data
     * 2. Calculates target dimensions (from scale or explicit size)
     * 3. Calls cv::resize() with configured interpolation
     * 4. Stores the result for output
     * 5. Notifies connected nodes of new data
     * 
     * @param nodeData The input CVImageData
     * @param portIndex The input port index (must be 0)
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

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
     * - "scale": Scale factor (double, > 0)
     * - "width": Target width in pixels (int, > 0)
     * - "height": Target height in pixels (int, > 0)
     * - "interpolation": Interpolation method (INTER_NEAREST, INTER_LINEAR, etc.)
     * 
     * When properties change, the node automatically reprocesses the current
     * input to reflect the new settings.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /** @brief Category name for node organization */
    static const QString _category;

    /** @brief Display name for the node type */
    static const QString _model_name;

private:
    /**
     * @brief Internal helper to perform the resize operation
     * 
     * Executes cv::resize() on the input image and stores the result.
     * This method handles:
     * - Size calculation from scale factor or explicit dimensions
     * - Interpolation method selection
     * - Error handling for invalid parameters
     * 
     * @param in The input CVImageData to resize
     * @param out The output CVImageData to populate with resized image
     * @note Uses cv::resize() internally with configured interpolation
     */
    void processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out );
    
    /** @brief Cached input image data */
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    
    /** @brief Cached resized image data for output */
    std::shared_ptr<CVImageData> mpCVImageOutData { nullptr };

    /** 
     * @brief Scale factor for proportional resizing
     * @note When scale != 1.0, dimensions are calculated as: new_size = original_size * scale
     */
    double mdScale { 1.f };
    
    /** 
     * @brief Explicit target dimensions
     * @note Used when not scaling by factor; overrides scale-based sizing
     */
    cv::Size mSize { cv::Size(640, 480) };
    QPixmap _minPixmap;
};


