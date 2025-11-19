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
 * @file CVOverlayImageModel.hpp
 * @brief Node model for overlaying one image on top of another at a specified position
 * 
 * This file defines a node that places a second image (overlay) on top of a base image
 * at a user-defined coordinate. The overlay is automatically cropped to fit within
 * the bounds of the base image.
 */

#pragma once

#include <iostream>

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
 * @class CVOverlayImageModel
 * @brief Node model for image overlay operations
 * 
 * This model overlays a second image on top of a base image at specified coordinates.
 * The overlay position is defined by (x, y) which determines where the top-left corner
 * (0, 0) of the overlay image will be placed on the base image.
 * 
 * Key characteristics:
 * - **Position control**: User-defined (x, y) offset for overlay placement
 * - **Automatic cropping**: Overlay is cropped to fit within base image bounds
 * - **Type matching**: Both images must have compatible types for overlay
 * - **Region of Interest**: Uses cv::Mat ROI for efficient copying
 * 
 * Common use cases:
 * - **Watermarking**: Add logo or watermark to images
 * - **Picture-in-picture**: Overlay smaller image on larger frame
 * - **Image composition**: Combine multiple image sources
 * - **Object insertion**: Place detected objects into scene
 * - **Augmented reality**: Overlay graphics on video frames
 * 
 * Input ports:
 * - Port 0: CVImageData - Base image (background)
 * - Port 1: CVImageData - Overlay image (foreground)
 * 
 * Output:
 * - Port 0: CVImageData - Resulting composite image
 * 
 * Properties:
 * - "offset_x": X-coordinate where overlay (0,0) is placed on base image
 * - "offset_y": Y-coordinate where overlay (0,0) is placed on base image
 * 
 * Design Note: Negative offsets are allowed and will crop the overlay from the
 * top-left. The overlay is automatically clipped to the base image boundaries.
 * 
 * @note Output is a copy of the base image with overlay applied
 * @see cv::Mat::copyTo, cv::Rect
 */
class CVOverlayImageModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new overlay image node
     * 
     * Initializes with offset (0, 0).
     */
    CVOverlayImageModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVOverlayImageModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the overlay position offsets.
     * 
     * @return QJsonObject containing node configuration
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved position settings.
     * 
     * @param p QJsonObject containing the saved node configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 2 input ports (base image, overlay image)
     * - 1 output port (result)
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
     * @brief Provides the overlay result
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the result CVImageData
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives input image data for overlay
     * 
     * Processes incoming data on either input port.
     * When both inputs are available, performs the overlay operation.
     * 
     * @param nodeData The input CVImageData
     * @param portIndex The input port (0=base, 1=overlay)
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief Indicates no embedded widget is provided.
     * @return nullptr (this is a pure processing node with no UI)
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets model properties from the property browser
     * 
     * Handles property changes for:
     * - "offset_x": X-coordinate offset for overlay placement
     * - "offset_y": Y-coordinate offset for overlay placement
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
     * @brief Handles input connection deletion
     * 
     * Releases the disconnected input data.
     * Triggered automatically by NodeEditor framework.
     */
    void
    inputConnectionDeleted(QtNodes::ConnectionId const&) override;

   
private:
    /**
     * @brief Internal helper to perform overlay operation
     * 
     * Creates a copy of the base image and overlays the second image at the
     * specified (x, y) position. Automatically crops the overlay to fit within
     * the base image bounds.
     * 
     * Algorithm:
     * 1. Calculate valid overlay region considering base image bounds
     * 2. Crop overlay if necessary to fit within base image
     * 3. Copy overlay to corresponding ROI in base image
     * 
     * @param in Array of 2 input images [base, overlay]
     * @param out Output image for the result
     */
    void processData(const std::vector< cv::Mat >&in, std::shared_ptr<CVImageData>& out);

    /** @brief Cached output result */
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    
    /** @brief Cached input images [base, overlay] */
    std::vector<cv::Mat> mvCVImageInData;

    /** @brief X-coordinate offset for overlay placement */
    int miOffsetX { 0 };
    
    /** @brief Y-coordinate offset for overlay placement */
    int miOffsetY { 0 };
    
    /** @brief Preview pixmap */
    QPixmap _minPixmap;
};
