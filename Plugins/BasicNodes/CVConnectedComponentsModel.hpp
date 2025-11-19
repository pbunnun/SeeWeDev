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
 * @file CVConnectedComponentsModel.hpp
 * @brief Connected component labeling node for blob detection and analysis.
 *
 * This node performs connected component analysis (CCA) on binary images, identifying
 * and labeling distinct regions of connected pixels. It's a fundamental operation in
 * image segmentation, object detection, and feature extraction.
 *
 * The node uses OpenCV's connectedComponents function with configurable connectivity
 * (4 or 8 neighbors) and algorithm types (SAUF, BBDT, Grana). It outputs both a
 * labeled image and the total component count.
 *
 * **Key Applications**:
 * - Blob detection and counting
 * - Region labeling for analysis
 * - Pre-processing for shape analysis
 * - Object separation in segmented images
 * - Particle counting in microscopy
 *
 * @see cv::connectedComponents for labeling algorithm
 * @see cv::connectedComponentsWithStats for statistics extraction
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <opencv2/imgproc.hpp>
#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVConnectedComponentsParameters
 * @brief Configuration for connected component labeling.
 *
 * This structure controls the connectivity model, output format, algorithm selection,
 * and visualization options for connected component analysis.
 *
 * **Parameters**:
 *
 * - **miConnectivity**: Pixel connectivity model (default: 4)
 *   * **4-connected**: Pixels share edge (N, S, E, W neighbors)
 *     ```
 *        N
 *      W X E
 *        S
 *     ```
 *   * **8-connected**: Pixels share edge or corner (includes diagonals)
 *     ```
 *      NW N NE
 *      W  X  E
 *      SW S SE
 *     ```
 *   4-connected is more conservative (splits diagonal touches), 8-connected is
 *   more permissive (connects diagonal neighbors). Choose based on application:
 *   - Use 4 for text, characters, precise separation
 *   - Use 8 for natural objects, blobs, general shapes
 *
 * - **miImageType**: Output label image depth (default: CV_32S)
 *   * CV_32S (int32): Supports up to 2^31 - 1 components
 *   * CV_16U (uint16): Supports up to 65,535 components (memory efficient)
 *   Most applications use CV_32S unless memory is constrained and component
 *   count is guaranteed to be small.
 *
 * - **miAlgorithmType**: Labeling algorithm (default: CCL_DEFAULT)
 *   * **CCL_DEFAULT**: Automatic selection (usually BBDT for best balance)
 *   * **CCL_WU**: Wu's algorithm - optimized for simple images
 *   * **CCL_GRANA**: Grana's BBDT - balanced performance
 *   * **CCL_BOLELLI**: Bolelli's SAUF - fastest for complex images
 *   * **CCL_SAUF**: Same as CCL_BOLELLI (SAUF = Scan Array Union Find)
 *   * **CCL_BBDT**: Block-Based Decision Tree - good general performance
 *   
 *   For most cases, CCL_DEFAULT auto-selects optimally. Advanced users can
 *   benchmark specific algorithms for their image characteristics.
 *
 * - **mbVisualize**: Enable pseudo-coloring of components (default: false)
 *   * **false**: Output is raw label image (CV_32S with label values 0, 1, 2, ...)
 *   * **true**: Output is pseudo-colored visualization (CV_8UC3, each label gets unique color)
 *   
 *   Visualization is useful for debugging and presentations but loses precise
 *   label information. For further processing, keep false and use labels directly.
 *
 * **Design Rationale**:
 * - Default 4-connectivity provides more conservative, predictable segmentation
 * - CV_32S output ensures no label overflow for complex scenes
 * - CCL_DEFAULT leverages OpenCV's automatic optimization
 * - Visualization disabled by default to preserve label data for downstream nodes
 */
typedef struct CVConnectedComponentsParameters{
    int miConnectivity;   ///< Pixel connectivity: 4 (edge only) or 8 (edge + corner)
    int miImageType;      ///< Output depth: CV_32S (int32) or CV_16U (uint16)
    int miAlgorithmType;  ///< Labeling algorithm: CCL_DEFAULT, CCL_WU, CCL_GRANA, CCL_SAUF, CCL_BBDT
    bool mbVisualize;     ///< Enable pseudo-color visualization (true) or raw labels (false)
    CVConnectedComponentsParameters()
        : miConnectivity(4),
          miImageType(CV_32S),
          miAlgorithmType(cv::CCL_DEFAULT),
          mbVisualize(false)
    {
    }
} CVConnectedComponentsParameters;

