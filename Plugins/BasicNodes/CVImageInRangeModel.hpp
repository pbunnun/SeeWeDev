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
 * @file CVImageInRangeModel.hpp
 * @brief Node model for thresholding images to create binary masks
 * 
 * This file defines a node that applies threshold operations to convert grayscale
 * images into binary images. Thresholding is a fundamental segmentation technique
 * that separates objects from background based on pixel intensity.
 */

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct InRangeParameters
 * @brief Parameter structure for thresholding operations
 * 
 * Configures the threshold operation type and threshold values.
 * Despite the name "InRange", this uses OpenCV's threshold function.
 */
typedef struct InRangeParameters{
    /** 
     * @brief Type of thresholding operation
     * @see cv::ThresholdTypes (THRESH_BINARY, THRESH_BINARY_INV, THRESH_TRUNC, etc.)
     */
    int miThresholdType;
    
    /** 
     * @brief Threshold value for comparison
     * @note Pixels are compared against this value
     */
    double mdThresholdValue;
    
    /** 
     * @brief Value assigned to pixels that pass the threshold
     * @note Typically 255 for binary images (white)
     */
    double mdBinaryValue;
    
    /**
     * @brief Default constructor with binary threshold at 128
     */
    InRangeParameters()
        : miThresholdType(cv::THRESH_BINARY),
          mdThresholdValue(128),
          mdBinaryValue(255)
    {
    }
} InRangeParameters;

/**
 * @class CVImageInRangeModel
 * @brief Node model for image thresholding and binarization
 * 
 * This model applies thresholding using OpenCV's cv::threshold() function.
 * Thresholding converts grayscale images to binary by comparing each pixel
 * against a threshold value.
 * 
 * Available threshold types:
 * 
 * **THRESH_BINARY:**
 * - dst = (src > thresh) ? maxVal : 0
 * - Use: Create binary mask where bright pixels are white
 * 
 * **THRESH_BINARY_INV:**
 * - dst = (src > thresh) ? 0 : maxVal
 * - Use: Inverted binary mask (bright becomes black)
 * 
 * **THRESH_TRUNC:**
 * - dst = (src > thresh) ? thresh : src
 * - Use: Cap bright values at threshold
 * 
 * **THRESH_TOZERO:**
 * - dst = (src > thresh) ? src : 0
 * - Use: Keep only pixels above threshold
 * 
 * **THRESH_TOZERO_INV:**
 * - dst = (src > thresh) ? 0 : src
 * - Use: Keep only pixels below threshold
 * 
 * **THRESH_OTSU (auto):**
 * - Automatically calculates optimal threshold
 * - Use: When threshold value is unknown
 * 
 * Common use cases:
 * - **Object segmentation**: Separate foreground from background
 * - **Preprocessing**: Create binary mask for contour detection
 * - **Document processing**: Binarize scanned documents
 * - **QR code detection**: Threshold for barcode readers
 * - **Shadow removal**: Separate lit areas from shadows
 * - **Motion detection**: Threshold difference images
 * 
 * Input:
 * - Port 0: CVImageData - Grayscale source image
 * 
 * Output:
 * - Port 0: CVImageData - Binary thresholded image
 * - Port 1: IntegerData - Number of non-zero pixels (white pixels in result)
 * 
 * Design Note: The second output provides a count of pixels that passed
 * the threshold, useful for area measurement or threshold validation.
 * 
 * @note For color images, convert to grayscale first (ColorSpaceModel)
 * @note For adaptive thresholding, use a different node (not implemented here)
 * @see cv::threshold for the underlying OpenCV operation
 * @see cv::ThresholdTypes for available threshold modes
 */
class CVImageInRangeModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new threshold node
     * 
     * Initializes with binary threshold at 128.
     */
    CVImageInRangeModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVImageInRangeModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves threshold type, value, and binary maximum.
     * 
     * @return QJsonObject containing threshold parameters
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved threshold settings.
     * 
     * @param p QJsonObject containing the saved configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports
     * 
     * - 1 input port (grayscale image)
     * - 2 output ports (binary image, pixel count)
     * 
     * @param portType The type of port
     * @return Number of ports
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a port
     * 
     * Input: CVImageData
     * Output 0: CVImageData (binary)
     * Output 1: IntegerData (pixel count)
     * 
     * @param portType The type of port
     * @param portIndex The port index
     * @return NodeDataType
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Provides output data
     * 
     * @param port 0=binary image, 1=pixel count
     * @return Shared pointer to output data
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input
     * 
     * Applies threshold and counts non-zero pixels.
     * 
     * @param nodeData Input grayscale image
     * @param portIndex Input port (must be 0)
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief No embedded widget
     * 
     * @return nullptr
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets properties from browser
     * 
     * Properties:
     * - "threshold_type": Type of thresholding
     * - "threshold_value": Threshold level (0-255)
     * - "binary_value": Max value for binary (usually 255)
     * 
     * @param property Property name
     * @param value New value
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /** @brief Category name */
    static const QString _category;

    /** @brief Model name */
    static const QString _model_name;

private:
    /**
     * @brief Performs threshold operation
     * 
     * Applies cv::threshold() and counts result pixels.
     * 
     * @param in Input grayscale image
     * @param outImage Output binary image
     * @param outInt Output pixel count
     * @param params Threshold parameters
     */
    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & outImage,
                      std::shared_ptr<IntegerData> &outInt, const InRangeParameters & params);

    /** @brief Threshold parameters */
    InRangeParameters mParams;
    
    /** @brief Input image cache */
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    
    /** @brief Binary output cache */
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    
    /** @brief Pixel count output */
    std::shared_ptr<IntegerData> mpIntegerData {nullptr};
};


