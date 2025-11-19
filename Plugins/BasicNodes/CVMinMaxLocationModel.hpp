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
 * @file CVMinMaxLocationModel.hpp
 * @brief Finds minimum and maximum pixel values and their locations in images.
 *
 * This file implements a node that identifies the minimum and maximum intensity values
 * in an image along with their pixel coordinates using OpenCV's cv::minMaxLoc function.
 * This is a fundamental image analysis operation useful for:
 *
 * - Finding brightest and darkest points
 * - Locating intensity extrema for thresholding decisions
 * - Identifying potential regions of interest
 * - Quality assessment (dynamic range measurement)
 * - Template matching score analysis
 * - Distance transform peak detection
 *
 * Functionality:
 * For a grayscale image I(x, y), finds:
 * - min_val = minimum pixel value in image
 * - max_val = maximum pixel value in image
 * - min_loc = (x, y) coordinates of minimum value
 * - max_loc = (x, y) coordinates of maximum value
 *
 * If multiple pixels share the same extremum value, only the first occurrence
 * (scanning left-to-right, top-to-bottom) is reported.
 *
 * Outputs:
 * - Port 0: min_loc (CVPointData) - location of minimum value
 * - Port 1: max_loc (CVPointData) - location of maximum value
 * - Port 2: min_val (DoubleData) - minimum pixel value
 * - Port 3: max_val (DoubleData) - maximum pixel value
 *
 * Common Use Cases:
 * - Find hottest spot in thermal image (max location)
 * - Locate darkest pixel for exposure verification (min value)
 * - Dynamic range calculation (max - min)
 * - Peak detection in correlation/matching results
 * - Seed point selection for region growing
 *
 * @see CVMinMaxLocationModel, cv::minMaxLoc, CVPointData, DoubleData
 */

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "CVPointData.hpp"
#include "DoubleData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVMinMaxLocationModel
 * @brief Node for finding minimum/maximum pixel values and their locations.
 *
 * This model wraps cv::minMaxLoc to extract intensity extrema and their spatial positions,
 * providing four outputs for comprehensive min/max analysis.
 *
 * Core Operation:
 * ```cpp
 * double min_val, max_val;
 * cv::Point min_loc, max_loc;
 * cv::minMaxLoc(input, &min_val, &max_val, &min_loc, &max_loc);
 * ```
 *
 * Output Ports:
 * 0. min_loc (CVPointData): Pixel coordinates of minimum value
 * 1. max_loc (CVPointData): Pixel coordinates of maximum value
 * 2. min_val (DoubleData): Minimum pixel intensity
 * 3. max_val (DoubleData): Maximum pixel intensity
 *
 * Common Use Cases:
 *
 * 1. Template Matching Peak:
 *    ```
 *    MatchTemplate → MinMaxLocation → max_loc → DrawMarker
 *    Identifies best match location
 *    ```
 *
 * 2. Dynamic Range:
 *    ```
 *    Image → MinMaxLocation → Subtract(max - min) → "Range: X"
 *    ```
 *
 * 3. Thermal Hotspot:
 *    ```
 *    ThermalImage → MinMaxLocation → max_loc → AnnotateImage
 *    ```
 *
 * 4. Distance Transform Peak:
 *    ```
 *    DistanceTransform → MinMaxLocation → max_loc → Skeleton
 *    ```
 *
 * Performance: O(W×H), single-pass scan
 *
 * @see cv::minMaxLoc, CVPointData, DoubleData
 */
class CVMinMaxLocationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVMinMaxLocationModel();

    virtual
    ~CVMinMaxLocationModel() override {}

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:

    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };       ///< Input image
    std::shared_ptr<CVPointData> mapCVPointData[2] {{nullptr}};     ///< Output locations [min, max]
    std::shared_ptr<DoubleData> mapDoubleData[2] {{nullptr}};       ///< Output values [min, max]
    QPixmap _minPixmap;

    /**
     * @brief Executes cv::minMaxLoc and packages results.
     *
     * ```cpp
     * double minVal, maxVal;
     * cv::Point minLoc, maxLoc;
     * cv::minMaxLoc(in->data(), &minVal, &maxVal, &minLoc, &maxLoc);
     *
     * outPoint[0] = std::make_shared<CVPointData>(minLoc);
     * outPoint[1] = std::make_shared<CVPointData>(maxLoc);
     * outDouble[0] = std::make_shared<DoubleData>(minVal);
     * outDouble[1] = std::make_shared<DoubleData>(maxVal);
     * ```
     *
     * @param in Input grayscale image
     * @param outPoint Output array [min_loc, max_loc]
     * @param outDouble Output array [min_val, max_val]
     */
    void processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVPointData> (&outPoint)[2], std::shared_ptr<DoubleData>(&outDouble)[2]);

};

