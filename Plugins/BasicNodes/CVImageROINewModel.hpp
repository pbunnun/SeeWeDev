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
 * @file CVImageROINewModel.hpp
 * @brief Node model for extracting a Region of Interest (ROI) from an image
 * 
 * This file defines a node that crops an image to a specified rectangular region.
 * ROI extraction is a fundamental operation in computer vision for focusing
 * processing on relevant areas and improving performance by reducing data size.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVImageROINewModel
 * @brief Node model for cropping images to a rectangular region of interest
 * 
 * This model extracts a rectangular subregion from an input image. ROI extraction
 * is one of the most common preprocessing operations in computer vision, used to:
 * - Focus processing on relevant image areas
 * - Reduce computational cost by processing smaller regions
 * - Extract detected objects for further analysis
 * - Create image patches for training datasets
 * - Isolate areas of interest before expensive operations
 * 
 * The ROI is defined by a cv::Rect structure with:
 * - (x, y): Top-left corner coordinates
 * - (width, height): Dimensions of the rectangle
 * 
 * Implementation details:
 * - Uses OpenCV's efficient matrix header sharing (no data copy)
 * - The output is a view into the input image's data (when possible)
 * - Automatically clamps ROI to image boundaries to prevent errors
 * - Preserves input image format (channels, depth, color space)
 * 
 * Common use cases:
 * - Cropping to detected faces or objects
 * - Processing only the central region of a wide-angle camera
 * - Extracting characters from text regions
 * - Creating training patches from large images
 * - Zooming into areas of interest
 * 
 * Input:
 * - Port 0: CVImageData - Source image to crop
 * 
 * Output:
 * - Port 0: CVImageData - Cropped region (subimage)
 * 
 * Design Note: The ROI rectangle is validated and clamped to image boundaries.
 * If the ROI extends beyond the image, it is automatically adjusted to fit.
 * This prevents crashes but may produce unexpected results if not configured properly.
 * 
 * @note For dynamic ROI based on detection results, connect CVRectData to future port
 * @see cv::Rect for the rectangle representation
 * @see cv::Mat::operator() for the underlying ROI extraction mechanism
 */
class CVImageROINewModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new image ROI extraction node
     * 
     * Initializes with default ROI:
     * - Position: (0, 0)
     * - Size: 640x480
     */
    CVImageROINewModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVImageROINewModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the current ROI rectangle for project persistence.
     * 
     * @return QJsonObject containing ROI x, y, width, height
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved ROI rectangle.
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
     * - 1 output port (cropped region)
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
     * @brief Provides the cropped ROI output
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the cropped CVImageData
     * @note Returns nullptr if no input has been processed
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input image data
     * 
     * When image data arrives, this method:
     * 1. Validates the input data
     * 2. Clamps ROI to image boundaries
     * 3. Extracts the rectangular subregion
     * 4. Stores the result for output
     * 5. Notifies connected nodes
     * 
     * @param nodeData The input CVImageData
     * @param portIndex The input port index (must be 0)
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    /**
     * @brief No embedded widget for this node
     * 
     * @return nullptr - Parameters are set via property browser
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets model properties from the property browser
     * 
     * Handles property changes for:
     * - "roi_x": X coordinate of top-left corner (int, >= 0)
     * - "roi_y": Y coordinate of top-left corner (int, >= 0)
     * - "roi_width": Width of the region (int, > 0)
     * - "roi_height": Height of the region (int, > 0)
     * 
     * When properties change, the node automatically reprocesses the current
     * input to extract the new ROI.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property
     * @note Values are clamped to valid ranges during processing
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /** @brief Category name for node organization */
    static const QString _category;

    /** @brief Display name for the node type */
    static const QString _model_name;

private:
    /**
     * @brief Internal helper to extract the ROI
     * 
     * Performs the ROI extraction:
     * 1. Validates ROI is within image bounds
     * 2. Clamps ROI rectangle if necessary
     * 3. Uses cv::Mat::operator()(Rect) to extract subimage
     * 4. Creates new CVImageData with the ROI
     * 
     * Why clamping is important:
     * - Prevents crashes from out-of-bounds access
     * - Allows graceful handling of misconfigured ROIs
     * - Useful when ROI comes from unreliable detection algorithms
     * 
     * @param in The input CVImageData to crop
     * @param out The output CVImageData to populate with ROI
     * @note The output shares data with input when possible (efficient)
     */
    void processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out );
    
    /** @brief Cached input image data */
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    
    /** @brief Cached ROI output data */
    std::shared_ptr<CVImageData> mpCVImageOutData { nullptr };

    /** 
     * @brief Rectangle defining the region of interest
     * @note Format: cv::Rect(x, y, width, height)
     * @note Automatically clamped to image boundaries during processing
     */
    cv::Rect mRectROI { cv::Rect( 0, 0, 640, 480 ) };
};


