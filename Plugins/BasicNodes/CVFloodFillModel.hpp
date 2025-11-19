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
 * @file CVFloodFillModel.hpp
 * @brief Flood fill node for region filling and interactive segmentation.
 *
 * This node performs flood fill operations (also known as seed fill or bucket fill),
 * filling connected regions of similar colors starting from a seed point. It's the
 * computer vision equivalent of the "paint bucket" tool in image editors.
 *
 * The node supports both simple filling and tolerance-based filling with upper/lower
 * difference bounds, enabling segmentation of regions with gradual color variations.
 * Optional mask output allows tracking of filled regions.
 *
 * **Key Applications**:
 * - Interactive image segmentation
 * - Region labeling and extraction
 * - Background removal
 * - Color replacement
 * - Connected component extraction from seed point
 * - Defect detection and marking
 *
 * @see cv::floodFill for implementation details
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "CVFloodFillEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVFloodFillParameters
 * @brief Configuration parameters for flood fill operations.
 *
 * This structure controls all aspects of the flood fill algorithm including seed point,
 * fill color, tolerance ranges, boundary constraints, connectivity, and mask generation.
 *
 * **Parameters**:
 *
 * - **mCVPointSeed**: Starting point for flood fill (default: 0, 0)
 *   * Coordinates (x, y) where filling begins
 *   * Must be within image boundaries
 *   * Typically set via interactive point selection in embedded widget
 *
 * - **mucFillColor[4]**: Fill color {B, G, R, Grayscale} (default: {0, 0, 0, 0})
 *   * Color to use for filling matched regions
 *   * For BGR images: use [0], [1], [2] for blue, green, red channels
 *   * For grayscale: use [3]
 *   * Example: {0, 255, 0, 0} = bright green fill
 *
 * - **mucLowerDiff[4]**: Lower tolerance bounds {B, G, R, Gray} (default: {0, 0, 0, 0})
 *   * Maximum difference below seed pixel value to include in fill
 *   * Per-channel tolerance for color images
 *   * Example: {10, 10, 10, 0} allows pixels 10 units darker in each channel
 *
 * - **mucUpperDiff[4]**: Upper tolerance bounds {B, G, R, Gray} (default: {0, 0, 0, 0})
 *   * Maximum difference above seed pixel value to include in fill
 *   * Per-channel tolerance for color images
 *   * Example: {10, 10, 10, 0} allows pixels 10 units brighter in each channel
 *
 * - **mbDefineBoundaries**: Enable rectangular boundary constraint (default: false)
 *   * When true, restricts filling to rectangle defined by mCVPointRect1 and mCVPointRect2
 *   * When false, fills entire connected region within tolerance
 *   * Useful for limiting fill scope in large uniform regions
 *
 * - **mCVPointRect1, mCVPointRect2**: Bounding rectangle corners (default: 0, 0)
 *   * Defines rectangular region to constrain filling (if mbDefineBoundaries = true)
 *   * mCVPointRect1: Top-left corner
 *   * mCVPointRect2: Bottom-right corner
 *
 * - **miFlags**: Flood fill behavior flags (default: 4)
 *   * Bit field controlling connectivity and comparison mode
 *   * **Connectivity** (lower bits 0-7):
 *     - 4: 4-connected (N, S, E, W neighbors)
 *     - 8: 8-connected (includes diagonals)
 *   * **Comparison mode** (flag bits):
 *     - cv::FLOODFILL_FIXED_RANGE: Compare to seed pixel (default)
 *     - 0: Compare to neighbor pixels (gradient fill)
 *   * **Mask mode** (flag bit):
 *     - cv::FLOODFILL_MASK_ONLY: Update mask but don't fill image
 *   
 *   Common combinations:
 *   - 4: 4-connected, fixed range
 *   - 8: 8-connected, fixed range
 *   - 4 | cv::FLOODFILL_FIXED_RANGE: Explicit 4-connected fixed range
 *
 * - **miMaskColor**: Mask fill value (default: 255)
 *   * Value to write in mask for filled regions
 *   * Typically 255 (white) to mark filled area
 *   * Mask is 1 pixel larger on each side than input image
 *
 * **Tolerance Semantics**:
 * For FLOODFILL_FIXED_RANGE mode (default):
 * A pixel at position (x, y) is filled if:
 * \f[
 * \text{seedValue} - \text{lowerDiff} \leq \text{pixel}(x,y) \leq \text{seedValue} + \text{upperDiff}
 * \f]
 *
 * For gradient mode (without FIXED_RANGE):
 * Comparison is to neighbor pixels, allowing gradual color changes.
 *
 * **Design Rationale**:
 * Default zero tolerances create exact color matching (fill only identical pixels).
 * Users increase tolerances to handle noise and gradual variations. The 4-connected
 * default is more conservative, preventing diagonal leakage through corner touches.
 */
