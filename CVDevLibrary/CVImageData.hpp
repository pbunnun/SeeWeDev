/**
 * @file CVImageData.hpp
 * @brief Node data wrapper for OpenCV cv::Mat (images).
 *
 * Encapsulates cv::Mat for image dataflow connections, with formatted
 * information display showing channels, depth, and dimensions.
 *
 * **Key Features:**
 * - Wraps cv::Mat with NodeData interface
 * - NodeDataType {"image", "Mat"}
 * - Formatted info: "3 CV_8U [480 px x 640 px]"
 * - Automatic depth detection (CV_8U through CV_64F)
 * - Zero-copy data access
 *
 * **Typical Usage:**
 * @code
 * // Create from OpenCV Mat
 * cv::Mat image = cv::imread("input.jpg");
 * auto imageData = std::make_shared<CVImageData>(image);
 * 
 * // Output to port
 * output[0] = imageData;
 * 
 * // Display info
 * QString info = imageData->info();  // "3 CV_8U [480 px x 640 px]"
 * 
 * // Extract Mat for processing
 * cv::Mat img = imageData->data();
 * cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
 * @endcode
 *
 * **Information Format Examples:**
 * @code
 * Grayscale 8-bit:   "1 CV_8U [480 px x 640 px]"
 * BGR color:         "3 CV_8U [480 px x 640 px]"
 * Float depth map:   "1 CV_32F [480 px x 640 px]"
 * Double matrix:     "1 CV_64F [100 px x 100 px]"
 * @endcode
 *
 * **Depth Constants:**
 * - CV_8U:  8-bit unsigned (0-255)
 * - CV_8S:  8-bit signed (-128 to 127)
 * - CV_16U: 16-bit unsigned
 * - CV_16S: 16-bit signed
 * - CV_32S: 32-bit signed integer
 * - CV_32F: 32-bit float
 * - CV_64F: 64-bit double
 *
 * @see cv::Mat for OpenCV matrix/image class
 * @see InformationData for base class
 * @see https://docs.opencv.org/master/d3/d63/classcv_1_1Mat.html
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
 * @class CVImageData
 * @brief Node data for OpenCV Mat (images and matrices).
 *
 * Wraps cv::Mat for dataflow connections, providing formatted display
 * with channel count, depth type, and dimensions.
 *
 * **Data Format:**
 * - Type: {"image", "Mat"}
 * - Info: "C DEPTH [H px x W px]" (e.g., "3 CV_8U [480 px x 640 px]")
 *
 * **Usage Patterns:**
 * @code
 * // From OpenCV Mat
 * cv::Mat image = cv::imread("photo.jpg");
 * auto imageData = std::make_shared<CVImageData>(image);
 * output[0] = imageData;
 * 
 * // Extract Mat
 * auto input = std::dynamic_pointer_cast<CVImageData>(nodeData);
 * if (input && !input->data().empty()) {
 *     cv::Mat img = input->data();
 *     processImage(img);
 * }
 * @endcode
 *
 * **Common Applications:**
 * @code
 * // 1. Pass processed image
 * cv::Mat processed;
 * cv::GaussianBlur(src, processed, cv::Size(5, 5), 0);
 * auto imageData = std::make_shared<CVImageData>(processed);
 * 
 * // 2. Update existing image data
 * auto imageData = std::make_shared<CVImageData>();
 * cv::Mat frame = camera.read();
 * imageData->set_image(frame);
 * 
 * // 3. Chain processing
 * auto inputData = getInputData<CVImageData>(0);
 * cv::Mat src = inputData->data();
 * cv::Mat dst;
 * cv::Canny(src, dst, 50, 150);
 * auto outputData = std::make_shared<CVImageData>(dst);
 * @endcode
 *
 * **Depth Detection:**
 * @code
 * CV_8U  → "CV_8U"   // 8-bit unsigned (images)
 * CV_8S  → "CV_8S"   // 8-bit signed
 * CV_16U → "CV_16U"  // 16-bit unsigned
 * CV_16S → "CV_16S"  // 16-bit signed
 * CV_32S → "CV_32S"  // 32-bit signed
 * CV_32F → "CV_32F"  // 32-bit float (depth maps)
 * CV_64F → "CV_64F"  // 64-bit double
 * @endcode
 *
 * @see cv::Mat for OpenCV documentation
 * @see InformationData for base class
 */
