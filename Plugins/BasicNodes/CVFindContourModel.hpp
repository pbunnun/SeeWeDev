//Copyright © 2024, NECTEC, all rights reserved

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
 * @file CVFindContourModel.hpp
 * @brief Node model for extracting contours from binary images
 * 
 * This file defines a node that detects and extracts contours (boundaries) from
 * binary images. Contour detection is fundamental for shape analysis, object
 * recognition, and boundary-based segmentation in computer vision.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <opencv2/imgproc.hpp>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "ContourPointsData.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct ContourParameters
 * @brief Parameter structure for contour detection
 * 
 * Configures the retrieval mode and approximation method for cv::findContours.
 */
typedef struct ContourParameters{
    /** 
     * @brief Contour retrieval mode
     * @see cv::RetrievalModes
     * - RETR_EXTERNAL: Only outermost contours
     * - RETR_LIST: All contours, no hierarchy
     * - RETR_TREE: All contours with full hierarchy
     * - RETR_CCOMP: Two-level hierarchy
     */
    int miContourMode;
    
    /** 
     * @brief Contour approximation method
     * @see cv::ContourApproximationModes
     * - CHAIN_APPROX_NONE: Store all boundary points
     * - CHAIN_APPROX_SIMPLE: Compress horizontal/vertical/diagonal segments
     * - CHAIN_APPROX_TC89_L1: Teh-Chin approximation
     */
    int miContourMethod;
    
    /**
     * @brief Default constructor
     * 
     * Initializes with:
     * - Mode 1: Typically RETR_LIST (all contours, no hierarchy)
     * - Method 1: Typically CHAIN_APPROX_SIMPLE (compressed)
     */
    ContourParameters()
        : miContourMode(1),
          miContourMethod(1)
    {
    }
} ContourParameters;

/**
 * @class CVFindContourModel
 * @brief Node model for contour extraction from binary images
 * 
 * This model detects contours using OpenCV's cv::findContours() function.
 * Contours are curves joining continuous points along a boundary, representing
 * the shape of objects in binary images.
 * 
 * How contour detection works:
 * 1. Input must be binary (typically from thresholding or edge detection)
 * 2. Algorithm scans image to find boundary pixels
 * 3. Traces boundary to extract sequence of points
 * 4. Organizes contours based on retrieval mode
 * 5. Optionally simplifies contours based on approximation method
 * 
 * Retrieval mode selection:
 * - **RETR_EXTERNAL**: Only outermost boundaries (ignore holes)
 *   - Use: Count objects, measure outer perimeter
 *   - Fast, simple hierarchy
 * 
 * - **RETR_LIST**: All contours as flat list
 *   - Use: When hierarchy doesn't matter
 *   - Includes both objects and holes
 * 
 * - **RETR_TREE**: Full hierarchical structure
 *   - Use: Nested contours (objects with holes)
 *   - Preserves parent-child relationships
 * 
 * - **RETR_CCOMP**: Two-level hierarchy
 *   - Use: Distinguish objects from holes
 *   - Level 1 = outer, Level 2 = inner
 * 
 * Approximation method selection:
 * - **CHAIN_APPROX_NONE**: All boundary pixels
 *   - Use: Maximum precision needed
 *   - Large memory, slow processing
 * 
 * - **CHAIN_APPROX_SIMPLE**: Compress segments
 *   - Use: Most applications (recommended)
 *   - Fewer points, faster, usually sufficient
 * 
 * Common use cases:
 * - **Object detection**: Find objects in segmented images
 * - **Shape analysis**: Compute area, perimeter, moments
 * - **Character recognition**: Extract letter boundaries (OCR)
 * - **Defect detection**: Find irregularities in manufactured parts
 * - **Gesture recognition**: Track hand contours
 * - **Path planning**: Extract obstacle boundaries
 * 
 * Typical pipeline:
 * 1. Capture image
 * 2. Preprocess (blur, color conversion)
 * 3. Threshold or edge detection → binary image
 * 4. **FindContour** → extract boundaries
 * 5. Filter contours (by area, shape)
 * 6. Analyze or visualize (DrawContour)
 * 
 * Input:
 * - Port 0: CVImageData - Binary source image (black background, white objects)
 * - Port 1: SyncData - Optional synchronization signal
 * 
 * Output:
 * - Port 0: ContourPointsData - Detected contours as point sequences
 * 
 * Design Note: Input should be binary. Non-zero pixels are treated as foreground.
 * For best results, use THRESH_BINARY or Canny edge detection beforehand.
 * 
 * @note Input image is modified during processing (use clone if preservation needed)
 * @see cv::findContours for the underlying OpenCV operation
 * @see ContourPointsData for output data format
 * @see cv::RetrievalModes for retrieval mode options
 * @see cv::ContourApproximationModes for approximation methods
 */
class CVFindContourModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new contour detection node
     * 
     * Initializes with list retrieval mode and simple approximation.
     */
    CVFindContourModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVFindContourModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves contour retrieval mode and approximation method.
     * 
     * @return QJsonObject containing contour parameters
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved contour detection settings.
     * 
     * @param p QJsonObject containing the saved configuration
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports
     * 
     * - 2 input ports (binary image, optional sync)
     * - 1 output port (contours)
     * 
     * @param portType The type of port
     * @return Number of ports
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a port
     * 
     * Input 0: CVImageData (binary)
     * Input 1: SyncData
     * Output 0: ContourPointsData
     * 
     * @param portType The type of port
     * @param portIndex The port index
     * @return NodeDataType
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Provides the detected contours
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to ContourPointsData
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input
     * 
     * When binary image arrives, detects contours using cv::findContours().
     * 
     * @param nodeData Input data (CVImageData or SyncData)
     * @param portIndex Input port (0=image, 1=sync)
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    /**
     * @brief No embedded widget
     * 
     * @return nullptr - Parameters set via property browser
     */
    QWidget *
    embeddedWidget() override {return nullptr;}

    /**
     * @brief Sets properties from browser
     * 
     * Properties:
     * - "retrieval_mode": Contour hierarchy mode
     * - "approximation_method": Point compression method
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
     * @brief Performs contour detection
     * 
     * Executes cv::findContours() on the binary input:
     * 1. Validates input is binary or converts if needed
     * 2. Calls cv::findContours() with configured mode/method
     * 3. Stores detected contours in ContourPointsData
     * 
     * @param in Input binary image
     * @param outContour Output contour data
     * @param params Detection parameters (mode, method)
     * @see cv::findContours
     */
    void processData(const std::shared_ptr<CVImageData>& in, std::shared_ptr<ContourPointsData> &outContour, const ContourParameters& params);

    /** @brief Contour detection parameters */
    ContourParameters mParams;
    
    /** @brief Input image cache */
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    
    /** @brief Output contours cache */
    std::shared_ptr<ContourPointsData> mpContourPointsData { nullptr };
    
    /** @brief Synchronization signal */
    std::shared_ptr<SyncData> mpSyncData { nullptr };
};


