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
 * @file CVBlendImagesModel.hpp
 * @brief Node model for alpha blending two images
 * 
 * This file defines a node that combines two input images using weighted addition.
 * It implements OpenCV's addWeighted function, allowing users to create smooth
 * transitions, overlays, and composite images.
 */

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "CVBlendImagesEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVBlendImagesParameters
 * @brief Parameter structure for image blending operations
 * 
 * Encapsulates the weighted blending formula:
 * output = alpha * image1 + beta * image2 + gamma
 * 
 * Where:
 * - alpha: Weight for first image (typically 0.0 to 1.0)
 * - beta: Weight for second image (typically 0.0 to 1.0)
 * - gamma: Scalar added to the result (brightness offset)
 * 
 * For standard alpha blending: alpha + beta = 1.0
 * Example: alpha=0.7, beta=0.3 gives 70% of image1 + 30% of image2
 */
typedef struct CVBlendImagesParameters{
    /** @brief Weight multiplier for the first input image */
    double mdAlpha;
    
    /** @brief Weight multiplier for the second input image */
    double mdBeta;
    
    /** @brief Scalar value added to the weighted sum (brightness adjustment) */
    double mdGamma;
    
    /** 
     * @brief Determines which input image's size to use for resizing
     * - true: Use size from port 0 (first image)
     * - false: Use size from port 1 (second image)
     * @note Both images are resized to match if they differ in dimensions
     */
    bool mbSizeFromPort0;
    
    /**
     * @brief Default constructor with standard 50/50 blend
     */
    CVBlendImagesParameters()
        : mdAlpha(0.5),
          mdBeta(0.5),
          mdGamma(0),
          mbSizeFromPort0(false)
    {
    }
} CVBlendImagesParameters;

/**
 * @class CVBlendImagesModel
 * @brief Node model for weighted blending of two images
 * 
 * This model provides alpha blending functionality using OpenCV's cv::addWeighted().
 * It combines two input images using the formula:
 * 
 *     output = alpha * img1 + beta * img2 + gamma
 * 
 * Key features:
 * - Automatic size matching (resizes images to common dimensions)
 * - Adjustable blend weights via sliders in embedded widget
 * - Brightness offset control (gamma parameter)
 * - Choice of which input determines output size
 * - Real-time preview with interactive controls
 * 
 * Common use cases:
 * - Cross-dissolve transitions between images
 * - Watermark overlay (low alpha on watermark)
 * - Double exposure effects
 * - Background subtraction visualization
 * - Comparison overlays (e.g., before/after with 50% blend)
 * 
 * Input:
 * - Port 0: CVImageData - First input image
 * - Port 1: CVImageData - Second input image
 * 
 * Output:
 * - Port 0: CVImageData - Blended result image
 * 
 * Design Note: If input images have different sizes, they are automatically
 * resized to match using the size specified by mbSizeFromPort0 parameter.
 * 
 * @note Both input images must have the same number of channels
 * @see cv::addWeighted for the underlying OpenCV operation
 * @see CVBlendImagesEmbeddedWidget for the interactive control interface
 */
class CVBlendImagesModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new image blending node
     * 
     * Initializes with default 50/50 blend (alpha=0.5, beta=0.5, gamma=0)
     * and creates the embedded widget for interactive control.
     */
    CVBlendImagesModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVBlendImagesModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the current blending parameters and size preference.
     * 
     * @return QJsonObject containing alpha, beta, gamma, and size source
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved blending parameters and updates the widget.
     * 
     * @param p QJsonObject containing the saved node configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 2 input ports (two images to blend)
     * - 1 output port (blended result)
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
     * @brief Provides the blended image output
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the blended CVImageData
     * @note Returns nullptr if both inputs aren't connected yet
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives input image data for blending
     * 
     * When data arrives on either input port, this method:
     * 1. Caches the input data
     * 2. Checks if both inputs are available
     * 3. If both inputs present, performs blending operation
     * 4. Notifies connected nodes of new output
     * 
     * The blending operation includes:
     * - Size matching (resize to common dimensions)
     * - Weighted addition using current alpha/beta/gamma
     * - Validation of compatible image formats
     * 
     * @param nodeData The input CVImageData
     * @param portIndex The input port index (0 or 1)
     * @note Blending only occurs when both ports have valid data
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;

    /**
     * @brief Returns the embedded widget for display in the node
     * 
     * The widget provides sliders for adjusting alpha, beta, and gamma
     * parameters in real-time.
     * 
     * @return Pointer to the CVBlendImagesEmbeddedWidget
     * @see CVBlendImagesEmbeddedWidget for widget implementation
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets model properties from the property browser
     * 
     * Handles property changes for:
     * - "alpha": Weight for first image (0.0 to 1.0)
     * - "beta": Weight for second image (0.0 to 1.0)
     * - "gamma": Brightness offset (-255 to +255)
     * - "size_from_port_0": Boolean for size source selection
     * 
     * When properties change, the node automatically reprocesses the inputs
     * if both are available.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property
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

private Q_SLOTS:

    /**
     * @brief Slot for radio button clicks in embedded widget
     * 
     * Handles user selection of which input image's size to use
     * as the reference for resizing.
     */
    void em_radioButton_clicked();

private:
    /**
     * @brief Internal helper to perform the blending operation
     * 
     * Executes the weighted blending algorithm:
     * 1. Determines target size from mbSizeFromPort0 setting
     * 2. Resizes images to match if necessary
     * 3. Calls cv::addWeighted() with current alpha/beta/gamma
     * 4. Stores result for output
     * 
     * @param in Array of two input CVImageData pointers
     * @param out The output CVImageData to populate with blended result
     * @param params The blending parameters (alpha, beta, gamma, size source)
     * @note Uses cv::addWeighted() internally
     * @see cv::addWeighted
     */
    void processData( const std::shared_ptr< CVImageData> (&in)[2], std::shared_ptr< CVImageData > & out,
                      const CVBlendImagesParameters & params);
    
    /**
     * @brief Checks if both input ports have valid data
     * 
     * Helper function to determine if blending can proceed.
     * 
     * @param ap Array of two input CVImageData pointers
     * @return true if both inputs are non-null and valid, false otherwise
     */
    bool allports_are_active(const std::shared_ptr<CVImageData> (&ap)[2] ) const;

    /** @brief Current blending parameters */
    CVBlendImagesParameters mParams;
    
    /** @brief Cached blended output image */
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    
    /** @brief Cached input images (index 0 and 1) */
    std::shared_ptr<CVImageData> mapCVImageInData[2] { {nullptr} };
    
    /** @brief Pointer to the embedded control widget */
    CVBlendImagesEmbeddedWidget* mpEmbeddedWidget;
    
    /** @brief Preview pixmap for node palette */
    QPixmap _minPixmap;
};


