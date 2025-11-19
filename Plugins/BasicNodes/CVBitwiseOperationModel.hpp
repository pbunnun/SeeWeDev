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
 * @file CVBitwiseOperationModel.hpp
 * @brief Node model for bitwise logical operations on images
 * 
 * This file defines a node that performs pixel-wise bitwise operations (AND, OR, XOR, NOT)
 * on images. Bitwise operations are fundamental for masking, combining binary images,
 * and performing logical operations on pixel values.
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
 * @class CVBitwiseOperationModel
 * @brief Node model for bitwise logical operations
 * 
 * This model performs pixel-wise bitwise operations using OpenCV's bitwise functions.
 * Bitwise operations treat each pixel value as a binary number and perform logical
 * operations bit-by-bit.
 * 
 * Available operations:
 * 
 * **AND (cv::bitwise_and):**
 * - Result pixel = Image1 & Image2
 * - Use cases: Apply mask to image, intersection of binary regions
 * - Example: mask & image keeps only pixels where mask is white
 * 
 * **OR (cv::bitwise_or):**
 * - Result pixel = Image1 | Image2
 * - Use cases: Combine binary masks, union of regions
 * - Example: mask1 | mask2 creates combined detection area
 * 
 * **XOR (cv::bitwise_xor):**
 * - Result pixel = Image1 ^ Image2
 * - Use cases: Find differences, toggle regions
 * - Example: image1 ^ image2 highlights changed pixels
 * 
 * **NOT (cv::bitwise_not):**
 * - Result pixel = ~Image
 * - Use cases: Invert binary mask, negative image
 * - Example: ~mask inverts black/white regions
 * 
 * Common use cases:
 * - **Masking**: Use AND to apply binary mask to image
 * - **ROI operations**: Combine multiple regions of interest
 * - **Background subtraction**: XOR for change detection
 * - **Mask inversion**: NOT to invert selection masks
 * - **Multi-object merging**: OR to combine detected objects
 * - **Conditional processing**: Mask determines which pixels to process
 * 
 * Input ports:
 * - Port 0: CVImageData - First input image
 * - Port 1: CVImageData - Second input image (not used for NOT)
 * - Port 2: CVImageData - Optional mask (when mbActiveMask = true)
 * 
 * Output:
 * - Port 0: CVImageData - Result of bitwise operation
 * 
 * Design Note: With masking enabled, the operation is only applied where
 * the mask is non-zero. Masked-out pixels remain zero in the output.
 * 
 * @note Operations work on all pixel types (8-bit, 16-bit, float, etc.)
 * @see cv::bitwise_and, cv::bitwise_or, cv::bitwise_xor, cv::bitwise_not
 */

enum BitwiseOperationType
{
    BITWISE_AND = 0,
    BITWISE_OR,
    BITWISE_XOR,
    BITWISE_NOT
};

class CVBitwiseOperationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new bitwise operation node
     * 
     * Initializes with AND operation and mask disabled.
     */
    CVBitwiseOperationModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVBitwiseOperationModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the operation type and mask activation state.
     * 
     * @return QJsonObject containing bitwise type and mask properties
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved operation settings.
     * 
     * @param p QJsonObject containing the saved node configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 3 input ports (image1, image2, optional mask)
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
     * @brief Provides the bitwise operation result
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the result CVImageData
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives input image data for bitwise operations
     * 
     * Processes incoming data on any of the three input ports.
     * When sufficient inputs are available, performs the selected operation.
     * 
     * @param nodeData The input CVImageData
     * @param portIndex The input port (0=img1, 1=img2, 2=mask)
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief Returns the embedded widget showing current operation
     * @return QLabel widget displaying the current operation type (AND/OR/XOR/NOT)
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets model properties from the property browser
     * 
     * Handles property changes for:
     * - "bitwise_type": Bitwise operation type (AND/OR/XOR/NOT)
     * 
     * Updates the embedded widget label and triggers reprocessing.
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
     * @brief Handles new input connection for mask port
     * 
     * Sets mbMaskActive to true when port 2 (mask) is connected.
     * Triggered automatically by NodeEditor framework. 
     */
    void
    inputConnectionCreated(QtNodes::ConnectionId const&) override;

    /**
     * @brief Handles mask port disconnection
     * 
     * Sets mbMaskActive to false and releases the mask data when port 2 is disconnected.
     * Triggers reprocessing if sufficient inputs remain available.
     * Triggered automatically by NodeEditor framework.
     */
    void
    inputConnectionDeleted(QtNodes::ConnectionId const&) override;

   
private:
    /**
     * @brief Internal helper to perform bitwise operations
     * 
     * Executes the selected bitwise operation:
     * - AND: cv::bitwise_and(img1, img2, temp, mask) then temp.copyTo(output)
     * - OR: cv::bitwise_or(img1, img2, temp, mask) then temp.copyTo(output)
     * - XOR: cv::bitwise_xor(img1, img2, temp, mask) then temp.copyTo(output)
     * - NOT: cv::bitwise_not(img1, temp, mask) then temp.copyTo(output)
     * 
     * Uses temporary cv::Mat to prevent reference aliasing issues.
     * 
     * @param in Array of 3 input images [img1, img2, mask]
     * @param out Output image for the result
     * @param type The bitwise operation type to perform
     */
    void processData(const std::vector< cv::Mat >&in, std::shared_ptr<CVImageData>& out, const BitwiseOperationType type);

    /** @brief Embedded widget displaying current operation type */
    QLabel* mpEmbeddedWidget { nullptr };

    /** @brief Current operation parameters */
    BitwiseOperationType mBitwiseOperationType;
     
    /** @brief Cached output result */
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    
    /** @brief Cached input images [img1, img2, mask] */
    std::vector<cv::Mat> mvCVImageInData;

    /** @brief Mask activation flag (true when port 2 is connected) */
    bool mbMaskActive { false };
    
    /** @brief Preview pixmap */
    QPixmap _minPixmap;
};


