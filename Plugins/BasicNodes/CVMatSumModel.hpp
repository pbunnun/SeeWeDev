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
 * @file CVMatSumModel.hpp
 * @brief Provides pixel sum calculation for images (matrix reduction to scalar).
 *
 * This file implements a node that computes the sum of all pixel values in an input
 * image using OpenCV's cv::sum() function. The operation reduces a 2D matrix (image)
 * to a scalar value representing the total intensity across all channels.
 *
 * Pixel sum is a fundamental image statistic useful for:
 * - Image brightness measurement (higher sum = brighter image)
 * - Change detection (compare sums across frames)
 * - Image validation (verify non-zero content)
 * - Threshold verification (check binary image white pixel count)
 * - Quality metrics (total energy, signal strength)
 *
 * Mathematical Operation:
 * For a single-channel image I with dimensions W×H:
 *   Sum = Σ(x=0 to W-1) Σ(y=0 to H-1) I(x, y)
 *
 * For multi-channel images (e.g., BGR), the sum is calculated per channel and
 * then aggregated (typically using the sum of all channel sums).
 *
 * Output:
 * - IntegerData containing the total pixel sum (integer value)
 * - For multi-channel: sum of all channels combined
 * - For binary images: count of white pixels (if values are 0 or 255)
 *
 * Typical Applications:
 * - Brightness estimation: Sum / (W × H) = average brightness
 * - Motion detection: Compare sum between consecutive frames
 * - Image validation: Sum == 0 indicates completely black image
 * - Threshold quality: Sum after threshold indicates foreground area
 * - Histogram moments: First moment calculation
 *
 * @see CVMatSumModel, cv::sum
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtWidgets/QPushButton>
#include "PBNodeDelegateModel.hpp"
#include "IntegerData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVMatSumModel
 * @brief Node that computes the sum of all pixel values in an image.
 *
 * This model provides a simple but useful image reduction operation: summing all pixel
 * intensities to produce a single scalar value. The operation uses OpenCV's cv::sum()
 * which efficiently computes the element-wise sum across the entire matrix.
 *
 * Functionality:
 * ```cpp
 * cv::Scalar channelSums = cv::sum(inputImage);
 * int totalSum = channelSums[0] + channelSums[1] + channelSums[2] + channelSums[3];
 * output = totalSum;
 * ```
 *
 * For Different Image Types:
 *
 * 1. Grayscale (1-channel):
 *    - Sum = Σ(all pixels)
 *    - Example: 100×100 image with average intensity 128 → Sum ≈ 1,280,000
 *
 * 2. BGR Color (3-channel):
 *    - Sum = Σ(B channel) + Σ(G channel) + Σ(R channel)
 *    - Example: 640×480 RGB image → Sum can be 0 to ~235,929,600 (for 8-bit)
 *
 * 3. Binary Image:
 *    - If values are 0 (black) and 255 (white): Sum / 255 = white pixel count
 *    - Example: Binary mask with 1000 white pixels → Sum = 255,000
 *
 * 4. Float Images:
 *    - Sum is floating-point (converted to integer for output)
 *    - Useful for normalized images (values 0.0 to 1.0)
 *
 * Common Use Cases:
 *
 * 1. Brightness Measurement:
 *    ```
 *    Camera → CVMatSum → Divide(by W×H) → Average Brightness
 *    ```
 *    Average = Sum / (width × height)
 *
 * 2. Change Detection (Frame Differencing):
 *    ```
 *    Frame(t) ┐
 *             ├→ AbsDiff → CVMatSum → Threshold → Alert if Sum > threshold
 *    Frame(t-1)┘
 *    ```
 *    High sum indicates significant changes between frames
 *
 * 3. Image Validation:
 *    ```
 *    LoadedImage → CVMatSum → Check(Sum > 0) → "Image not blank"
 *    ```
 *    Sum == 0 means completely black/empty image
 *
 * 4. White Pixel Counting in Binary Mask:
 *    ```
 *    Threshold → CVMatSum → Divide(by 255) → "White pixel count"
 *    ```
 *    For binary images (0 or 255), Sum/255 = number of white pixels
 *
 * 5. Motion Energy Calculation:
 *    ```
 *    OpticalFlow → Magnitude → CVMatSum → "Motion energy"
 *    ```
 *    Higher sum indicates more motion in the scene
 *
 * 6. Foreground Area Estimation:
 *    ```
 *    BackgroundSubtractor → CVMatSum → Scale → "Foreground coverage %"
 *    ```
 *
 * Mathematical Properties:
 * - Linearity: sum(α·I) = α·sum(I) for scalar α
 * - Additivity: sum(I₁ + I₂) = sum(I₁) + sum(I₂)
 * - Range for 8-bit images: [0, 255 × width × height × channels]
 * - Zero sum indicates all-black image (or negative values canceling positive)
 *
 * Performance Characteristics:
 * - Complexity: O(W × H × C) where W=width, H=height, C=channels
 * - Highly optimized in OpenCV (SIMD instructions used)
 * - Typical Time:
 *   * 640×480 grayscale: < 0.5ms
 *   * 1920×1080 BGR: ~2ms
 *   * 4K (3840×2160): ~8ms
 * - Memory: O(1) - no additional buffers required
 *
 * Output Data Type:
 * - IntegerData (32-bit signed integer)
 * - Range: -2,147,483,648 to 2,147,483,647
 * - Overflow Handling: For very large images or high bit-depths, sum may overflow
 *   * 8-bit 1920×1080 BGR: max sum ~1.5 billion (safe)
 *   * 16-bit images: risk of overflow (use cv::sum returns double internally)
 *
 * Practical Considerations:
 *
 * Overflow Risk:
 * For 8-bit BGR image with dimensions W×H:
 *   Max Sum = 255 × 3 × W × H
 *   Safe if: W × H < 2,796,202 (e.g., up to 1920×1080 is safe)
 *
 * Normalization:
 * To get average brightness per pixel:
 *   Average = Sum / (W × H × C)
 *
 * Binary Image White Count:
 * For thresholded image with values {0, 255}:
 *   White Pixels = Sum / 255
 *
 * Advantages:
 * - Extremely fast (SIMD-optimized)
 * - Simple, single-purpose node
 * - No parameters required
 * - Predictable behavior
 * - Low memory footprint
 *
 * Limitations:
 * - Only sum operation (no mean, std, min, max)
 * - Integer output (precision loss for large sums)
 * - Potential overflow for very large images or high bit-depths
 * - No per-channel breakdown (aggregates all channels)
 * - No spatial information (loses where bright pixels are located)
 *
 * Design Rationale:
 * - No Embedded Widget: Operation is parameter-free (pure calculation)
 * - IntegerData Output: Most common use cases involve integer arithmetic
 * - Single Output: Focused on one statistic (simplicity)
 *
 * Best Practices:
 * 1. For brightness: Divide sum by (W × H × C) to get average
 * 2. For binary masks: Divide by 255 to get white pixel count
 * 3. For change detection: Compute sum of absolute difference between frames
 * 4. For validation: Check sum > threshold to verify non-empty image
 * 5. For large images: Consider downsampling first to avoid overflow
 * 6. For float images: Be aware of integer conversion truncation
 *
 * Comparison with Alternatives:
 * - vs. cv::mean(): Sum requires division by area to get mean
 * - vs. cv::countNonZero(): CVMatSum works on all intensities, not just binary
 * - vs. Histogram: Sum is faster but provides less information
 * - vs. Custom Loop: cv::sum() is heavily optimized with SIMD
 *
 * Extension Ideas:
 * - Per-channel sum output (separate R, G, B sums)
 * - Double-precision output for high bit-depth images
 * - Additional statistics (mean, std dev) in same node
 * - Weighted sum (with mask or weighting matrix)
 *
 * @see cv::sum, IntegerData, CVImageData
 */
class CVMatSumModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVMatSumModel();

    virtual
    ~CVMatSumModel() override {}

    QJsonObject
    save() const override;

    void
    load(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the computed sum of all pixel values.
     *
     * The sum is calculated using cv::sum() and aggregated across all channels.
     *
     * @param port Output port index (always 0, single output)
     * @return Shared pointer to IntegerData containing pixel sum
     *
     * @note Returns 0 if no input image is available
     * @note Sum is recomputed only when input changes (cached otherwise)
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives input image and triggers sum calculation.
     *
     * When new image data arrives, this method computes the pixel sum:
     * ```cpp
     * cv::Scalar s = cv::sum(inputImage.data());
     * int totalSum = s[0] + s[1] + s[2] + s[3];  // Aggregate all channels
     * mpIntegerData = std::make_shared<IntegerData>(totalSum);
     * ```
     *
     * @param nodeData Input CVImageData containing image to sum
     * @param port Input port index (always 0, single input)
     *
     * @note Handles nullptr gracefully (sets sum to 0)
     * @note Works with any image type supported by cv::sum()
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override; 

    /**
     * @brief No embedded widget provided.
     * @return nullptr (parameter-free operation)
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Handles property changes from the property browser.
     *
     * Currently no properties to configure (sum operation is parameter-free).
     * Included for future extensibility (e.g., per-channel output, ROI specification).
     *
     * @param property Property name
     * @param value New property value
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /*
     * These two static members must be defined for every models. _category can be duplicate with existing categories.
     * However, _model_name has to be a unique name.
     */
    static const QString _category;

    static const QString _model_name;

private:
    /**
     * @brief Cached output containing the computed pixel sum.
     *
     * Updated whenever a new input image is received. Stores the aggregated sum
     * of all pixel values across all channels as a single integer.
     *
     * Calculation:
     * - cv::sum() returns cv::Scalar with per-channel sums
     * - All channel sums are added together
     * - Result stored as integer (may truncate for float images)
     *
     * Value Range:
     * - Minimum: 0 (all-black image, or negative overflow)
     * - Maximum: ~2 billion (limited by int32 range)
     * - Typical: Varies widely based on image content and size
     */
    std::shared_ptr< IntegerData > mpIntegerData;
};

