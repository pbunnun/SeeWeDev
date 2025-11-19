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
 * @file CVThresholdingModel.hpp
 * @brief Image thresholding node for binary segmentation and intensity remapping.
 *
 * This node performs pixel-wise thresholding operations, converting grayscale images
 * into binary or remapped outputs based on intensity criteria. Thresholding is one
 * of the most fundamental segmentation techniques, separating foreground from background
 * or isolating intensity ranges of interest.
 *
 * The node supports multiple thresholding types including binary, inverse binary,
 * truncate, to-zero, and Otsu's automatic threshold selection, providing flexibility
 * for various segmentation scenarios.
 *
 * **Key Applications**:
 * - Binary segmentation (foreground/background separation)
 * - Object extraction from uniform backgrounds
 * - Document binarization (text extraction)
 * - Pre-processing for contour detection
 * - Adaptive intensity remapping
 *
 * @see cv::threshold for thresholding implementation
 * @see cv::adaptiveThreshold for local adaptive thresholding
 */

#pragma once

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
 * @struct ThresholdingParameters
 * @brief Configuration for thresholding operations.
 *
 * This structure defines the threshold type, threshold value, and maximum value
 * for various thresholding modes.
 *
 * **Parameters**:
 *
 * - **miThresholdType**: Thresholding algorithm (default: THRESH_BINARY)
 *   * **THRESH_BINARY** (0): Binary thresholding
 *     \f$ dst(x,y) = \begin{cases} maxValue & \text{if } src(x,y) > threshold \\ 0 & \text{otherwise} \end{cases} \f$
 *     Use for: Standard foreground/background separation
 *
 *   * **THRESH_BINARY_INV** (1): Inverse binary thresholding
 *     \f$ dst(x,y) = \begin{cases} 0 & \text{if } src(x,y) > threshold \\ maxValue & \text{otherwise} \end{cases} \f$
 *     Use for: Dark objects on bright background
 *
 *   * **THRESH_TRUNC** (2): Truncate thresholding
 *     \f$ dst(x,y) = \begin{cases} threshold & \text{if } src(x,y) > threshold \\ src(x,y) & \text{otherwise} \end{cases} \f$
 *     Use for: Clipping bright values while preserving dark ones
 *
 *   * **THRESH_TOZERO** (3): To-zero thresholding
 *     \f$ dst(x,y) = \begin{cases} src(x,y) & \text{if } src(x,y) > threshold \\ 0 & \text{otherwise} \end{cases} \f$
 *     Use for: Removing low-intensity noise while keeping bright values
 *
 *   * **THRESH_TOZERO_INV** (4): Inverse to-zero thresholding
 *     \f$ dst(x,y) = \begin{cases} 0 & \text{if } src(x,y) > threshold \\ src(x,y) & \text{otherwise} \end{cases} \f$
 *     Use for: Removing high-intensity values while keeping dark ones
 *
 *   * **THRESH_OTSU** (8): Otsu's automatic threshold (can combine with above using bitwise OR)
 *     Automatically calculates optimal threshold using histogram analysis.
 *     Assumes bimodal distribution (two peaks). Returns calculated threshold.
 *     Use for: Automatic threshold selection when intensity distribution is bimodal
 *
 *   * **THRESH_TRIANGLE** (16): Triangle algorithm for automatic threshold
 *     Works well for unimodal histograms (single peak).
 *     Use for: Images without clear bimodal distribution
 *
 * - **mdThresholdValue**: Threshold value (default: 128)
 *   * Range: 0-255 for 8-bit images
 *   * Determines the cutoff point for classification
 *   * Ignored when using THRESH_OTSU or THRESH_TRIANGLE (auto-calculated)
 *   * Common values:
 *     - 128: Middle gray (equal split)
 *     - 0-50: For dark object extraction
 *     - 200-255: For bright object extraction
 *
 * - **mdBinaryValue**: Maximum value for binary modes (default: 255)
 *   * Used as the "high" value in THRESH_BINARY and THRESH_BINARY_INV
 *   * Typically 255 (white) for 8-bit images
 *   * Can be reduced for partial intensity output
 *
 * **Choosing a Threshold Value**:
 * 1. **Manual**: Analyze histogram to find valley between peaks
 * 2. **Otsu**: Automatic for bimodal distributions (two distinct intensity groups)
 * 3. **Triangle**: Automatic for skewed distributions (single peak)
 * 4. **Trial-and-error**: Adjust until desired segmentation achieved
 * 5. **Adaptive**: Use cv::adaptiveThreshold for varying illumination
 *
 * **Design Rationale**:
 * Default THRESH_BINARY with threshold=128 provides standard mid-level binary
 * segmentation, suitable for images with relatively uniform lighting and clear
 * intensity separation between foreground and background.
 */