typedef struct CVFloodFillParameters{
    cv::Point mCVPointSeed;       ///< Seed point to start filling (x, y)
    int mucFillColor[4];          ///< Fill color {B, G, R, Grayscale}
    int mucLowerDiff[4];          ///< Lower difference tolerance {B, G, R, Gray}
    int mucUpperDiff[4];          ///< Upper difference tolerance {B, G, R, Gray}
    bool mbDefineBoundaries;      ///< Enable rectangular boundary constraint
    cv::Point mCVPointRect1;      ///< Bounding rectangle top-left corner
    cv::Point mCVPointRect2;      ///< Bounding rectangle bottom-right corner
    int miFlags;                  ///< Connectivity (4/8) and behavior flags
    int miMaskColor;              ///< Value for mask pixels in filled region
    CVFloodFillParameters()
        : mCVPointSeed(cv::Point(0,0)),
          mucFillColor{0},
          mucLowerDiff{0},
          mucUpperDiff{0},
          mbDefineBoundaries(false),
          mCVPointRect1(cv::Point(0,0)),
          mCVPointRect2(cv::Point(0,0)),
          miFlags(4),
          miMaskColor(255)
    {
    }
} CVFloodFillParameters;

/**
 * @struct CVFloodFillProperties
 * @brief Runtime properties for flood fill state.
 *
 * Tracks whether a mask is actively being generated during flood fill operations.
 *
 * - **mbActiveMask**: True if mask output is enabled (default: false)
 *   * When true, generates a binary mask showing filled regions
 *   * Mask is output on second port
 *   * Useful for extracting segmentation results
 */
typedef struct CVFloodFillProperties
{
    bool mbActiveMask;  ///< Enable mask generation and output
    CVFloodFillProperties()
        : mbActiveMask(false)
    {
    }
} CVFloodFillProperties;


