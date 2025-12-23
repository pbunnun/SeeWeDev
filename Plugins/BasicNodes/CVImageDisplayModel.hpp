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
 * @file CVImageDisplayModel.hpp
 * @brief Node model for displaying images in a standalone window
 * 
 * This file defines a node that receives image data from the data flow graph
 * and displays it in an embedded widget. It serves as a visualization endpoint
 * for image processing pipelines, allowing users to inspect intermediate and
 * final results.
 */

#pragma once

#include <QtCore/QObject>

#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "PBImageDisplayWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVImageDisplayModel
 * @brief Node model for real-time image visualization
 * 
 * This model provides functionality to receive and display images from
 * the data flow graph. Key features include:
 * - Real-time image display with automatic conversion from cv::Mat to QPixmap
 * - Resizable display window with aspect ratio preservation
 * - Support for various image formats (grayscale, RGB, BGR, etc.)
 * - Optional synchronization signal input for frame-controlled display
 * - Mouse event handling for interactive features
 * 
 * The node has inputs for:
 * - Port 0: CVImageData - The image to display
 * - Port 1: SyncData - Optional synchronization signal for controlled updates
 * 
 * This node has no outputs as it serves as a visualization endpoint.
 * 
 * Design Decision: Cannot be minimized to ensure the display remains visible.
 * This is critical for real-time monitoring during pipeline execution.
 * 
 * @note The embedded widget handles the actual image rendering and user interaction
 * @see PBImageDisplayWidget for the display implementation
 * @see CVImageData for the expected input data format
 */
class CVImageDisplayModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new image display node
     * 
     * Initializes the embedded display widget and sets up event filtering
     * for handling mouse interactions within the display area.
     */
    CVImageDisplayModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVImageDisplayModel() override {}

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 2 input ports (image data and optional sync signal)
     * - 0 output ports (display endpoint)
     * 
     * @param portType The type of port (In or Out)
     * @return Number of ports of the specified type
     */
    unsigned int
    nPorts( PortType portType ) const override;

    /**
     * @brief Returns the data type for a specific port
     * 
     * Input ports accept:
     * - Port 0: CVImageData - The image to display
     * - Port 1: SyncData - Synchronization signal for controlled updates
     * 
     * @param portType The type of port (In or Out)
     * @param portIndex The index of the port
     * @return NodeDataType describing the data type at this port
     */
    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Provides output data (not used in this node)
     * 
     * @param port The output port index
     * @return nullptr - This node has no outputs
     */
    std::shared_ptr<NodeData>
    outData(PortIndex) override;

    /**
     * @brief Receives and displays input image data
     * 
     * When image data arrives on port 0, this method:
     * 1. Extracts the cv::Mat from CVImageData
     * 2. Converts the image format if necessary (BGR to RGB)
     * 3. Creates a QPixmap for display
     * 4. Updates the embedded widget
     * 
     * Port 1 receives optional synchronization signals to control
     * when the display updates (useful for frame-by-frame inspection).
     * 
     * @param nodeData The input data (CVImageData or SyncData)
     * @param port The input port index (0 = image, 1 = sync)
     * @note Display updates are throttled based on sync signal when port 1 is connected
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    /**
     * @brief Returns the embedded widget for display in the node
     * 
     * @return Pointer to the PBImageDisplayWidget
     * @see PBImageDisplayWidget for widget implementation
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    QPixmap
    minPixmap() const override{ return _minPixmap; }

     /**
     * @brief Event filter for capturing widget events
     * 
     * Intercepts events from the embedded widget, particularly mouse events,
     * to enable interactive features like:
     * - Pixel value inspection on mouse hover
     * - Region selection for zooming
     * - Click-to-measure functionality
     * 
     * @param object The object that generated the event
     * @param event The event to process
     * @return true if the event was handled, false to pass it along
     */
    bool
    eventFilter(QObject *object, QEvent *event) override;

    /** @brief Category name for node organization */
    static const QString _category;

    /** @brief Display name for the node type */
    static const QString _model_name;

private:
    /**
     * @brief Internal helper to update the displayed image
     * 
     * Performs the actual image conversion and widget update:
     * 1. Converts cv::Mat to QImage (handling color space conversion)
     * 2. Creates QPixmap from QImage
     * 3. Updates the widget's pixmap
     * 4. Triggers widget repaint
     * 
     * @note This method handles various image formats (CV_8UC1, CV_8UC3, CV_8UC4)
     */
    void display_image( );

    /** @brief Pointer to the embedded display widget */
    PBImageDisplayWidget * mpEmbeddedWidget;

    /** @brief OpenCV image buffer for the current displayed image */
    cv::Mat mCVImageDisplay;
    
    /** @brief Cached pixmap for efficient rendering */
    QPixmap _minPixmap;

    /** @brief Width of the currently displayed image */
    int miImageWidth{0};
    
    /** @brief Height of the currently displayed image */
    int miImageHeight{0};
    
    /** @brief OpenCV format code of the current image (CV_8UC1, CV_8UC3, etc.) */
    int miImageFormat{0};

    /** @brief Synchronization data for controlled frame updates */
    std::shared_ptr< SyncData > mpSyncData { nullptr };
};