/**
 * @class CVConnectedComponentsModel
 * @brief Identifies and labels connected regions in binary images.
 *
 * This segmentation node performs connected component analysis (CCA), a fundamental
 * operation that identifies groups of connected pixels sharing the same value. It
 * assigns unique labels to each connected region, enabling individual blob analysis,
 * counting, and feature extraction.
 *
 * **Algorithm Overview**:
 * Connected component labeling scans a binary image and assigns the same label to
 * all pixels belonging to the same connected region:
 * 1. Scan image pixel by pixel
 * 2. For each foreground pixel, check already-labeled neighbors
 * 3. Assign label based on connectivity rules (4 or 8-connected)
 * 4. Resolve equivalences when components merge
 * 5. Output labeled image with unique integer per component
 *
 * **Input Port**:
 * - Port 0: CVImageData - Binary image (8-bit, 0=background, non-zero=foreground)
 *
 * **Output Ports**:
 * - Port 0: CVImageData - Labeled image (CV_32S or CV_8UC3 if visualized)
 * - Port 1: IntegerData - Number of components (excluding background)
 *
 * **Typical Pipeline**:
 * ImageSource → Grayscale → Threshold → **CVConnectedComponents** → Analysis/Filtering
 *
 * **Common Use Cases**:
 * - **Blob Detection**: Identify and count distinct objects in binary images
 * - **Particle Counting**: Count cells, particles, or objects in microscopy
 * - **OCR Pre-processing**: Isolate individual characters for recognition
 * - **Defect Detection**: Identify and label defects in quality inspection
 * - **Object Separation**: Separate touching objects by analyzing component labels
 * - **Region Analysis**: Extract features (area, centroid, bounding box) per component
 *
 * **Label Image Format**:
 * - Background pixels: label = 0
 * - Component 1: label = 1
 * - Component 2: label = 2
 * - Component N: label = N
 * 
 * Total components = max(labels) = N (background not counted)
 *
 * **Connectivity Impact**:
 * Consider a diagonal pattern:
 * ```
 * # . #
 * . # .
 * # . #
 * ```
 * - **4-connected**: 5 separate components (diagonals don't connect)
 * - **8-connected**: 1 component (diagonals connect)
 *
 * **Visualization Mode**:
 * When mbVisualize=true, each label is mapped to a pseudo-color for visual inspection.
 * This is useful for debugging but destroys label information. For downstream processing
 * (statistics, filtering), keep visualization disabled and use raw labels.
 *
 * **Performance**:
 * - Processing time: ~2-5ms for 640x480 images
 * - Complexity: O(N) where N = number of pixels
 * - Modern algorithms (BBDT, SAUF) are highly optimized with SIMD
 *
 * **Design Decision**:
 * Default 4-connectivity is chosen for more precise, conservative segmentation.
 * 8-connectivity tends to merge components that are only diagonal-touching, which
 * may not be desirable for applications like character separation or precise object
 * detection. Users can switch to 8-connected for natural object analysis.
 *
 * @see cv::connectedComponents for basic labeling
 * @see cv::connectedComponentsWithStats for statistics (area, bbox, centroid)
 * @see FindContourModel for boundary-based object detection
 */
class CVConnectedComponentsModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVConnectedComponentsModel with 4-connectivity.
     */
    CVConnectedComponentsModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVConnectedComponentsModel() override {}

    /**
     * @brief Serializes model parameters to JSON.
     * @return QJsonObject containing connectivity, algorithm, and visualization settings
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
     * @return 1 for Input (binary image), 2 for Output (labels + count)
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
     * @param port Output port index (0=labels, 1=count)
     * @return Shared pointer to CVImageData (port 0) or IntegerData (port 1)
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and triggers connected component analysis.
     * @param nodeData Input binary image
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
     * @brief Updates model parameters from the property browser.
     * @param property Property name (e.g., "connectivity", "algorithm", "visualize")
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
    static const QString _model_name;  ///< Unique model name: "Connected Components"

private:
    CVConnectedComponentsParameters mParams;                      ///< CCA parameters (connectivity, algorithm, visualization)
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };   ///< Input binary image
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };     ///< Output labeled image
    std::shared_ptr<IntegerData> mpIntegerData { nullptr };     ///< Output component count
    QPixmap _minPixmap;                                         ///< Minimized node icon

    /**
     * @brief Processes data by performing connected component labeling.
     * @param in Input binary image (8-bit, non-zero pixels are foreground)
     * @param outImage Output labeled image (CV_32S) or pseudo-colored (CV_8UC3)
     * @param outInt Output component count (excluding background)
     * @param params CCA parameters (connectivity, algorithm, visualization)
     *
     * **Algorithm**:
     * ```cpp
     * // Perform connected component labeling
     * cv::Mat labels;
     * int numComponents = cv::connectedComponents(
     *     inputBinary,           // Input binary image
     *     labels,                // Output label image
     *     params.miConnectivity, // 4 or 8
     *     params.miImageType,    // CV_32S or CV_16U
     *     params.miAlgorithmType // CCL_DEFAULT, CCL_WU, etc.
     * );
     * 
     * // Optional visualization
     * if (params.mbVisualize) {
     *     cv::Mat colorLabels = cv::Mat::zeros(labels.size(), CV_8UC3);
     *     for (int label = 1; label < numComponents; label++) {
     *         cv::Vec3b color(rand() % 256, rand() % 256, rand() % 256);
     *         colorLabels.setTo(color, labels == label);
     *     }
     *     outImage = colorLabels;
     * } else {
     *     outImage = labels;
     * }
     * 
     * // Output count (background label 0 not included)
     * outInt = numComponents - 1;
     * ```
     *
     * **Return Values**:
     * - numComponents includes background (label 0), so subtract 1 for actual count
     * - Labels range from 0 (background) to numComponents-1
     *
     * **Post-Processing Options**:
     * After labeling, common next steps include:
     * - Filter by area: Remove small/large components
     * - Extract statistics: Use cv::connectedComponentsWithStats
     * - Extract contours: Use cv::findContours on individual labels
     * - Measure features: Compute area, perimeter, circularity per label
     */
    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & outImage,
                      std::shared_ptr<IntegerData> &outInt, const CVConnectedComponentsParameters & params );
};