/**
 * @class CVFloodFillModel
 * @brief Interactive flood fill node for region-based image segmentation.
 *
 * This segmentation node performs flood fill operations starting from a user-specified
 * seed point, filling connected regions of similar colors. It combines the functionality
 * of interactive segmentation tools with tolerance-based region growing, enabling both
 * exact color matching and fuzzy similarity-based filling.
 *
 * **Functionality**:
 * - Interactive seed point selection via embedded widget
 * - Tolerance-based similarity matching (adjustable per channel)
 * - 4-connected or 8-connected region filling
 * - Optional rectangular boundary constraints
 * - Dual output: filled image and binary mask
 * - Support for both grayscale and color images
 *
 * **Input Ports**:
 * - Port 0: CVImageData - Image to fill
 * - Port 1: CVImageData - Optional mask input (constrains fillable regions)
 *
 * **Output Ports**:
 * - Port 0: CVImageData - Image with filled region
 * - Port 1: CVImageData - Binary mask of filled region (if active)
 *
 * **Embedded Widget**:
 * - Interactive point selection for seed placement
 * - Real-time parameter adjustment (color, tolerance, connectivity)
 * - Visual feedback of fill parameters
 *
 * **Flood Fill Algorithm**:
 * 1. Start at seed point
 * 2. Check if pixel is within tolerance of seed color
 * 3. If yes, fill with target color and mark in mask
 * 4. Recursively/iteratively check neighbors (4 or 8 connected)
 * 5. Continue until no more pixels match criteria
 *
 * **Tolerance Modes**:
 *
 * 1. **Fixed Range** (default, FLOODFILL_FIXED_RANGE):
 *    - Compare all pixels to original seed pixel value
 *    - Fills region where: |pixel - seed| ≤ tolerance
 *    - More predictable, prevents gradient drift
 *    - Use for: Uniform regions with noise
 *
 * 2. **Gradient Mode** (without FIXED_RANGE flag):
 *    - Compare each pixel to its filled neighbors
 *    - Fills region following gradual color changes
 *    - Can "leak" through gradients
 *    - Use for: Regions with gradual transitions
 *
 * **Connectivity Impact**:
 * Consider a checkerboard pattern:
 * ```
 * X . X
 * . X .
 * X . X
 * ```
 * - **4-connected**: Fills only seed pixel (no edge neighbors match)
 * - **8-connected**: Fills all X pixels (diagonal connection allowed)
 *
 * **Common Use Cases**:
 *
 * 1. **Interactive Segmentation**:
 *    - User clicks on object to select it
 *    - Flood fill extracts connected region
 *    - Useful for manual annotation tools
 *
 * 2. **Background Removal**:
 *    - Click on uniform background
 *    - Fill with transparency or specific color
 *    - Extract foreground object
 *
 * 3. **Defect Detection**:
 *    - Click on defect region
 *    - Flood fill highlights entire defect
 *    - Measure filled area
 *
 * 4. **Color Replacement**:
 *    - Select region by clicking
 *    - Replace with new color
 *    - Preserve region boundaries
 *
 * 5. **Region Labeling**:
 *    - Use mask output to extract filled region
 *    - Combine multiple fills for multi-region segmentation
 *
 * **Typical Pipelines**:
 * - Image → **CVFloodFill** (interactive) → MaskOutput → AnalyzeRegion
 * - Image → **CVFloodFill** → ColorReplacement → Display
 * - GrabCut → **CVFloodFill** (refine) → FinalSegmentation
 *
 * **Mask Input/Output**:
 * - **Input mask** (port 1): Non-zero pixels cannot be filled (protection)
 * - **Output mask** (port 1): Binary image showing filled region
 * - Mask is (width+2) × (height+2) to handle boundary conditions
 *
 * **Performance**:
 * - Simple fill: ~1-5ms for 640x480 (depends on region size)
 * - Large regions: Can take 50-100ms for entire image
 * - 4-connected is slightly faster than 8-connected
 * - Most time spent on region traversal
 *
 * **Tips for Effective Use**:
 * 1. **Zero tolerance**: Exact color match, for uniform regions
 * 2. **Small tolerance (5-10)**: Handle minor noise and compression artifacts
 * 3. **Large tolerance (20-50)**: Fill regions with gradual variations
 * 4. **Use boundaries**: Prevent fill from leaking to unwanted areas
 * 5. **4-connected**: More conservative, less leakage
 * 6. **8-connected**: More inclusive, fills diagonal touches
 *
 * **Comparison with Other Segmentation**:
 * - **CVFloodFill**: Interactive, tolerance-based, seed-driven
 * - **Threshold**: Global, intensity-based, automatic
 * - **GrabCut**: Interactive, graph-based, more sophisticated
 * - **Watershed**: Marker-based, handles touching objects
 *
 * Use CVFloodFill for quick interactive segmentation of uniform regions.
 *
 * @see cv::floodFill for algorithm implementation
 * @see GrabCut for advanced interactive segmentation
 * @see Watershed for marker-based segmentation
 */
class CVFloodFillModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVFloodFillModel with default parameters.
     */
    CVFloodFillModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVFloodFillModel() override {}

    /**
     * @brief Serializes model parameters to JSON.
     * @return QJsonObject containing flood fill configuration
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
     * @return 2 for both Input (image + optional mask) and Output (filled + mask)
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for the specified port.
     * @param portType Input or Output
     * @param portIndex Port index (0 or 1)
     * @return CVImageData for all ports
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the output data for the specified port.
     * @param port Output port index (0=filled image, 1=mask)
     * @return Shared pointer to CVImageData
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and triggers flood fill operation.
     * @param nodeData Input image (port 0) or mask (port 1)
     * @param portIndex Input port index
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;

    /**
     * @brief Returns the embedded widget for interactive control.
     * @return CVFloodFillEmbeddedWidget for seed point selection and parameters
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Updates flood fill parameters from the property browser.
     * @param property Property name
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
    static const QString _model_name;  ///< Unique model name: "Flood Fill"

private Q_SLOTS:

    /**
     * @brief Handles spinbox value changes from embedded widget.
     * @param spinbox Spinbox identifier
     * @param value New spinbox value
     */
    void em_spinbox_clicked( int spinbox, int value );

