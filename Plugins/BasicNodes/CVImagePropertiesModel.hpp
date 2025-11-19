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
 * @file CVImagePropertiesModel.hpp
 * @brief Image metadata inspection node for analyzing image characteristics.
 *
 * This utility node extracts and displays key properties of cv::Mat images,
 * providing essential metadata for debugging, validation, and pipeline design.
 * It analyzes image dimensions, channels, memory layout, and content characteristics
 * without modifying the image data.
 *
 * **Key Features**:
 * - Extracts dimensional properties (width, height, channels)
 * - Analyzes content characteristics (binary, grayscale, continuous)
 * - Provides formatted property descriptions
 * - Zero computational overhead (metadata only)
 * - Useful for debugging and validation
 *
 * @see cv::Mat properties and methods
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include <opencv2/core.hpp>
#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVImagePropertiesProperties
 * @brief Container for extracted image metadata and characteristics.
 *
 * This structure holds various properties extracted from cv::Mat images:
 *
 * **Basic Properties**:
 * - **msImageName**: User-defined identifier for the image
 * - **miChannels**: Number of color channels (1=grayscale, 3=BGR, 4=BGRA)
 * - **mCVMSizeImage**: Image dimensions (width, height)
 *
 * **Content Characteristics**:
 * - **mbIsBinary**: True if image contains only 0 and 255 values
 *   * Indicates binary/thresholded image
 *   * Useful for validating preprocessing results
 *
 * - **mbIsBAndW**: True if image is single-channel (grayscale)
 *   * Distinguishes grayscale from color images
 *   * Important for algorithm compatibility
 *
 * - **mbIsContinuous**: True if image data is stored in continuous memory
 *   * cv::Mat can have discontinuous rows (padding or ROI)
 *   * Continuous storage is faster for some operations
 *   * Most images are continuous unless created as ROI
 *
 * - **msDescription**: Human-readable summary of image properties
 *   * Formatted string with key information
 *   * Displayed in property browser
 *
 * **Use Cases**:
 * - Pipeline debugging: Verify image format at each stage
 * - Validation: Ensure input meets algorithm requirements
 * - Documentation: Record image characteristics for reports
 * - Conditional processing: Branch logic based on image type
 */
typedef struct CVImagePropertiesProperties
{
    std::string msImageName;     ///< User-defined image identifier
    int miChannels;              ///< Number of color channels (1, 3, or 4)
    cv::Size mCVMSizeImage;      ///< Image dimensions (width, height)

    bool mbIsBinary;             ///< True if image contains only 0 and 255 values
    bool mbIsBAndW;              ///< True if image is single-channel (grayscale)
    bool mbIsContinuous;         ///< True if image data is stored continuously in memory
    std::string msDescription;   ///< Human-readable property summary
    CVImagePropertiesProperties()
        : msImageName("ImageName"),
          miChannels(0),
          mCVMSizeImage(cv::Size(0,0)),
          mbIsBinary(true),
          mbIsBAndW(true),
          mbIsContinuous(true),
          msDescription("-")
    {
    }
} CVImagePropertiesProperties;


