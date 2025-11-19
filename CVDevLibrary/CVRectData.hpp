/**
 * @file CVRectData.hpp
 * @brief Node data wrapper for OpenCV cv::Rect.
 *
 * Encapsulates cv::Rect (x, y, width, height) for dataflow connections,
 * with formatted information display "[W px x H px] @ (X , Y)".
 *
 * **Key Features:**
 * - Wraps cv::Rect with NodeData interface
 * - NodeDataType {"information", "Rct"}
 * - Formatted info: "[640 px x 480 px] @ (100 , 50)"
 * - Integrates with OpenCV ROI operations
 *
 * **Typical Usage:**
 * @code
 * // Create ROI rectangle
 * cv::Rect roi(100, 50, 640, 480);
 * auto rectData = std::make_shared<CVRectData>(roi);
 * 
 * // Output to port
 * output[0] = rectData;
 * 
 * // Display info
 * QString info = rectData->info();  // "[640 px x 480 px] @ (100 , 50)"
 * 
 * // Extract ROI from image
 * cv::Rect rect = rectData->data();
 * cv::Mat roiImage = image(rect);
 * @endcode
 *
 * **Common Scenarios:**
 * - Defining image regions of interest (ROI)
 * - Passing bounding boxes between detection nodes
 * - Configuring cropping operations
 * - Tracking object locations
 *
 * @see cv::Rect for OpenCV rectangle structure
 * @see InformationData for base class
 * @see CVSizeData for size-only data
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
 * @class CVRectData
 * @brief Node data for OpenCV Rect (x, y, width, height).
 *
 * Wraps cv::Rect for dataflow connections, providing formatted display
 * and OpenCV ROI integration.
 *
 * **Data Format:**
 * - Type: {"information", "Rct"}
 * - Info: "[W px x H px] @ (X , Y)" (e.g., "[640 px x 480 px] @ (100 , 50)")
 *
 * **Usage Patterns:**
 * @code
 * // From OpenCV Rect
 * cv::Rect roi(100, 50, 640, 480);
 * auto rectData = std::make_shared<CVRectData>(roi);
 * output[0] = rectData;
 * 
 * // Extract ROI
 * auto input = std::dynamic_pointer_cast<CVRectData>(nodeData);
 * if (input) {
 *     cv::Rect rect = input->data();
 *     cv::Mat cropped = image(rect);
 * }
 * @endcode
 *
 * **Common Applications:**
 * @code
 * // 1. Define ROI from detection
 * cv::Rect bbox = detectObject(image);
 * auto rectData = std::make_shared<CVRectData>(bbox);
 * 
 * // 2. Crop image region
 * auto rectInput = getInputData<CVRectData>(0);
 * cv::Mat cropped = src(rectInput->data());
 * 
 * // 3. Draw bounding box
 * cv::Rect rect = rectData->data();
 * cv::rectangle(image, rect, cv::Scalar(0, 255, 0), 2);
 * @endcode
 *
 * @see cv::Rect for OpenCV documentation
 * @see InformationData for base class
 */
class CVRectData : public InformationData
{
public:

    /**
     * @brief Default constructor - creates zero rectangle.
     *
     * Initializes with cv::Rect(0, 0, 0, 0).
     *
     * **Example:**
     * @code
     * auto rectData = std::make_shared<CVRectData>();
     * cv::Rect r = rectData->data();  // (0, 0, 0, 0)
     * @endcode
     */
    CVRectData()
        : mCVRect()
    {}

    /**
     * @brief Constructs from OpenCV Rect.
     *
     * @param rect cv::Rect to wrap
     *
     * **Example:**
     * @code
     * cv::Rect roi(100, 50, 640, 480);
     * auto rectData = std::make_shared<CVRectData>(roi);
     * QString info = rectData->info();  // "[640 px x 480 px] @ (100 , 50)"
     * @endcode
     */
    CVRectData( const cv::Rect & rect )
        : mCVRect( rect )
    {}

    /**
     * @brief Returns the node data type identifier.
     *
     * @return NodeDataType with id="information", name="Rct"
     *
     * **Example:**
     * @code
     * NodeDataType type = rectData->type();
     * // type.id == "information"
     * // type.name == "Rct"
     * @endcode
     */
    NodeDataType
    type() const override
    {
        return { "information", "Rct" };
    }

    /**
     * @brief Returns the wrapped cv::Rect.
     *
     * @return cv::Rect& Reference to stored rectangle
     *
     * **Example:**
     * @code
     * auto rectData = getInputData<CVRectData>(0);
     * cv::Rect rect = rectData->data();
     * 
     * cv::Mat roi = image(rect);
     * cv::imshow("ROI", roi);
     * @endcode
     */
    cv::Rect &
    data()
    {
        return mCVRect;
    }

    /**
     * @brief Formats rectangle information as "[W px x H px] @ (X , Y)".
     *
     * Overrides base class to provide Rect-specific formatting.
     *
     * **Output Format:**
     * @code
     * cv::Rect(100, 50, 640, 480) → "[640 px x 480 px] @ (100 , 50)"
     * cv::Rect(0, 0, 1920, 1080) → "[1920 px x 1080 px] @ (0 , 0)"
     * @endcode
     */
    void set_information() override
    {
        mQSData = QString("Data Type : cv::Rect \n");
        mQSData += QString("[%1 px x %2 px] @ (%3 , %4)\n")
                  .arg(mCVRect.width).arg(mCVRect.height)
                  .arg(mCVRect.x).arg(mCVRect.y);
    }

private:
    /**
     * @brief Stored OpenCV Rect.
     */
    cv::Rect mCVRect;

};