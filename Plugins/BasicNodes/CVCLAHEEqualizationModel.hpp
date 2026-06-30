//Copyright © 2020 - 2026, NECTEC, all rights reserved

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
 * @file CVCLAHEEqualizationModel.hpp
 * @brief Contrast Limited Adaptive Histogram Equalization (CLAHE) image enhancement node.
 *
 * Implements asynchronous image enhancement using OpenCV's CLAHE algorithm for
 * histogram equalization with local contrast control. Supports single-channel and
 * multi-channel images with optional color-space conversion (YCrCb, Lab) to preserve
 * color while enhancing luminance.
 *
 * **Algorithm Overview:**
 * - CLAHE divides image into tiles and applies histogram equalization to each tile
 * - Clip limit prevents noise amplification in homogeneous regions
 * - Tile-based approach preserves edge details while avoiding over-enhancement
 * - Optional color space conversion separates luma from chroma channels
 *
 * **Features:**
 * - Single/multi-channel image support (auto-converts non-8U if enabled)
 * - Configurable clip limit (0.1-40.0) and tile size (2-64)
 * - Color space selection: YCrCb (default) or Lab
 * - Optional non-8U frame conversion with normalization
 * - Async processing with thread pool and CVImagePool integration
 * - Persistent property save/load via JSON
 *
 * **Node Type**: PBAsyncDataModel (worker thread with frame pool)
 * **Category**: Image Enhancement
 * **Display Name**: CV CLAHE Equalization
 */

#pragma once

#include <QtCore/QObject>

#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"
#include "CVImagePool.hpp"
#include "SyncData.hpp"

using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;

/**
 * @struct CVCLAHEEqualizationParameters
 * @brief Configuration parameters for CLAHE enhancement operation.
 *
 * Controls all aspects of histogram equalization and color handling.
 */
typedef struct CVCLAHEEqualizationParameters {
    /// Clip limit (0.1-40.0, default 2.0). Higher values increase contrast gain.
    double mdClipLimit { 2.0 };

    /// Tile grid size (2-64, default 8). Square tiles for uniform histogram equalization.
    int    miTileSize { 8 };

    /// Apply CLAHE only to luma channel (preserve color) in multi-channel images.
    bool   mbApplyColorLuma { true };

    /// Color space selection: 0=YCrCb (default), 1=Lab.
    int    miColorSpaceIndex { 0 };

    /// Auto-convert non-8U images to 8U via normalization before CLAHE (default: false).
    bool   mbConvertTo8Bit { false };
} CVCLAHEEqualizationParameters;

/**
 * @class CVCLAHEEqualizationWorker
 * @brief Worker thread for asynchronous CLAHE processing.
 *
 * Runs in a dedicated worker thread (QThread) managed by PBAsyncDataModel.
 * Processes frames on demand and emits results via frameReady signal.
 *
 * **Workflow:**
 * 1. Receives frame from main thread via processFrame slot
 * 2. Converts to 8-bit if needed (optional normalization)
 * 3. Applies CLAHE with configured parameters
 * 4. Handles color space conversion if enabled
 * 5. Returns result via frameReady signal (back to main thread)
 */

class CVCLAHEEqualizationWorker : public QObject
{
    Q_OBJECT

public:
    /// @brief Constructs the worker (empty initialization).
    CVCLAHEEqualizationWorker() {}

public Q_SLOTS:
    /// @name Frame Processing
    /// @{

    /**
     * @brief Processes a frame with CLAHE enhancement.
     *
     * Invoked from main thread via Qt::QueuedConnection. Performs:
     * - Non-8U conversion (if convertTo8Bit is true)
     * - CLAHE application with clip limit and tile size
     * - Optional color space conversion (YCrCb/Lab)
     * - Pool allocation and frame metadata tracking
     *
     * @param input Input image (may be empty, will emit nullptr on error).
     * @param clipLimit CLAHE clip limit (contrast control).
     * @param tileSize Tile grid size (square tiles).
     * @param applyColorLuma Apply CLAHE only to luma channel (multi-channel).
     * @param colorSpaceIndex Color space: 0=YCrCb, 1=Lab.
     * @param convertTo8Bit Auto-normalize non-8U to 8U before CLAHE.
     * @param mode Frame sharing mode (Pool/Broadcast).
     * @param pool CVImagePool for frame allocation (if mode=Pool).
     * @param frameId Frame identifier for metadata.
     * @param producerId Node ID of producer (for tracking).
     */
    void processFrame(cv::Mat input,
                      double clipLimit,
                      int tileSize,
                      bool applyColorLuma,
                      int colorSpaceIndex,
                      bool convertTo8Bit,
                      FrameSharingMode mode,
                      std::shared_ptr<CVImagePool> pool,
                      long frameId,
                      QString producerId);

    /// @}

Q_SIGNALS:
    /// @name Worker Results
    /// @{

    /**
     * @brief Emitted when frame processing completes.
     *
     * Delivered to PBAsyncDataModel::handleFrameReady on main thread
     * via Qt::QueuedConnection (safe for widget updates).
     *
     * @param img Result image (may be nullptr on error).
     */
    void frameReady(std::shared_ptr<CVImageData> img);

    /// @}
};