/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class CVImageData : public InformationData
{
public:

    /**
     * @brief Default constructor - creates empty image.
     *
     * Initializes with empty cv::Mat.
     *
     * **Example:**
     * @code
     * auto imageData = std::make_shared<CVImageData>();
     * if (imageData->data().empty()) {
     *     qDebug() << "No image yet";
     * }
     * @endcode
     */
    CVImageData()
        : mCVImage()
    {}

    /**
     * @brief Constructs from OpenCV Mat.
     *
     * Creates a deep copy (clone) of the input image.
     *
     * @param image cv::Mat to wrap (image or matrix)
     *
     * **Example:**
     * @code
     * cv::Mat image = cv::imread("photo.jpg");
     * auto imageData = std::make_shared<CVImageData>(image);
     * QString info = imageData->info();  // "3 CV_8U [480 px x 640 px]"
     * @endcode
     */
    CVImageData( const cv::Mat &image )
    {
        mCVImage = image.clone();
    }

    /**
     * @brief Move-constructor from cv::Mat rvalue.
     *
     * Accepts an rvalue `cv::Mat` and takes ownership of the underlying
     * buffer without cloning. Use this when the caller can relinquish
     * ownership of the source matrix to avoid an extra deep copy.
     */
    CVImageData( cv::Mat &&image ) noexcept
    {
        mCVImage = std::move(image);
    }

    /**
     * @brief Returns the node data type identifier.
     *
     * @return NodeDataType with id="image", name="Mat"
     *
     * **Example:**
     * @code
     * NodeDataType type = imageData->type();
     * // type.id == "image"
     * // type.name == "Mat"
     * @endcode
     */
    NodeDataType
    type() const override
    {
        //       id       name
        return { "image", "Mat" };
    }

    /**
     * @brief Updates the wrapped image.
     *
     * Replaces the stored cv::Mat with a deep copy of the new image.
     *
     * @param image New cv::Mat to store
     *
     * **Example:**
     * @code
     * auto imageData = std::make_shared<CVImageData>();
     * 
     * // Update with new frame
     * cv::Mat frame = camera.read();
     * imageData->set_image(frame);
     * 
     * // Update information display
     * imageData->set_information();
     * @endcode
     *
     * @note Does not automatically call set_information()
     */
    void
    set_image (const cv::Mat &image )
    {
        image.copyTo( mCVImage );
        //mCVImage = image.clone(); RUT
    }

    /**
     * @brief Move-set the wrapped image.
     *
     * Replaces the stored image by moving the provided rvalue `cv::Mat`.
     * This avoids a deep copy and is useful when the caller has a
     * temporary or otherwise relinquishes ownership.
     */
    void
    set_image (cv::Mat &&image ) noexcept
    {
        mCVImage = std::move(image);
    }

    /**
     * @brief Returns the wrapped cv::Mat.
     *
     * @return cv::Mat& Reference to stored image/matrix
     *
     * **Example:**
     * @code
     * auto imageData = getInputData<CVImageData>(0);
     * cv::Mat img = imageData->data();
     * 
     * if (!img.empty()) {
     *     cv::imshow("Image", img);
     * }
     * @endcode
     *
     * @note Returns reference - modifications affect stored image
     */
    cv::Mat &
    data()
    {
        return mCVImage;
    }

    /**
     * @brief Formats image information with channels, depth, and size.
     *
     * Overrides base class to provide Mat-specific formatting with
     * channel count, depth type detection, and dimensions.
     *
     * **Output Format Examples:**
     * @code
     * BGR image (640×480):
     *   "Data Type : cv::Mat
     *    Channels : 3
     *    Depth : CV_8U
     *    WxH : 640 x 480"
     * 
     * Grayscale (1920×1080):
     *   "Data Type : cv::Mat
     *    Channels : 1
     *    Depth : CV_8U
     *    WxH : 1920 x 1080"
     * 
     * Float depth map (640×480):
     *   "Data Type : cv::Mat
     *    Channels : 1
     *    Depth : CV_32F
     *    WxH : 640 x 480"
     * 
     * Empty Mat:
     *   "Data Type : cv::Mat"
     * @endcode
     *
     * **Depth Detection Logic:**
     * @code
     * int depth = image.depth();
     * switch (depth) {
     *     case CV_8U:  depthStr = "CV_8U"; break;
     *     case CV_8S:  depthStr = "CV_8S"; break;
     *     case CV_16U: depthStr = "CV_16U"; break;
     *     case CV_16S: depthStr = "CV_16S"; break;
     *     case CV_32S: depthStr = "CV_32S"; break;
     *     case CV_32F: depthStr = "CV_32F"; break;
     *     case CV_64F: depthStr = "CV_64F"; break;
     * }
     * @endcode
     */
    void set_information() override
    {
        mQSData  = QString("Data Type\t : cv::Mat \n");
        if( !mCVImage.empty() )
        {
            mQSData += "Channels\t : " + QString::number( mCVImage.channels() ) + "\n";
            mQSData += "Depth\t : ";
            auto depth = mCVImage.depth();
            if( depth == CV_8U )
                mQSData += "CV_8U \n";
            else if( depth == CV_8S )
                mQSData += "CV_8S \n";
            else if( depth == CV_16U )
                mQSData += "CV_16U \n";
            else if( depth == CV_16S )
                mQSData += "CV_16S \n";
            else if( depth == CV_32S )
                mQSData += "CV_32S \n";
            else if( depth == CV_32F )
                mQSData += "CV_32F \n";
            else if( depth == CV_64F )
                mQSData += "CV_64F \n";
            mQSData += "WxH\t : " + QString::number( mCVImage.cols ) + " x " + QString::number( mCVImage.rows ) + "\n";
        }
    }

private:

    /**
     * @brief Stored OpenCV Mat (image or matrix).
     */
    cv::Mat mCVImage;
};
