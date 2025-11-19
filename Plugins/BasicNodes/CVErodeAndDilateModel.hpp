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
 * @file CVErodeAndDilateModel.hpp
 * @brief Node model for basic morphological erosion and dilation
 * 
 * This file defines a node that applies either erosion or dilation operations.
 * These are the fundamental building blocks of morphological image processing,
 * used for shape manipulation and noise removal in binary and grayscale images.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include <opencv2/imgproc.hpp>
#include "CVErodeAndDilateEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVErodeAndDilateParameters
 * @brief Parameter structure for erosion and dilation operations
 * 
 * Configures the structuring element and iteration count for morphological operations.
 */
typedef struct CVErodeAndDilateParameters{
    /** 
     * @brief Shape of the structuring element
     * @see cv::MorphShapes (MORPH_RECT, MORPH_ELLIPSE, MORPH_CROSS)
     */
    int miKernelShape;
    
    /** @brief Size of the structuring element */
    cv::Size mCVSizeKernel;
    
    /** 
     * @brief Anchor position within kernel (-1,-1 = center)
     * @note Usually set to (-1,-1) for automatic centering
     */
    cv::Point mCVPointAnchor;
    
    /** 
     * @brief Number of times to apply the operation
     * @note More iterations = stronger effect
     */
    int miIterations;
    
    /** 
     * @brief Border extrapolation method
     * @see cv::BorderTypes
     */
    int miBorderType;
    
    /**
     * @brief Default constructor with 3×3 rectangular kernel
     */
    CVErodeAndDilateParameters()
        : miKernelShape(cv::MORPH_RECT),
          mCVSizeKernel(cv::Size(3,3)),
          mCVPointAnchor(cv::Point(-1,-1)),
          miIterations(1),
          miBorderType(cv::BORDER_DEFAULT)
    {
    }
} CVErodeAndDilateParameters;

/**
 * @class CVErodeAndDilateModel
 * @brief Node model for erosion and dilation morphological operations
 * 
 * This model provides the two fundamental morphological operations using
 * OpenCV's cv::erode() and cv::dilate() functions. Users select the operation
 * via an embedded widget (radio buttons).
 * 
 * **Erosion (cv::erode):**
 * - Shrinks bright regions
 * - Removes small white noise/objects
 * - Separates touching objects
 * - Thins boundaries
 * - Formula: Output pixel = minimum of neighborhood
 * 
 * **Dilation (cv::dilate):**
 * - Expands bright regions
 * - Fills small holes/gaps
 * - Connects nearby objects
 * - Thickens boundaries
 * - Formula: Output pixel = maximum of neighborhood
 * 
 * How structuring element shape affects results:
 * - **Rectangle**: Preserves horizontal/vertical features
 * - **Ellipse**: Isotropic (same in all directions), smooth circular effect
 * - **Cross**: Emphasizes + shaped patterns, thinner than rectangle
 * 
 * Iteration effects:
 * - 1 iteration: Subtle change (1 pixel width change)
 * - Multiple iterations: Stronger effect, reaches further into image
 * - Example: 3 iterations ≈ using a 3× larger kernel (but faster)
 * 
 * Common use cases:
 * - **Noise removal**: Erode to remove white noise, dilate to restore size
 * - **Hole filling**: Dilate to fill small gaps in objects
 * - **Object separation**: Erode to separate touching objects
 * - **Edge cleanup**: Smooth jagged edges in binary masks
 * - **Size filtering**: Remove objects below certain size (erode + threshold)
 * - **Preprocessing**: Clean masks before contour detection
 * 
 * Input:
 * - Port 0: CVImageData - Source image (binary or grayscale)
 * 
 * Output:
 * - Port 0: CVImageData - Eroded or dilated image
 * 
 * Design Note: This node handles only the basic operations. For compound
 * operations (opening, closing, gradient), use MorphologicalTransformationModel.
 * 
 * @note Works on both binary and grayscale images
 * @see cv::erode for erosion operation
 * @see cv::dilate for dilation operation
 * @see MorphologicalTransformationModel for compound operations
 * @see CVErodeAndDilateEmbeddedWidget for operation selector UI
 */
class CVErodeAndDilateModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new erode/dilate node
     * 
     * Initializes with 3×3 rectangular kernel and 1 iteration.
     * Operation is selected via embedded widget.
     */
    CVErodeAndDilateModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVErodeAndDilateModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves kernel shape, size, iterations, and selected operation.
     * 
     * @return QJsonObject containing morphological parameters
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved parameters and operation selection.
     * 
     * @param p QJsonObject containing the saved configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports
     * 
     * - 1 input port (source image)
     * - 1 output port (processed image)
     * 
     * @param portType The type of port
     * @return Number of ports
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a port
     * 
     * All ports use CVImageData type.
     * 
     * @param portType The type of port
     * @param portIndex The port index
     * @return NodeDataType describing CVImageData
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Provides the processed output
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to eroded/dilated CVImageData
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input
     * 
     * Applies selected operation (erode or dilate) when data arrives.
     * 
     * @param nodeData Input image
     * @param portIndex Input port (must be 0)
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief Returns the embedded widget
     * 
     * The widget provides radio buttons for selecting erosion or dilation.
     * 
     * @return Pointer to CVErodeAndDilateEmbeddedWidget
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets properties from browser
     * 
     * Properties:
     * - "kernel_shape": Structuring element shape (RECT/ELLIPSE/CROSS)
     * - "kernel_width": Kernel width (odd int, >= 1)
     * - "kernel_height": Kernel height (odd int, >= 1)
     * - "iterations": Number of times to apply (int, >= 1)
     * - "border_type": Edge handling method
     * 
     * @param property Property name
     * @param value New value
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Provides thumbnail preview
     * 
     * @return QPixmap for node palette
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    /** @brief Category name */
    static const QString _category;

    /** @brief Model name */
    static const QString _model_name;

private Q_SLOTS:
    /**
     * @brief Handles operation selection from widget
     * 
     * Called when user clicks erode or dilate radio button.
     * Triggers reprocessing with the new operation.
     */
    void em_radioButton_clicked();

private:
    /**
     * @brief Performs erosion or dilation
     * 
     * Creates structuring element and applies selected operation:
     * - Erosion: cv::erode() - shrinks bright regions
     * - Dilation: cv::dilate() - expands bright regions
     * 
     * @param in Input image
     * @param out Output processed image
     * @param params Morphological parameters
     */
    void processData(const std::shared_ptr<CVImageData>& in, std::shared_ptr<CVImageData>& out, const CVErodeAndDilateParameters& params);

    /** @brief Current parameters */
    CVErodeAndDilateParameters mParams;
    
    /** @brief Output cache */
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    
    /** @brief Input cache */
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    
    /** @brief Embedded operation selector */
    CVErodeAndDilateEmbeddedWidget* mpEmbeddedWidget;
    
    /** @brief Preview pixmap */
    QPixmap _minPixmap;
};


