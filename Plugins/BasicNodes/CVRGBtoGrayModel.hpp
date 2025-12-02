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
 * @file CVRGBtoGrayModel.hpp
 * @brief Converts RGB/BGR color images to grayscale.
 *
 * This file defines a node that performs color-to-grayscale conversion using
 * OpenCV's cv::cvtColor() with standard luminance weighting. Commonly used
 * for preprocessing before edge detection, thresholding, or other operations
 * that work better on single-channel images.
 */

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBAsyncDataModel.hpp"

#include "CVImageData.hpp"
#include "CVImagePool.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;
using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;

/**
 * @class CVRGBtoGrayWorker
 * @brief Worker class for asynchronous RGB to grayscale conversion.
 */
class CVRGBtoGrayWorker : public QObject
{
    Q_OBJECT
public:
    CVRGBtoGrayWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                      FrameSharingMode mode,
                      std::shared_ptr<CVImagePool> pool,
                      long frameId,
                      QString producerId);

Q_SIGNALS:
    // CRITICAL: This signal MUST be declared in each worker class
    // CANNOT be inherited from base class due to Qt MOC limitation
    void frameReady(std::shared_ptr<CVImageData> img);
};

/**
 * @class CVRGBtoGrayModel
 * @brief Converts RGB/BGR color images to single-channel grayscale.
 *
 * ## Overview
 * This node performs standard color-to-grayscale conversion using OpenCV's
 * cv::cvtColor() function with the COLOR_BGR2GRAY conversion code. The
 * conversion uses the standard ITU-R BT.601 luminance formula.
 *
 * ## Conversion Formula
 * Grayscale value = 0.299×R + 0.587×G + 0.114×B
 *
 * This weighted sum reflects human perception where green contributes most
 * to perceived brightness, followed by red, then blue.
 *
 * ## Use Cases
 * 1. **Preprocessing**: Simplify images before edge detection or thresholding
 * 2. **Performance**: Reduce memory/computation by 3× (1 channel vs 3)
 * 3. **Algorithm Requirements**: Many CV algorithms require grayscale input
 * 4. **Feature Detection**: Harris corners, SIFT, ORB work on grayscale
 * 5. **Segmentation**: Simplify analysis by removing color information
 *
 * ## Processing Behavior
 * - Input: 3-channel BGR color image (CV_8UC3, CV_16UC3, CV_32FC3)
 * - Output: 1-channel grayscale image (CV_8UC1, CV_16UC1, CV_32FC1)
 * - Preserves spatial dimensions (width × height unchanged)
 * - Automatically handles different bit depths
 *
 * Example:
 * @code
 * // Input: BGR color image [100, 150, 200] (B, G, R)
 * // Conversion: 0.299×200 + 0.587×150 + 0.114×100
 * //           = 59.8 + 88.05 + 11.4 = 159.25 ≈ 159
 * // Output: Grayscale image [159]
 * @endcode
 *
 * ## Design Rationale
 * Uses cv::COLOR_BGR2GRAY instead of simple averaging because:
 * - Matches human visual perception
 * - Preserves perceived brightness/contrast
 * - Standard for most computer vision algorithms
 * - Compatible with scientific/technical imaging standards
 *
 * ## Performance
 * - Computational cost: O(width × height) - single-pass conversion
 * - Memory reduction: 67% smaller than color image
 * - Hardware optimization: Uses SIMD instructions when available
 *
 * ## Sync Data
 * Also outputs SyncData to enable synchronization with other nodes,
 * allowing grayscale conversion to trigger downstream processing.
 *
 * @note For custom weighting, use cv::transform() or MatrixOperationModel
 * @see cv::cvtColor, cv::COLOR_BGR2GRAY
 * @see CVImageOperationModel For other color space conversions
 */
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class CVRGBtoGrayModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVRGBtoGrayModel();

    ~CVRGBtoGrayModel() override = default;

    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Returns the node's icon for visual identification.
     * @return QPixmap containing the RGB to Gray conversion icon
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;     ///< Node category: "Image Conversion"

    static const QString _model_name;   ///< Node display name: "RGB to Gray"

protected:
    // Implement PBAsyncDataModel pure virtuals
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;


private:
    void process_cached_input() override;

    QPixmap _minPixmap;                             ///< Node icon

    // Pending data for backpressure
    cv::Mat mPendingFrame;
};
