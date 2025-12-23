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
 * @file CVImageLoaderModel.hpp
 * @brief Node model for loading and displaying static images
 * 
 * This file defines a node that loads image files from disk and outputs them
 * to the data flow graph. It provides an embedded widget for interactive file
 * selection and displays a thumbnail preview.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include "PBNodeDelegateModel.hpp"

#include "CVImageData.hpp"
#include "InformationData.hpp"
#include "SyncData.hpp"
#include "CVImageLoaderEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVImageLoaderModel
 * @brief Node model for loading static images from files
 * 
 * This model provides functionality to load image files in various formats
 * (PNG, JPEG, BMP, etc.) and emit them as CVImageData. The node features:
 * - Interactive file browser via embedded widget
 * - Thumbnail preview of loaded image
 * - Support for multiple image formats through OpenCV
 * - Automatic size detection and output
 * 
 * The node has no input ports and provides outputs for:
 * - Port 0: The loaded image as CVImageData
 * - Port 1: Image dimensions as CVSizeData
 * 
 * @note The embedded widget provides visual feedback and file selection UI
 * @see CVImageLoaderEmbeddedWidget for the user interface component
 * @see CVImageData for the output image format
 */
class CVImageLoaderModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new image loader node
     * 
     * Initializes the embedded widget and sets up signal/slot connections
     * for interactive file selection and widget resize events.
     */
    CVImageLoaderModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVImageLoaderModel() {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the currently loaded image filename to allow project persistence.
     * 
     * @return QJsonObject containing the node's configuration
     * @note Stores the image file path for restoration on project load
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads the previously saved image filename and attempts to reload
     * the image file from disk.
     * 
     * @param p QJsonObject containing the saved node configuration
     * @note If the file path is invalid, the node will be in an error state
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 0 input ports (image source, no inputs needed)
     * - 2 output ports (image data and size)
     * 
     * @param portType The type of port (In or Out)
     * @return Number of ports of the specified type
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port
     * 
     * Output ports provide:
     * - Port 0: CVImageData - The loaded image
     * - Port 1: CVSizeData - Image dimensions (width, height)
     * 
     * @param portType The type of port (In or Out)
     * @param portIndex The index of the port
     * @return NodeDataType describing the data type at this port
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Provides output data for a specific port
     * 
     * @param port The output port index (0 = image, 1 = size)
     * @return Shared pointer to the output data
     * @note Returns nullptr if no image is loaded
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives input data (not used in this node)
     * 
     * @param nodeData The input data (ignored)
     * @param portIndex The input port index
     * @note This node has no inputs, so this method does nothing
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    /**
     * @brief Returns the embedded widget for display in the node
     * 
     * @return Pointer to the CVImageLoaderEmbeddedWidget
     * @see CVImageLoaderEmbeddedWidget for widget implementation
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets model properties from the property browser
     * 
     * Handles property changes from the Qt Property Browser, particularly
     * the image filename property.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property
     * @note Currently handles the "image_filename" property
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    /**
     * @brief Enable or disable the node functionality
     * 
     * This method is called to enable or disable the node, affecting
     * its processing and UI elements.
     * 
     * @param enable True to enable the node, false to disable
     */
    void
    enable_changed(bool enable) override;

    /** @brief Category name for node organization */
    static const QString _category;

    /** @brief Display name for the node type */
    static const QString _model_name;

private Q_SLOTS:
    /**
     * @brief Handles button clicks from the embedded widget
     * 
     * Processes user interactions with the embedded widget buttons:
     * - Button 0: Load image from file
     * - Button 1: Clear loaded image
     * - Button 2: Reload current image
     * 
     * @param button The button identifier (0-based index)
     * @note This slot is connected to the embedded widget's button signals
     */
    void
    em_button_clicked( int button );

    /**
     * @brief Advances to the next image in a directory sequence
     * 
     * When a directory is loaded, this slot cycles through all images
     * in the directory, supporting slideshow functionality.
     * 
     * @note Controlled by mTimer for automatic playback
     */
    void
    flip_image( );

    /**
     * @brief Handles new input connection creation
     * 
     * Currently unused as this node has no input ports, but required
     * by the node framework.
     * 
     * @param connectionId The ID of the newly created connection
     */
    void
    inputConnectionCreated(QtNodes::ConnectionId const&) override;

    /**
     * @brief Handles input connection deletion
     * 
     * Currently unused as this node has no input ports, but required
     * by the node framework.
     * 
     * @param connectionId The ID of the deleted connection
     */
    void
    inputConnectionDeleted(QtNodes::ConnectionId const&) override;

private:
    /**
     * @brief Internal helper to set and load an image file
     * 
     * @param filename Path to the image file to load
     * @note Updates all output data and triggers re-rendering
     */
    void
    set_image_filename(QString &);
    
    /**
     * @brief Internal helper to load all images from a directory
     * 
     * @param dirname Path to the directory containing images
     * @note Populates mvsImageFilenames with all valid image files
     */
    void
    set_dirname(QString &);

    /** @brief Currently loaded image file path */
    QString msImageFilename {""};
    
    /** @brief Currently loaded directory path (for batch mode) */
    QString msDirname {""};
    
    /** @brief List of all image files in the loaded directory */
    std::vector<QString> mvsImageFilenames;
    
    /** @brief Current index in the image sequence (for slideshow) */
    int miFilenameIndex{ 0 };

    /** @brief Delay between images in slideshow mode (milliseconds) */
    int miFlipPeriodInMillisecond{ 1000 };
    
    /** @brief Timer for automatic image cycling in slideshow mode */
    QTimer mTimer;
    
    /** @brief Whether to loop back to first image after reaching the end */
    bool mbLoop{true};

    /** @brief Pointer to the embedded UI widget */
    CVImageLoaderEmbeddedWidget * mpEmbeddedWidget;

    /** @brief Cached output data: the loaded image */
    std::shared_ptr< CVImageData > mpCVImageData;
    
    /** @brief Cached output data: image metadata information */
    std::shared_ptr< InformationData > mpInformationData;
    
    /** @brief Cached output data: sync signal for synchronized processing */
    std::shared_ptr< SyncData > mpSyncData;

    // Information display flags - control what metadata is shown
    /** @brief Display timestamp in info panel */
    bool mbInfoTime{true};
    
    /** @brief Display image type (CV_8UC3, etc.) in info panel */
    bool mbInfoImageType{true};
    
    /** @brief Display file format (JPEG, PNG, etc.) in info panel */
    bool mbInfoImageFormat{true};
    
    /** @brief Display image dimensions in info panel */
    bool mbInfoImageSize{true};
    
    /** @brief Display filename in info panel */
    bool mbInfoImageFilename{true};

    /** @brief Enable synchronization signal mode for frame-by-frame control */
    bool mbUseSyncSignal{false};

    QPixmap _minPixmap;
};