typedef struct ThresholdingParameters{
    int miThresholdType;      ///< Threshold type: BINARY, BINARY_INV, TRUNC, TOZERO, TOZERO_INV, OTSU, TRIANGLE
    double mdThresholdValue;  ///< Threshold value (0-255 for 8-bit); ignored for OTSU/TRIANGLE
    double mdBinaryValue;     ///< Maximum value for binary modes (typically 255)
    ThresholdingParameters()
        : miThresholdType(cv::THRESH_BINARY),
          mdThresholdValue(128),
          mdBinaryValue(255)
    {
    }
} ThresholdingParameters;

/**
 * @class CVThresholdingModel
 * @brief Performs intensity-based image thresholding for segmentation.
 *
 * This segmentation node applies pixel-wise thresholding to grayscale images,
 * transforming continuous intensity values into discrete categories (typically
 * binary: foreground vs. background). It's the foundation of many computer vision
 * pipelines, converting complex images into simplified, analyzable forms.
 *
 * **Functionality**:
 * - Supports 5 basic threshold types plus automatic methods (Otsu, Triangle)
 * - Configurable threshold value and maximum value
 * - Outputs both thresholded image and calculated threshold (for auto methods)
 * - Operates on single-channel (grayscale) images
 *
 * **Input Port**:
 * - Port 0: CVImageData - Grayscale image (8-bit single channel)
 *
 * **Output Ports**:
 * - Port 0: CVImageData - Thresholded image
 * - Port 1: IntegerData - Calculated threshold value (useful for OTSU/TRIANGLE modes)
 *
 * **Processing Algorithm**:
 * ```cpp
 * double calculatedThreshold = cv::threshold(
 *     inputGray,              // Input grayscale image
 *     outputBinary,           // Output thresholded image
 *     params.mdThresholdValue, // Threshold value (or ignored for auto)
 *     params.mdBinaryValue,    // Max value for binary modes
 *     params.miThresholdType   // Threshold type
 * );
 * ```
 *
 * **Threshold Type Examples**:
 *
 * Given input pixel value = 150, threshold = 128, maxValue = 255:
 * - **THRESH_BINARY**: 150 > 128 → output = 255
 * - **THRESH_BINARY_INV**: 150 > 128 → output = 0
 * - **THRESH_TRUNC**: 150 > 128 → output = 128 (clipped)
 * - **THRESH_TOZERO**: 150 > 128 → output = 150 (preserved)
 * - **THRESH_TOZERO_INV**: 150 > 128 → output = 0 (removed)
 *
 * **Common Use Cases**:
 * - **Document Scanning**: Binarize text for OCR (BINARY or OTSU)
 * - **Object Detection**: Separate objects from background (BINARY, BINARY_INV)
 * - **Contour Detection**: Create binary input for findContours (BINARY)
 * - **Noise Removal**: Eliminate low-intensity noise (TOZERO)
 * - **Highlight Saturation**: Clip bright values (TRUNC)
 * - **Adaptive Segmentation**: Use OTSU for varying lighting conditions
 *
 * **Typical Pipelines**:
 * - ImageSource → Grayscale → **Threshold** → FindContours → Analysis
 * - Camera → **Threshold**(OTSU) → MorphologicalOps → BlobDetection
 * - Document → **Threshold**(BINARY, 200) → OCR
 *
 * **Otsu's Method**:
 * Automatically calculates optimal threshold by:
 * 1. Computing histogram of image
 * 2. Trying all possible thresholds
 * 3. Calculating between-class variance for each
 * 4. Selecting threshold that maximizes variance (best separation)
 *
 * **Advantages**: Fully automatic, no parameter tuning required
 * **Requirements**: Bimodal histogram (two distinct peaks)
 * **Limitation**: Fails on unimodal distributions (use Triangle instead)
 *
 * **Triangle Method**:
 * Geometric approach for skewed/unimodal histograms:
 * 1. Find histogram peak (mode)
 * 2. Draw line from peak to histogram end
 * 3. Find point with maximum perpendicular distance
 * 4. That point becomes the threshold
 *
 * **Performance**:
 * - Simple thresholding: ~0.5ms for 640x480 images (very fast)
 * - Otsu's method: ~2-3ms (includes histogram computation)
 * - Triangle method: ~2-3ms
 * All methods are real-time capable even for high-resolution images.
 *
 * **Design Decision**:
 * Default THRESH_BINARY with threshold=128 (middle gray) provides a reasonable
 * starting point for most images. Users should adjust based on:
 * - Histogram analysis (look for valley between peaks)
 * - Visual inspection of thresholded result
 * - Or switch to OTSU for automatic selection
 *
 * **Limitations and Alternatives**:
 * - **Global threshold**: Fails with uneven illumination → Use cv::adaptiveThreshold
 * - **Fixed threshold**: Fails with varying lighting → Use OTSU or TRIANGLE
 * - **Binary only**: Need multi-level → Use multiple thresholds or clustering
 *
 * @see cv::threshold for implementation details
 * @see cv::adaptiveThreshold for local adaptive thresholding
 * @see Otsu's paper (1979) for algorithm theory
 */
class CVThresholdingModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVThresholdingModel with binary threshold at 128.
     */
    CVThresholdingModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVThresholdingModel() override {}

    /**
     * @brief Serializes model parameters to JSON.
     * @return QJsonObject containing threshold type, value, and max value
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model parameters from JSON.
     * @param p JSON object with saved parameters
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the specified type.
     * @param portType Input or Output
     * @return 1 for Input (grayscale image), 2 for Output (thresholded + threshold value)
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for the specified port.
     * @param portType Input or Output
     * @param portIndex Port index
     * @return CVImageData for ports 0, IntegerData for output port 1
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the output data for the specified port.
     * @param port Output port index (0=image, 1=threshold value)
     * @return Shared pointer to CVImageData (port 0) or IntegerData (port 1)
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and triggers thresholding.
     * @param nodeData Input grayscale image
     * @param portIndex Input port index (0)
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief No embedded widget for this node.
     * @return nullptr
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Updates threshold parameters from the property browser.
     * @param property Property name (e.g., "threshold_type", "threshold_value", "max_value")
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

    static const QString _category;    ///< Node category: "Image Processing"
    static const QString _model_name;  ///< Unique model name: "Thresholding"

private:
    ThresholdingParameters mParams;                             ///< Threshold parameters (type, value, max)
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };   ///< Input grayscale image
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };     ///< Output thresholded image
    std::shared_ptr<IntegerData> mpIntegerData {nullptr};       ///< Output calculated threshold value
    QPixmap _minPixmap;                                         ///< Minimized node icon

    /**
     * @brief Processes data by applying threshold operation.
     * @param in Input grayscale image (8-bit single channel)
     * @param outImage Output thresholded image
     * @param outInt Output calculated threshold value
     * @param params Threshold parameters (type, value, max)
     *
     * **Algorithm**:
     * ```cpp
     * cv::Mat inputGray = in->getData();
     * cv::Mat outputThresholded;
     * 
     * // Apply threshold
     * double calculatedThreshold = cv::threshold(
     *     inputGray,
     *     outputThresholded,
     *     params.mdThresholdValue,  // Input threshold (or 0 for OTSU/TRIANGLE)
     *     params.mdBinaryValue,     // Max value for binary modes
     *     params.miThresholdType    // BINARY, BINARY_INV, TRUNC, TOZERO, TOZERO_INV, OTSU, TRIANGLE
     * );
     * 
     * // Store results
     * outImage->setData(outputThresholded);
     * outInt->setData(static_cast<int>(calculatedThreshold));
     * ```
     *
     * **Return Value**:
     * - For manual thresholding: Returns the input threshold value
     * - For OTSU/TRIANGLE: Returns the automatically calculated threshold
     *
     * **Input Requirements**:
     * - Image must be 8-bit single-channel (grayscale)
     * - For color images, convert to grayscale first using ColorSpaceModel
     *
     * **Output Format**:
     * - Output image has same size and type as input
     * - For binary modes: Pixel values are either 0 or maxValue
     * - For non-binary modes: Pixel values may be continuous
     *
     * **Combining Flags**:
     * Can combine threshold types with OTSU or TRIANGLE using bitwise OR:
     * ```cpp
     * cv::THRESH_BINARY | cv::THRESH_OTSU      // Binary with auto threshold
     * cv::THRESH_TRUNC | cv::THRESH_TRIANGLE   // Truncate with auto threshold
     * ```
     */
    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & outImage,
                      std::shared_ptr<IntegerData> &outInt, const ThresholdingParameters & params);
};

