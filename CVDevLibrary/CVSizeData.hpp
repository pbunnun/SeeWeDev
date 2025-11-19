/**
 * @file CVSizeData.hpp
 * @brief Node data wrapper for OpenCV cv::Size.
 *
 * Encapsulates cv::Size (width × height dimensions) for dataflow connections,
 * with formatted information display "[H px x W px]".
 *
 * **Key Features:**
 * - Wraps cv::Size with NodeData interface
 * - NodeDataType {"Size", "Sze"}
 * - Formatted info: "[480 px x 640 px]"
 * - Integrates with OpenCV ROI operations
 *
 * **Typical Usage:**
 * @code
 * // Create from OpenCV Size
 * cv::Size imgSize(640, 480);
 * auto sizeData = std::make_shared<CVSizeData>(imgSize);
 * 
 * // Output to port
 * output[0] = sizeData;
 * 
 * // Display info
 * QString info = sizeData->info();  // "[480 px x 640 px]"
 * 
 * // Extract Size
 * cv::Size size = sizeData->data();
 * cv::Mat roi(image, cv::Rect(0, 0, size.width, size.height));
 * @endcode
 *
 * **Common Scenarios:**
 * - Passing image dimensions between nodes
 * - Configuring ROI sizes
 * - Defining output canvas dimensions
 * - Validating resize operations
 *
 * @see cv::Size for OpenCV size structure
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
 * @class CVSizeData
 * @brief Node data for OpenCV Size (width × height).
 *
 * Wraps cv::Size for dataflow connections, providing formatted display
 * and OpenCV integration.
 *
 * **Data Format:**
 * - Type: {"Size", "Sze"}
 * - Info: "[H px x W px]" (e.g., "[480 px x 640 px]")
 *
 * **Usage Patterns:**
 * @code
 * // From OpenCV Size
 * cv::Size imgSize(1920, 1080);
 * auto sizeData = std::make_shared<CVSizeData>(imgSize);
 * output[0] = sizeData;
 * 
 * // Extract Size
 * auto input = std::dynamic_pointer_cast<CVSizeData>(nodeData);
 * if (input) {
 *     cv::Size size = input->data();
 *     cv::Mat resized;
 *     cv::resize(src, resized, size);
 * }
 * @endcode
 *
 * **Common Applications:**
 * @code
 * // 1. Output image dimensions
 * cv::Mat image = ...;
 * auto sizeData = std::make_shared<CVSizeData>(image.size());
 * 
 * // 2. Define ROI size
 * auto sizeInput = getInputData<CVSizeData>(0);
 * cv::Rect roi(origin, sizeInput->data());
 * 
 * // 3. Validate resize target
 * cv::Size target = sizeData->data();
 * if (target.width > 0 && target.height > 0) {
 *     cv::resize(src, dst, target);
 * }
 * @endcode
 *
 * @see cv::Size for OpenCV documentation
 * @see InformationData for base class
 */
class CVSizeData : public InformationData
{
public:

    /**
     * @brief Default constructor - creates zero size.
     *
     * Initializes with cv::Size(0, 0).
     *
     * **Example:**
     * @code
     * auto sizeData = std::make_shared<CVSizeData>();
     * cv::Size s = sizeData->data();  // (0, 0)
     * @endcode
     */
    CVSizeData()
        : mCVSize()
    {}

    /**
     * @brief Constructs from OpenCV Size.
     *
     * @param size cv::Size to wrap
     *
     * **Example:**
     * @code
     * cv::Size imgSize(640, 480);
     * auto sizeData = std::make_shared<CVSizeData>(imgSize);
     * QString info = sizeData->info();  // "[480 px x 640 px]"
     * @endcode
     */
    CVSizeData( const cv::Size & size )
        : mCVSize( size )
    {}

    /**
     * @brief Returns the node data type identifier.
     *
     * @return NodeDataType with id="Size", name="Sze"
     *
     * **Example:**
     * @code
     * NodeDataType type = sizeData->type();
     * // type.id == "Size"
     * // type.name == "Sze"
     * @endcode
     */
    NodeDataType
    type() const override
    {
        return { "Size", "Sze" };
    }

    /**
     * @brief Returns the wrapped cv::Size.
     *
     * @return cv::Size& Reference to stored size
     *
     * **Example:**
     * @code
     * auto sizeData = getInputData<CVSizeData>(0);
     * cv::Size size = sizeData->data();
     * 
     * cv::Mat canvas(size, CV_8UC3, cv::Scalar(255, 255, 255));
     * @endcode
     */
    cv::Size &
    data()
    {
        return mCVSize;
    }

    /**
     * @brief Formats size information as "[H px x W px]".
     *
     * Overrides base class to provide Size-specific formatting.
     *
     * **Output Format:**
     * @code
     * cv::Size(640, 480) → "[480 px x 640 px]"
     * cv::Size(1920, 1080) → "[1080 px x 1920 px]"
     * cv::Size(0, 0) → "[0 px x 0 px]"
     * @endcode
     *
     * @note Height comes before width in display
     */
    void set_information() override
    {
        mQSData = QString("Data Type : cv::Size \n");
        mQSData += QString("[%1 px x %2 px]\n").arg(mCVSize.height).arg(mCVSize.width);
    }

private:
    /**
     * @brief Stored OpenCV Size.
     */
    cv::Size mCVSize;

};
