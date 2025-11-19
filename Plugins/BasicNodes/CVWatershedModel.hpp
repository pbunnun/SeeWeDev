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
 * @file CVWatershedModel.hpp
 * @brief Node model for watershed segmentation algorithm.
 *
 * This file implements the watershed transform, a powerful region-based segmentation
 * technique that treats images as topographic surfaces where pixel intensity represents
 * elevation. The algorithm floods basins from markers to separate touching or overlapping
 * objects.
 *
 * **Algorithm Overview:**
 * The watershed transform segments an image into regions by:
 * 1. Treating grayscale image as topographic map
 * 2. Local minima = basin bottoms (object centers)
 * 3. Water rises from markers, flooding basins
 * 4. When waters meet, watershed lines are drawn (boundaries)
 *
 * **Key Applications:**
 * - Separating touching objects (coins, cells, particles)
 * - Region-based segmentation with prior knowledge
 * - Interactive segmentation (user-marked regions)
 * - Medical image analysis (organ/tissue boundaries)
 * - Material science (grain boundary detection)
 *
 * @see cv::watershed()
 * @see DistanceTransformModel (often used to generate markers)
 * @see MorphologyExModel (for pre-processing)
 */


#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVWatershedModel
 * @brief Node model implementing the watershed segmentation algorithm.
 *
 * This model applies the watershed transform to segment images into distinct regions
 * based on topological analysis. It requires two inputs: the source image and a marker
 * image that labels known regions. The algorithm separates touching objects and produces
 * labeled output.
 *
 * **Input Ports:**
 * 1. **CVImageData (port 0)** - Source image (grayscale or color)
 * 2. **CVImageData (port 1)** - Markers image (CV_32S, each region labeled with unique ID)
 *
 * **Output Ports:**
 * 1. **CVImageData** - Segmented result (CV_32S with region labels)
 * 2. **SyncData** - Synchronization signal
 *
 * **Marker Image Requirements:**
 * - Type: CV_32S (32-bit signed integer)
 * - Background pixels: 0
 * - Known regions: Positive integers (1, 2, 3, ...)
 * - Unknown regions: 0 (will be assigned labels)
 * - Boundaries will be marked: -1
 *
 * **Watershed Algorithm:**
 * The cv::watershed() function modifies markers in-place:
 * 1. Flood from each marker (local minimum)
 * 2. Expand regions until meeting boundaries
 * 3. Boundaries marked with -1
 * 4. All pixels assigned to regions or boundaries
 *
 * **Typical Workflow:**
 * @code
 * // Example 1: Separating coins
 * [Image] -> [Threshold] -> [DistanceTransform] -> [Threshold] -> [ConnectedComponents] -> [Watershed:Markers]
 * [Image] -> [Watershed:Image] -> [Segmented Coins]
 * 
 * // Example 2: Interactive segmentation
 * [Image] -> [UserMarkerTool] -> [Watershed:Markers]
 * [Image] -> [Watershed:Image] -> [Labeled Regions]
 * 
 * // Example 3: Cell segmentation
 * [MicroscopyImage] -> [MorphologyEx(Opening)] -> [DistanceTransform] -> [Threshold] -> [Markers]
 * [MicroscopyImage] -> [Gradient] -> [Watershed:Image]
 * [Markers] -> [Watershed:Markers] -> [Cell Boundaries]
 * @endcode
 *
 * **Creating Markers:**
 * 
 * **Method 1: Distance Transform**
 * @code
 * 1. Threshold binary image
 * 2. Distance transform
 * 3. Threshold distance (peaks = object centers)
 * 4. Connected components labeling
 * 5. Use as markers
 * @endcode
 *
 * **Method 2: Manual Markers**
 * @code
 * cv::Mat markers = cv::Mat::zeros(image.size(), CV_32S);
 * markers.at<int>(y1, x1) = 1;  // Object 1
 * markers.at<int>(y2, x2) = 2;  // Object 2
 * // Background remains 0
 * @endcode
 *
 * **Method 3: Morphological Opening**
 * @code
 * 1. Binary threshold
 * 2. Opening (removes noise)
 * 3. Sure foreground = erosion
 * 4. Sure background = dilation
 * 5. Unknown = background - foreground
 * 6. Connected components on foreground
 * @endcode
 *
 * **Image Input Types:**
 * - **Grayscale:** Direct gradient information
 * - **Color:** Converted internally or use gradient magnitude
 * - **Gradient Image:** Often gives better boundaries (recommended)
 *
 * **Output Interpretation:**
 * - Positive values (1, 2, 3, ...): Region labels
 * - -1: Watershed boundaries (ridges between regions)
 * - 0: Background (if any remains)
 *
 * **Visualization:**
 * @code
 * // Mark boundaries on original image
 * result[watershed_output == -1] = cv::Scalar(0, 0, 255);  // Red boundaries
 * 
 * // Color regions differently
 * for (int label = 1; label <= num_regions; ++label) {
 *     result[watershed_output == label] = colors[label];
 * }
 * @endcode
 *
 * **Common Issues:**
 *
 * **Over-segmentation:**
 * - Too many markers (every noise peak creates region)
 * - Solution: Better marker creation, morphological opening, distance threshold
 *
 * **Under-segmentation:**
 * - Too few markers (objects not separated)
 * - Solution: Lower distance threshold, better foreground detection
 *
 * **Incorrect Boundaries:**
 * - Wrong input image (should use gradient, not original)
 * - Solution: Apply Sobel/gradient before watershed
 *
 * **Performance Considerations:**
 * - Complexity: O(N log N) where N = pixels
 * - Marker count affects performance (more markers = faster)
 * - Large images (>2MP) may take seconds
 * - Consider downsampling for real-time applications
 *
 * **Mathematical Formulation:**
 * 
 * Watershed treats image I as topographic surface:
 * - Height at (x,y) = I(x,y)
 * - Markers M define starting points (local minima)
 * - Flooding simulation from each marker
 * - Catchment basins expand until meeting
 * - Watershed lines = ridge points separating basins
 *
 * **Advantages:**
 * - Handles touching/overlapping objects
 * - Closed contours (no gaps in boundaries)
 * - Works with any shape complexity
 * - Incorporates prior knowledge (markers)
 *
 * **Limitations:**
 * - Requires good markers (sensitive to marker quality)
 * - Over-segmentation with noisy images
 * - Computationally expensive for large images
 * - No built-in marker creation
 *
 * **Best Practices:**
 * 1. Pre-process image (denoise, morphological operations)
 * 2. Use gradient magnitude as input (not original image)
 * 3. Create robust markers (distance transform method)
 * 4. Post-process to merge small regions
 * 5. Visualize markers and result for debugging
 * 6. Adjust marker creation threshold iteratively
 *
 * **Example Parameter Tuning:**
 * @code
 * // Step 1: Create binary mask
 * threshold(gray, binary, 0, 255, THRESH_BINARY | THRESH_OTSU);
 * 
 * // Step 2: Remove noise
 * morphologyEx(binary, opening, MORPH_OPEN, kernel);
 * 
 * // Step 3: Sure background
 * dilate(opening, sure_bg, kernel);
 * 
 * // Step 4: Distance transform for foreground
 * distanceTransform(opening, dist_transform, DIST_L2, 5);
 * threshold(dist_transform, sure_fg, 0.5*max_dist, 255, 0);  // Adjust 0.5
 * 
 * // Step 5: Unknown region
 * sure_fg.convertTo(sure_fg, CV_8U);
 * subtract(sure_bg, sure_fg, unknown);
 * 
 * // Step 6: Markers
 * connectedComponents(sure_fg, markers);
 * markers += 1;  // Background = 1, so markers start at 2
 * markers[unknown == 255] = 0;  // Mark unknown
 * 
 * // Step 7: Watershed
 * watershed(image, markers);
 * @endcode
 *
 * **Related Nodes:**
 * - DistanceTransformModel: Generate markers from binary mask
 * - ThresholdModel: Create binary input
 * - MorphologyExModel: Pre-process (opening/closing)
 * - ConnectedComponentsModel: Label markers
 * - SobelAndScharrModel: Compute gradient for input
 *
 * @see cv::watershed()
 * @see cv::distanceTransform()
 * @see cv::connectedComponents()
 */
class CVWatershedModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVWatershedModel.
     *
     * Initializes with null input/output data.
     */
    CVWatershedModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVWatershedModel() override {}

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 2 for input (image + markers), 2 for output (result + sync).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return CVImageData for image ports, SyncData for sync.
     */
    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Returns output data for a specific port.
     * @param port Output port index (0=segmented image, 1=sync).
     * @return Shared pointer to output data.
     */
    std::shared_ptr< NodeData >
    outData( PortIndex port ) override;

    /**
     * @brief Sets input data and triggers watershed segmentation.
     * @param nodeData Input CVImageData (image or markers).
     * @param Port index (0=image, 1=markers).
     *
     * When both inputs are available, applies watershed algorithm.
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    /**
     * @brief Returns null (no embedded widget).
     * @return nullptr.
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Returns the minimized node pixmap.
     * @return Icon representing watershed operation.
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private:

    std::shared_ptr< CVImageData > mapCVImageInData[2]; ///< Input images (source, markers)
    std::shared_ptr< CVImageData > mpCVImageData;       ///< Output segmented image
    std::shared_ptr<SyncData> mpSyncData;               ///< Output synchronization signal

    QPixmap _minPixmap;                                 ///< Minimized node icon
    
    /**
     * @brief Processes watershed segmentation.
     * @param in Array of input images [0]=source, [1]=markers.
     * @param out Output segmented image (CV_32S with labels).
     *
     * Applies cv::watershed() to segment regions. Markers modified in-place.
     * Boundaries marked with -1, regions with positive integers.
     */
    void processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr< CVImageData > & out );
};

