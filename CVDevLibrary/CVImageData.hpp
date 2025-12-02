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

#include <atomic>
#include <utility>
#include <opencv2/core/core.hpp>

#include <QtCore/QDateTime>
#include <QtNodes/NodeData>

#include "CVImagePool.hpp"
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using CVDevLibrary::CVImagePool;
using CVDevLibrary::FrameMetadata;

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
 * imageData->updateClone(frame, {});
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
     * imageData->updateClone(frame, {});
     * 
     * // Update information display
     * imageData->set_information();
     * @endcode
     *
     * @note Does not automatically call set_information()
     */
    void
    updateClone(const cv::Mat &image, FrameMetadata metadata)
    {
        image.copyTo(mCVImage);
        mPoolHandle = {};
        assignMetadata(metadata);
    }

    /**
     * @brief Move-set the wrapped image.
     *
     * Replaces the stored image by moving the provided rvalue `cv::Mat`.
     * This avoids a deep copy and is useful when the caller has a
     * temporary or otherwise relinquishes ownership.
     */
    void
    updateMove(cv::Mat &&image, FrameMetadata metadata) noexcept
    {
        mCVImage = std::move(image);
        mPoolHandle = {};
        assignMetadata(metadata);
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
        if (mPoolHandle)
            return mPoolHandle.matrix();
        return mCVImage;
    }

    const cv::Mat &
    data() const
    {
        if (mPoolHandle)
            return mPoolHandle.matrix();
        return mCVImage;
    }

    /**
     * @brief Legacy API: Updates the wrapped image (clone).
     *
     * Provided for backward compatibility with existing nodes.
     * New code should use `updateClone()` with explicit metadata.
     *
     * @param image New cv::Mat to store (will be cloned)
     */
    void
    set_image(const cv::Mat &image)
    {
        updateClone(image, {});
    }

    /**
     * @brief Legacy API: Updates the wrapped image (move).
     *
     * Provided for backward compatibility with existing nodes.
     * New code should use `updateMove()` with explicit metadata.
     *
     * @param image New cv::Mat to move (ownership transferred)
     */
    void
    set_image(cv::Mat &&image) noexcept
    {
        updateMove(std::move(image), {});
    }

    /**
     * @brief Checks if this image data owns a pooled frame.
     *
     * @return true if frame is backed by a pool slot, false if owned cv::Mat
     *
     * Useful for debugging or metrics collection. Consumer nodes don't
     * need to check this - just use `data()` which returns the correct reference.
     */
    bool
    hasPoolFrame() const
    {
        return static_cast<bool>(mPoolHandle);
    }

    /**
     * @brief Returns the metadata attached to this frame.
     *
     * @return FrameMetadata with timestamp, frameId, and producerId
     *
     * Used for tracing, debugging, and property browser display.
     * Automatically populated by producer nodes via `adoptPoolFrame()`
     * or `updateClone()`/`updateMove()` with metadata argument.
     */
    FrameMetadata const &
    metadata() const
    {
        return mMetadata;
    }

    /**
     * @brief Adopts a pooled frame handle (pool-aware producer path).
     *
     * @param handle FrameHandle from CVImagePool::acquire()
     * @return true if handle was valid and adopted, false if handle was empty
     *
     * **Migration Guide for Node Developers:**
     *
     * This method is the "opt-in helper" for transitioning nodes to zero-copy
     * pooling without breaking existing code. It wraps the pooling path so you
     * don't need to manually manage metadata or handle ownership.
     *
     * **Pattern 1: Producer Node (Pool-Aware)**
     * @code
     * // In process_decoded_frame() or similar:
     * FrameMetadata meta;
     * meta.producerId = getNodeId();
     * meta.frameId = currentFrameNumber;
     * 
     * auto handle = mpFramePool->acquire(1, std::move(meta));
     * if (mpCVImageData->adoptPoolFrame(std::move(handle))) {
     *     // Success: frame now owns pool slot
     * } else {
     *     // Fallback: pool exhausted, use legacy clone/move
     *     mpCVImageData->updateMove(std::move(frame), meta);
     * }
     * @endcode
     *
     * **Pattern 2: Consumer Node (Pool-Aware)**
     * @code
     * // In setInData():
     * auto imageData = std::dynamic_pointer_cast<CVImageData>(nodeData);
     * const cv::Mat& frame = imageData->data();
     * 
     * // Works transparently with both pooled and owned frames
     * frame.copyTo(localBuffer);
     * // Pool slot released when imageData destructs
     * @endcode
     *
     * **Pattern 3: Legacy Node (No Changes Needed)**
     * @code
     * // Existing code continues to work:
     * mpCVImageData->set_image(frame);          // Clone path
     * cv::Mat& img = mpCVImageData->data();     // Access path
     * @endcode
     *
     * **When to Migrate:**
     * - High-throughput producers (cameras, video loaders): migrate first for max benefit
     * - Display/recorder consumers: migrate to use const accessor for correctness
     * - Processing nodes: migrate when convenient (gradual migration is safe)
     * - Simple passthrough nodes: low priority (minimal performance gain)
     *
     * **Implementation Checklist:**
     * 1. Producer creates pool in `late_constructor()` or `ensure_frame_pool()`
     * 2. Producer calls `pool->acquire(consumerCount, metadata)` for new frames
     * 3. Producer writes data into `handle.matrix()` buffer
     * 4. Producer calls `adoptPoolFrame(std::move(handle))` with success check
     * 5. Consumer uses `const cv::Mat& frame = data()` for read access
     * 6. Consumer copies immediately if retaining beyond current scope
     *
     * @see CVVideoLoaderModel for complete producer example
     * @see CVImageDisplayModel for consumer example
     * @see CVImagePool for pool creation and management
     *
     * @note The boolean return allows clean fallback logic: if pool is exhausted,
     *       producer can immediately switch to `updateMove()` without leaving
     *       inconsistent state.
     */
    bool
    adoptPoolFrame(CVImagePool::FrameHandle handle)
    {
        if (!handle)
            return false;
        mPoolHandle = std::move(handle);
        assignMetadata(mPoolHandle.metadata());
        return true;
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
        const cv::Mat &frame = data();
        mQSData  = QString("Data Type\t : cv::Mat \n");
        if( !frame.empty() )
        {
            mQSData += "Channels\t : " + QString::number(frame.channels()) + "\n";
            mQSData += "Depth\t : ";
            auto depth = frame.depth();
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
            mQSData += "WxH\t : " + QString::number(frame.cols) + " x " + QString::number(frame.rows) + "\n";
        }
        if (!mMetadata.producerId.isEmpty())
        {
            mQSData += "Producer\t : " + mMetadata.producerId + "\n";
            mQSData += "Frame ID\t : " + QString::number(mMetadata.frameId) + "\n";
        }
    }

private:
    void assignMetadata(FrameMetadata metadata)
    {
        if (metadata.timestamp == 0)
            metadata.timestamp = QDateTime::currentMSecsSinceEpoch();
        if (metadata.frameId == 0)
            metadata.frameId = sFrameCounter.fetch_add(1, std::memory_order_relaxed);
        mMetadata = std::move(metadata);
        set_timestamp(mMetadata.timestamp);
        set_information();
    }

    static std::atomic<long> sFrameCounter;

    cv::Mat mCVImage;
    FrameMetadata mMetadata;
    CVImagePool::FrameHandle mPoolHandle;
};

inline std::atomic<long> CVImageData::sFrameCounter{1};