/**
 * @class CVImagePropertiesModel
 * @brief Extracts and displays metadata properties from images.
 *
 * This inspection node analyzes cv::Mat images and extracts key properties including
 * dimensions, channel count, memory layout, and content characteristics. It serves
 * as a diagnostic and validation tool, displaying properties in the property browser
 * without producing output data.
 *
 * **Functionality**:
 * - Extracts image dimensions (width x height)
 * - Determines channel count (1, 3, or 4)
 * - Checks memory continuity (affects performance)
 * - Analyzes content (binary, grayscale detection)
 * - Formats readable description for display
 *
 * **Input Port**:
 * - Port 0: CVImageData - Image to inspect
 *
 * **Output Ports**:
 * - None (properties displayed in property browser only)
 *
 * **Extracted Properties**:
 * 1. **Dimensions**: Image width and height in pixels
 * 2. **Channels**: 1 (grayscale), 3 (BGR), or 4 (BGRA)
 * 3. **Binary Detection**: Checks if image contains only 0 and 255 values
 * 4. **Grayscale Detection**: Verifies single-channel format
 * 5. **Memory Layout**: Checks if data is stored continuously (isContinuous)
 *
 * **Binary Detection Algorithm**:
 * Scans image pixels to verify all values are exactly 0 or 255:
 * ```
 * for each pixel p:
 *     if (p != 0 && p != 255):
 *         mbIsBinary = false
 * ```
 * This is useful for validating thresholding operations.
 *
 * **Common Use Cases**:
 * - **Debugging**: Verify image format at pipeline stages
 * - **Validation**: Ensure images meet algorithm requirements before processing
 * - **Documentation**: Record image characteristics for reports
 * - **Quality Assurance**: Verify preprocessing results (e.g., binary after threshold)
 * - **Pipeline Design**: Understand data flow and transformations
 * - **Troubleshooting**: Identify format mismatches causing errors
 *
 * **Typical Usage**:
 * ```
 * ImageSource → [Processing] → ImageProperties (inspect)
 *                            ↓
 *                         Display
 * ```
 * Insert ImageProperties node anywhere to inspect image state without affecting flow.
 *
 * **Property Browser Display**:
 * Properties are shown in the Qt property browser:
 * - Image Name: User-defined identifier
 * - Dimensions: "640 x 480" format
 * - Channels: "1" (grayscale), "3" (color), "4" (with alpha)
 * - Is Binary: "Yes" or "No"
 * - Is Grayscale: "Yes" or "No"
 * - Is Continuous: "Yes" or "No"
 * - Description: Summary string with all info
 *
 * **Memory Continuity**:
 * The isContinuous property indicates if image data is stored in a single
 * contiguous memory block:
 * - **Continuous**: Standard cv::Mat, faster access, better cache performance
 * - **Non-continuous**: ROI or submatrix, may have padding between rows
 * 
 * Most algorithms work with both, but continuous is generally faster.
 * To convert: `cv::Mat continuous = nonContinuous.clone();`
 *
 * **Performance**:
 * - Dimension/channel extraction: Instant (metadata access)
 * - Binary detection: O(N) where N = pixels (can be slow for large images)
 * - Total overhead: Minimal for small images, ~5-10ms for megapixel images
 *
 * **Design Decision**:
 * This node has no output ports, displaying results only in the property browser.
 * This design avoids cluttering the data flow graph while providing essential
 * inspection capabilities. For programmatic access to properties, use custom
 * nodes that output structured data.
 *
 * @see cv::Mat::channels() for channel count
 * @see cv::Mat::isContinuous() for memory layout
 * @see cv::Mat::size() for dimensions
 */
class CVImagePropertiesModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVImagePropertiesModel.
     */
    CVImagePropertiesModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVImagePropertiesModel() override {}

    /**
     * @brief Serializes model state to JSON.
     * @return QJsonObject containing image name and properties
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p JSON object with saved state
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the specified type.
     * @param portType Input or Output
     * @return 1 for Input (image), 0 for Output (no data output)
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for the specified port.
     * @param portType Input or Output
     * @param portIndex Port index
     * @return CVImageData for input port 0
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Sets input image data and triggers property extraction.
     * @param nodeData Input image data
     * @param port Input port index (0)
     *
     * Analyzes the input image and updates displayed properties.
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    /**
     * @brief No embedded widget for this node.
     * @return nullptr
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Updates model properties from the property browser.
     * @param property Property name (e.g., "image_name")
     * @param value New property value
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Returns the minimized pixmap icon for the node.
     * @return QPixmap representing the node in minimized state
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;    ///< Node category: "Utility"
    static const QString _model_name;  ///< Unique model name: "Image Properties"

private:

    /**
     * @brief Extracts properties from input image.
     * @param in Input image to analyze
     * @param props Properties structure to populate
     *
     * **Extraction Algorithm**:
     * ```cpp
     * cv::Mat image = in->getData();
     * 
     * // Basic properties
     * props.mCVMSizeImage = image.size();       // Width x Height
     * props.miChannels = image.channels();      // 1, 3, or 4
     * props.mbIsContinuous = image.isContinuous(); // Memory layout
     * props.mbIsBAndW = (props.miChannels == 1);   // Grayscale check
     * 
     * // Binary detection (scan all pixels)
     * props.mbIsBinary = true;
     * for (int y = 0; y < image.rows; y++) {
     *     for (int x = 0; x < image.cols; x++) {
     *         uchar value = image.at<uchar>(y, x);
     *         if (value != 0 && value != 255) {
     *             props.mbIsBinary = false;
     *             break;
     *         }
     *     }
     * }
     * 
     * // Create description string
     * props.msDescription = format("%dx%d, %d channels, %s, %s",
     *     props.mCVMSizeImage.width,
     *     props.mCVMSizeImage.height,
     *     props.miChannels,
     *     props.mbIsBinary ? "binary" : "non-binary",
     *     props.mbIsContinuous ? "continuous" : "non-continuous");
     * ```
     *
     * **Optimization Note**:
     * Binary detection scans all pixels, which can be slow for large images.
     * For performance-critical applications, this check could be made optional
     * or sampled rather than exhaustive.
     */
    void processData(const std::shared_ptr< CVImageData > & in, CVImagePropertiesProperties & props );

    CVImagePropertiesProperties mProps;                 ///< Current image properties
    std::shared_ptr<CVImageData> mpCVImageInData {nullptr}; ///< Input image data
    QPixmap _minPixmap;                                 ///< Minimized node icon
};