private:
    CVFloodFillParameters mParams;                    ///< Flood fill configuration parameters
    CVFloodFillProperties mProps;                     ///< Runtime properties (mask active state)
    std::shared_ptr<CVImageData> mapCVImageData[2] { {nullptr} };   ///< Output: [0]=filled image, [1]=mask
    std::shared_ptr<CVImageData> mapCVImageInData[2] { {nullptr} }; ///< Input: [0]=image, [1]=optional mask
    CVFloodFillEmbeddedWidget* mpEmbeddedWidget;      ///< Interactive control widget
    QPixmap _minPixmap;                             ///< Minimized node icon

    static const std::string color[4];              ///< Color channel names for UI

    /**
     * @brief Processes data by performing flood fill operation.
     * @param in Input images [0]=image to fill, [1]=optional mask
     * @param out Output images [0]=filled image, [1]=fill mask
     * @param params Flood fill parameters (seed, color, tolerance, etc.)
     * @param props Runtime properties (mask activation)
     *
     * **Algorithm**:
     * ```cpp
     * // Clone input to preserve original
     * cv::Mat output = in[0]->getData().clone();
     * 
     * // Create or use existing mask (must be 2 pixels larger on each dimension)
     * cv::Mat mask;
     * if (in[1] != nullptr) {
     *     mask = in[1]->getData();  // Use provided mask
     * } else if (props.mbActiveMask) {
     *     mask = cv::Mat::zeros(output.rows + 2, output.cols + 2, CV_8U);  // Create new
     * }
     * 
     * // Set up fill parameters
     * cv::Scalar fillColor(params.mucFillColor[0], params.mucFillColor[1], 
     *                      params.mucFillColor[2]);
     * cv::Scalar lowerDiff(params.mucLowerDiff[0], params.mucLowerDiff[1], 
     *                      params.mucLowerDiff[2]);
     * cv::Scalar upperDiff(params.mucUpperDiff[0], params.mucUpperDiff[1], 
     *                      params.mucUpperDiff[2]);
     * 
     * // Optional boundary rectangle
     * cv::Rect* pRect = nullptr;
     * if (params.mbDefineBoundaries) {
     *     pRect = new cv::Rect(params.mCVPointRect1, params.mCVPointRect2);
     * }
     * 
     * // Perform flood fill
     * int area = cv::floodFill(
     *     output,                 // Image to fill (modified in-place)
     *     mask,                   // Optional mask (width+2 x height+2)
     *     params.mCVPointSeed,    // Seed point to start filling
     *     fillColor,              // New color for filled region
     *     pRect,                  // Optional bounding rectangle (filled, returns actual bounds)
     *     lowerDiff,              // Max color/brightness difference below seed
     *     upperDiff,              // Max color/brightness difference above seed
     *     params.miFlags          // Connectivity (4/8) and mode flags
     * );
     * 
     * // Output results
     * out[0]->setData(output);           // Filled image
     * if (props.mbActiveMask && mask.data) {
     *     out[1]->setData(mask);         // Fill mask
     * }
     * ```
     *
     * **Return Value**:
     * cv::floodFill returns the number of pixels filled (area of region).
     *
     * **Mask Format**:
     * - Input/output mask must be (width+2) × (height+2)
     * - Extra border prevents edge artifacts
     * - Non-zero mask pixels block filling (protection)
     * - Filled pixels are marked with miMaskColor (typically 255)
     */
    void processData( const std::shared_ptr<CVImageData> (&in)[2], std::shared_ptr< CVImageData > (&out)[2],
                      const CVFloodFillParameters & params, CVFloodFillProperties &props);

};

