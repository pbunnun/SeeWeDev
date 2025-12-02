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
 * @file CVAdditionModel.hpp
 * @brief Pixel-wise addition node for blending two images with an optional mask.
 *
 * This node wraps OpenCV's `cv::add` utility to perform saturated addition on
 * incoming image streams. Two image inputs are required and an optional third
 * input acts as an 8-bit mask that limits where addition occurs. The node emits
 * a single image output suitable for downstream visualization or processing.
 *
 * **Key Capabilities**
 * - Adds/gracefully blends two images of the same type and resolution
 * - Supports optional mask-driven compositing (third input)
 * - Maintains per-input readiness before triggering processing
 * - Provides a minimized pixmap icon for compact node rendering
 *
 * **Typical Use Cases**
 * - Exposure fusion (base + highlight images)
 * - Overlaying synthetic graphics on camera frames with a mask
 * - Accumulating incremental processing results (e.g., edge + texture maps)
 * - Fast prototyping for arithmetic image pipelines
 */

#pragma once

#include <iostream>
#include <atomic>

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
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
 * @class CVAdditionWorker
 * @brief Worker class for asynchronous image addition processing.
 */
class CVAdditionWorker : public QObject
{
    Q_OBJECT
public:
    CVAdditionWorker() {}

public Q_SLOTS:
    void processFrames(cv::Mat a,
                       cv::Mat b,
                       cv::Mat mask,
                       bool maskActive,
                       FrameSharingMode mode,
                       std::shared_ptr<CVImagePool> pool,
                       long frameId,
                       QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);
};

/**
 * @class CVAdditionModel
 * @brief Saturated addition node with optional mask input.
 *
 * The model accepts up to three inputs: Image A, Image B, and an optional
 * single-channel mask. When both required images are present (and the mask is
 * either inactive or available) the node performs a saturated addition using
 * OpenCV. The result is cached in `mpCVImageData` and propagated to connected
 * outputs.
 *
 * **Port Layout**
 * - In Port 0: Image A (`CVImageData`)
 * - In Port 1: Image B (`CVImageData`)
 * - In Port 2: Mask (`CVImageData`, treated as `CV_8UC1`, optional)
 * - Out Port 0: Result (`CVImageData`)
 *
 * Internal state keeps a vector of the latest mats per input plus a mask flag
 * so that reconnections or deletions immediately re-trigger processing when
 * enough data is present.
 */
class CVAdditionModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Initializes inputs, mask tracking, and the cached result.
     */
    CVAdditionModel();

    /**
     * @brief Destructor - ensures safe cleanup of async operations.
     */
    virtual ~CVAdditionModel() override;

    /**
     * @brief Serializes node configuration. There are no extra parameters to save.
     * @return QJsonObject containing serialized state (currently empty). 
     */
    QJsonObject 
    save() const override;

    /**
     * @brief Restores node state from a JSON object.
     * @param p Serialized JSON payload (currently unused).
     */
    void 
    load(QJsonObject const &p) override;

    /**
     * @brief Late constructor - initializes worker thread.
     */
    void
    late_constructor() override;

    /**
     * @brief Reports port counts for the given direction.
     * @param portType Input or Output
     * @return 2 input ports (images) or 1 output port (result).
     */
    unsigned int 
    nPorts(PortType portType) const override;

    /**
     * @brief Describes the data type that the node handles.
     * @param portType Input or Output
     * @param portIndex Port index
     * @return CVImageData for image ports, SyncData for sync port.
     */
    NodeDataType 
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the latest computed CVImageData when available.
     * @param port Output port index
     * @return Shared pointer to the output data or nullptr if not available.
     */
    std::shared_ptr<NodeData> 
    outData(PortIndex port) override;

    /**
     * @brief Stores incoming data and triggers addition when inputs are ready.
     * @param nodeData Incoming CVImageData for the specified port.
     * @param portIndex Port index (0, 1, or 2).
     */
    void 
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief No embedded widget for this node.
     * @return nullptr since there is no embedded widget.
     */
    QWidget * 
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Syncs property browser values with node behavior (no additional properties).
     * @param id Property identifier
     * @param value New property value
     */
    void 
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Provides the icon used when the node is minimized.
     * @return QPixmap for the minimized node representation.
     */
    QPixmap 
    minPixmap() const override { return _minPixmap; }

    static const QString _category;
    static const QString _model_name;

private Q_SLOTS:
    /**
     * @brief Tracks when a connection is made so mask availability can be toggled.
     * @return void
     */
    void 
    inputConnectionCreated(QtNodes::ConnectionId const&) override;

    /**
     * @brief Reacts to connection removal, clearing cached mats and reprocessing if needed.
     * @return void
     */
    void 
    inputConnectionDeleted(QtNodes::ConnectionId const&) override;

private Q_SLOTS:
    void handleFrameReady(std::shared_ptr<CVImageData> img);

private:
    /**
     * @brief Applies saturated addition to the cached inputs asynchronously.
     * @param in Vector containing Image A, Image B, and optional mask.
     * @param out Destination CVImageData that stores the resulting frame.
     *
     * Performs cv::add synchronously on the calling thread.
     */
    void processData(const std::vector< cv::Mat >&in, std::shared_ptr<CVImageData>& out);

    /**
     * @brief Ensures frame pool exists with correct dimensions.
     * @param width Frame width
     * @param height Frame height
     * @param type OpenCV Mat type
     */
    void ensure_frame_pool(int width, int height, int type);

    /**
     * @brief Resets the frame pool.
     */
    void reset_frame_pool();

    /**
     * @brief Processes cached input data (extracted to avoid duplication)
     */
    void process_cached_input();

    void start_worker_if_needed();
    void dispatch_pending();

    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<SyncData> mpSyncData { nullptr };
    std::vector<cv::Mat> mvCVImageInData;
    bool mbMaskActive { false };
    QPixmap _minPixmap;

    // Frame pool management
    int miPoolSize{CVImagePool::DefaultPoolSize};
    FrameSharingMode meSharingMode{FrameSharingMode::PoolMode};
    std::shared_ptr<CVImagePool> mpFramePool;
    int miPoolFrameWidth{0};
    int miPoolFrameHeight{0};
    int miActivePoolSize{0};
    QMutex mFramePoolMutex;
    bool mbUseSyncSignal{false};
    long mFrameCounter{0};

    // Asynchronous processing
    QThread mWorkerThread;
    CVAdditionWorker* mpWorker{nullptr};
    bool mWorkerBusy{false};
    bool mHasPending{false};
    std::vector<cv::Mat> mPendingFrames; // size 3 when used
    std::atomic<bool> mShuttingDown{false};
};
