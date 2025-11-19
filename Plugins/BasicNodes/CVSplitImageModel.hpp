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
 * @file CVSplitImageModel.hpp
 * @brief Node model for splitting multi-channel images into separate channels
 * 
 * This file defines a node that decomposes a multi-channel image (e.g., RGB, BGR)
 * into individual channel images. Channel splitting is useful for analyzing or
 * processing color channels independently.
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
 * @struct SplitImageParameters
 * @brief Parameter structure for image channel splitting
 * 
 * Controls how channels are represented in the output images.
 */
typedef struct SplitImageParameters
{
    /** 
     * @brief Whether to maintain 3-channel output format
     * - false: Output single-channel grayscale images (e.g., 1-channel)
     * - true: Output 3-channel images with only one channel active (others zero)
     * 
     * Example for BGR input:
     * - false: Three 1-channel outputs (B, G, R)
     * - true: Three 3-channel outputs ([B,0,0], [0,G,0], [0,0,R])
     */
    bool mbMaintainChannels;
    
    /**
     * @brief Default constructor with single-channel output
     */
    SplitImageParameters()
        : mbMaintainChannels(false)
    {
    }
} SplitImageParameters;

/**
 * @class CVSplitImageModel
 * @brief Node model for decomposing images into color channels
 * 
 * This model splits multi-channel images using OpenCV's cv::split().
 * It separates the color channels of an image into individual outputs,
 * enabling independent processing of each channel.
 * 
 * How channel splitting works:
 * For a 3-channel BGR image:
 * ```
 * Input: [B,G,R] pixel values
 * Output Port 0: B channel (Blue)
 * Output Port 1: G channel (Green)
 * Output Port 2: R channel (Red)
 * ```
 * 
 * Channel meanings by color space:
 * - **BGR**: Blue, Green, Red (OpenCV default)
 * - **RGB**: Red, Green, Blue
 * - **HSV**: Hue, Saturation, Value
 * - **LAB**: L (lightness), A (green-red), B (blue-yellow)
 * - **YCrCb**: Y (luma), Cr (red-diff), Cb (blue-diff)
 * 
 * Common use cases:
 * - **Color analysis**: Process specific color channels (e.g., only red channel)
 * - **Channel enhancement**: Apply different filters to different channels
 * - **Feature extraction**: Use specific channels for detection (e.g., saturation for color segmentation)
 * - **Debugging**: Inspect individual channels to understand color distribution
 * - **Custom color operations**: Manipulate channels independently then merge
 * - **White balance**: Adjust individual channels for color correction
 * 
 * Input:
 * - Port 0: CVImageData - Multi-channel source image
 * 
 * Output:
 * - Port 0: CVImageData - First channel (e.g., Blue in BGR)
 * - Port 1: CVImageData - Second channel (e.g., Green in BGR)
 * - Port 2: CVImageData - Third channel (e.g., Red in BGR)
 * 
 * Design Note: The number of output ports is fixed at 3 for efficiency.
 * Images with fewer channels (e.g., grayscale) will produce empty outputs
 * on unused ports. Images with more than 3 channels will only split the
 * first 3 channels.
 * 
 * @note For merging channels back together, use MergeImageModel
 * @see cv::split for the underlying OpenCV operation
 * @see cv::merge for the inverse operation
 */
class CVSplitImageModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new channel split node
     * 
     * Initializes with single-channel output mode (mbMaintainChannels = false).
     */
    CVSplitImageModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVSplitImageModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the channel output format preference.
     * 
     * @return QJsonObject containing the maintain_channels parameter
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved channel format preference.
     * 
     * @param p QJsonObject containing the saved node configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 1 input port (multi-channel source image)
     * - 3 output ports (individual channels)
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
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Provides individual channel output
     * 
     * Returns the split channel data for the specified output port.
     * 
     * @param port The output port index (0 = channel 0, 1 = channel 1, 2 = channel 2)
     * @return Shared pointer to the channel CVImageData
     * @note Returns nullptr if no input has been processed or port is invalid
     */
    std::shared_ptr< NodeData >
    outData( PortIndex port ) override;

    /**
     * @brief Receives and processes input image data
     * 
     * When multi-channel image data arrives, this method:
     * 1. Validates the input has at least one channel
     * 2. Calls cv::split() to separate channels
     * 3. Optionally maintains 3-channel format with zeros
     * 4. Stores results for each output port
     * 5. Notifies connected nodes
     * 
     * @param nodeData The input CVImageData (multi-channel image)
     * @param portIndex The input port index (must be 0)
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

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
     * - "maintain_channels": Keep 3-channel output format (bool)
     * 
     * When this property changes, the node reprocesses current input
     * to apply the new format.
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

private:

    /**
     * @brief Internal helper to perform channel splitting
     * 
     * Executes the channel separation:
     * 1. Uses cv::split() to separate channels into vector of cv::Mat
     * 2. If mbMaintainChannels is true, converts each single-channel
     *    result back to 3-channel with zeros in other channels
     * 3. Populates output array with split channel data
     * 
     * Why maintain channels option exists:
     * - Some downstream nodes expect 3-channel input
     * - Easier visualization (can display as color images)
     * - Preserves original color space structure
     * 
     * @param in The input multi-channel CVImageData
     * @param out Array of 3 output CVImageData pointers (one per channel)
     * @param params The splitting parameters (maintain channels flag)
     * @note Uses cv::split() internally
     * @see cv::split
     */
    void processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr< CVImageData > (&out)[3], const SplitImageParameters &params);

    /** @brief Current split parameters */
    SplitImageParameters mParams;
    
    /** @brief Cached input image data */
    std::shared_ptr< CVImageData > mpCVImageInData {nullptr};
    
    /** @brief Cached split channel outputs (3 channels) */
    std::shared_ptr< CVImageData > mapCVImageData[3] {nullptr};

    /** @brief Preview pixmap for node palette */
    QPixmap _minPixmap;
};