/**
 * @class CVCLAHEEqualizationModel
 * @brief Node model for Contrast Limited Adaptive Histogram Equalization.
 *
 * Provides asynchronous CLAHE image enhancement with user-configurable
 * parameters (clip limit, tile size, color space). Inherits async frame
 * processing infrastructure from PBAsyncDataModel.
 *
 * **Node Properties:**
 * - **Clip Limit** (double): 0.1-40.0 (default 2.0) - controls contrast gain
 * - **Tile Size** (int): 2-64 (default 8) - tile grid dimension
 * - **Apply On Color Luma** (bool): default true - preserve color in enhancement
 * - **Color Space** (enum): YCrCb (0, default) or Lab (1)
 * - **Convert Non-8U** (bool): default false - auto-normalize before CLAHE
 *
 * **Node Metadata:**
 * - Category: Image Enhancement
 * - Display Name: CV CLAHE Equalization
 * - Input Ports: 1 (cv::Mat image)
 * - Output Ports: 1 (cv::Mat enhanced image)
 * - Widget: None (property-based)
 *
 * **Async Architecture:**
 * - Worker thread processes frames asynchronously
 * - CVImagePool for zero-copy frame sharing (if enabled)
 * - Backpressure handling via pending frame queue
 * - Graceful shutdown with 3-second worker timeout
 *
 * **Save/Load Format:**
 * Stores parameters in JSON object \"cParams\":
 * ```json
 * {
 *   \"cParams\": {
 *     \"clipLimit\": 2.5,
 *     \"tileSize\": 8,
 *     \"applyColorLuma\": true,
 *     \"colorSpaceIndex\": 0,
 *     \"convertTo8Bit\": false
 *   }
 * }
 * ```
 */
class CVCLAHEEqualizationModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    /// @name Construction & Destruction
    /// @{

    /**
     * @brief Constructs the CLAHE node with default parameters.
     *
     * Initializes property vector and registers all configurable properties
     * (clip limit, tile size, color space, etc.) with default values.
     */
    CVCLAHEEqualizationModel();

    /**
     * @brief Destroys the node and cleans up resources.
     */
    ~CVCLAHEEqualizationModel() override = default;

    /// @}

    /// @name Persistence
    /// @{

    /**
     * @brief Saves node state to JSON for serialization.
     *
     * Captures all CLAHE parameters (clip limit, tile size, color space, etc.)
     * in JSON format for .flow file storage.
     *
     * @return JSON object with node state including \"cParams\" sub-object.
     */
    QJsonObject save() const override;

    /**
     * @brief Loads node state from JSON during deserialization.
     *
     * Restores all parameters and updates property system from saved JSON.
     *
     * @param p JSON object from .flow file (expects \"cParams\" sub-object).
     */
    void load(const QJsonObject &p) override;

    /// @}

    /// @name UI Integration
    /// @{

    /**
     * @brief Returns embedded widget (none for this node).
     * @return nullptr (property-based configuration only).
     */
    QWidget * embeddedWidget() override { return nullptr; }

    /**
     * @brief Updates a property value by ID.
     *
     * Called when user changes a property in the Property Browser.
     * Validates bounds (clip: 0.1-40.0, tile: 2-64) and updates parameters.
     *
     * @param id Property identifier (\"clip_limit\", \"tile_size\", etc.).
     * @param value New property value.
     */
    void setModelProperty(QString & id, const QVariant & value) override;

    /**
     * @brief Returns the minimized node icon/pixmap.
     * @return Node icon (from resources: CLAHEEqualization.png).
     */
    QPixmap minPixmap() const override { return _minPixmap; }

    /// @}

    /// @name Node Metadata
    /// @{

    /// Node category for UI organization (\"Image Enhancement\").
    static const QString _category;

    /// Display name in node menu (\"CV CLAHE Equalization\").
    static const QString _model_name;

    /// @}

protected:
    /// @name Worker Management (PBAsyncDataModel Implementation)
    /// @{

    /**
     * @brief Creates a new worker instance for frame processing.
     * @return New CVCLAHEEqualizationWorker instance.
     */
    QObject* createWorker() override;

    /**
     * @brief Connects worker signals to model slots.
     *
     * Establishes Qt::QueuedConnection from worker->frameReady
     * to this->handleFrameReady for thread-safe result delivery.
     *
     * @param worker Worker instance to connect.
     */
    void connectWorker(QObject* worker) override;

    /**
     * @brief Dispatches pending frame to worker for processing.
     *
     * Queues a frame processing job via QMetaObject::invokeMethod.
     * Called when worker becomes available and frame is pending.
     */
    void dispatchPendingWork() override;

    /// @}
private:
    /// @name Private Implementation
    /// @{

    /**
     * @brief Caches input frame for async processing.
     *
     * Called by setPortData when new image arrives.
     * Stores frame and parameters for dispatchPendingWork.
     */
    void process_cached_input() override;

    /// @}

    /// @name Member Variables
    /// @{

    /// Current CLAHE parameters (clip limit, tile size, color space, etc.).
    CVCLAHEEqualizationParameters mParams;

    /// Minimized node icon.
    QPixmap _minPixmap;

    /// Cached input frame pending async processing.
    cv::Mat mPendingFrame;

    /// Cached parameters for the pending frame.
    CVCLAHEEqualizationParameters mPendingParams;

    /// @}
};
