/**
 * @file CVPointData.hpp
 * @brief Node data wrapper for OpenCV cv::Point.
 *
 * Encapsulates cv::Point (x, y coordinates) for dataflow connections,
 * with formatted information display "(X , Y)".
 *
 * **Key Features:**
 * - Wraps cv::Point with NodeData interface
 * - NodeDataType {"information", "Pnt"}
 * - Formatted info: "(320 , 240)"
 * - Integrates with OpenCV drawing and geometry
 *
 * **Typical Usage:**
 * @code
 * // Create point
 * cv::Point center(320, 240);
 * auto pointData = std::make_shared<CVPointData>(center);
 * 
 * // Output to port
 * output[0] = pointData;
 * 
 * // Display info
 * QString info = pointData->info();  // "(320 , 240)"
 * 
 * // Use in drawing
 * cv::Point pt = pointData->data();
 * cv::circle(image, pt, 5, cv::Scalar(0, 255, 0), -1);
 * @endcode
 *
 * **Common Scenarios:**
 * - Marking feature points or keypoints
 * - Defining anchor positions
 * - Passing click coordinates
 * - Setting reference origins
 *
 * @see cv::Point for OpenCV point structure
 * @see InformationData for base class
 * @see CVRectData for rectangle data
 */

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

#pragma once

#include <opencv2/core/core.hpp>

#include <QtNodes/NodeData>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class CVPointData
 * @brief Node data for OpenCV Point (x, y coordinates).
 *
 * Wraps cv::Point for dataflow connections, providing formatted display
 * and OpenCV geometry integration.
 *
 * **Data Format:**
 * - Type: {"information", "Pnt"}
 * - Info: "(X , Y)" (e.g., "(320 , 240)")
 *
 * **Usage Patterns:**
 * @code
 * // From OpenCV Point
 * cv::Point center(320, 240);
 * auto pointData = std::make_shared<CVPointData>(center);
 * output[0] = pointData;
 * 
 * // Extract Point
 * auto input = std::dynamic_pointer_cast<CVPointData>(nodeData);
 * if (input) {
 *     cv::Point pt = input->data();
 *     cv::circle(image, pt, 10, cv::Scalar(255, 0, 0), 2);
 * }
 * @endcode
 *
 * **Common Applications:**
 * @code
 * // 1. Mark detected feature
 * cv::Point keypoint = detector.detect(image);
 * auto pointData = std::make_shared<CVPointData>(keypoint);
 * 
 * // 2. Define anchor position
 * auto anchorInput = getInputData<CVPointData>(0);
 * cv::Point anchor = anchorInput->data();
 * cv::Rect roi(anchor, cv::Size(100, 100));
 * 
 * // 3. Draw marker
 * cv::Point pt = pointData->data();
 * cv::drawMarker(image, pt, cv::Scalar(0, 255, 0), cv::MARKER_CROSS, 20, 2);
 * @endcode
 *
 * @see cv::Point for OpenCV documentation
 * @see InformationData for base class
 */
class CVPointData : public InformationData
{
public:

    /**
     * @brief Default constructor - creates origin point.
     *
     * Initializes with cv::Point(0, 0).
     *
     * **Example:**
     * @code
     * auto pointData = std::make_shared<CVPointData>();
     * cv::Point pt = pointData->data();  // (0, 0)
     * @endcode
     */
    CVPointData()
        : mCVPoint()
    {}

    /**
     * @brief Constructs from OpenCV Point.
     *
     * @param point cv::Point to wrap
     *
     * **Example:**
     * @code
     * cv::Point center(320, 240);
     * auto pointData = std::make_shared<CVPointData>(center);
     * QString info = pointData->info();  // "(320 , 240)"
     * @endcode
     */
    CVPointData( const cv::Point & point )
        : mCVPoint( point )
    {}

    /**
     * @brief Returns the node data type identifier.
     *
     * @return NodeDataType with id="information", name="Pnt"
     *
     * **Example:**
     * @code
     * NodeDataType type = pointData->type();
     * // type.id == "information"
     * // type.name == "Pnt"
     * @endcode
     */
    NodeDataType
    type() const override
    {
        return { "information", "Pnt" };
    }

    /**
     * @brief Returns the wrapped cv::Point.
     *
     * @return cv::Point& Reference to stored point
     *
     * **Example:**
     * @code
     * auto pointData = getInputData<CVPointData>(0);
     * cv::Point pt = pointData->data();
     * 
     * cv::putText(image, "X", pt, cv::FONT_HERSHEY_SIMPLEX, 
     *             0.5, cv::Scalar(0, 255, 0), 1);
     * @endcode
     */
    cv::Point &
    data()
    {
        return mCVPoint;
    }

    /**
     * @brief Formats point information as "(X , Y)".
     *
     * Overrides base class to provide Point-specific formatting.
     *
     * **Output Format:**
     * @code
     * cv::Point(320, 240) → "(320 , 240)"
     * cv::Point(0, 0) → "(0 , 0)"
     * cv::Point(-10, 15) → "(-10 , 15)"
     * @endcode
     */
    void set_information() override
    {
        mQSData = QString("Data Type : cv::Point \n");
        mQSData += QString("(%1 , %2)\n").arg(mCVPoint.x).arg(mCVPoint.y);
    }

private:
    /**
     * @brief Stored OpenCV Point.
     */
    cv::Point mCVPoint;

};
