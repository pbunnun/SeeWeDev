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
 * @file PBAsyncDataModel.hpp
 * @brief Base class for async worker + image pool pattern
 * 
 * This class provides common infrastructure for node models that use:
 * - QObject worker + moveToThread() pattern for async processing
 * - CVImagePool for zero-copy memory management
 * - Backpressure handling with pending frame queue
 * - Sync signal support for synchronized processing
 * - Configurable pool size and sharing mode
 * 
 * Derived classes must implement:
 * - createWorker() - Create worker instance
 * - connectWorker() - Connect worker signals to model slots
 * - dispatchWork() - Dispatch work to worker thread
 */

#pragma once

#include <atomic>
#include <QThread>
#include <QMutex>
#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "CVImagePool.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;

/**
 * @class PBAsyncDataModel
 * @brief Base class for async worker + pool pattern
 * 
 * Provides common infrastructure for:
 * - Worker thread lifecycle management
 * - CVImagePool management with configurable size/mode
 * - Backpressure handling (busy + pending)
 * - Sync signal support
 * - Pool/sharing mode properties
 * 
 * Derived classes override:
 * - createWorker() - instantiate worker object
 * - connectWorker() - connect worker signals
 * - dispatchWork() - invoke worker method with Q_ARG
 */
class CVDEVSHAREDLIB_EXPORT PBAsyncDataModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    explicit PBAsyncDataModel(const QString& modelName);
    ~PBAsyncDataModel() override;

    /**
     * @brief Initialize worker thread (call from late_constructor())
     */
    //void initializeWorkerThread();
    void late_constructor() override;

    /**
     * @brief Set model property (handles pool_size and sharing_mode)
     */
    void setModelProperty(QString& id, const QVariant& value) override;

    /**
     * @brief Save base class state (pool settings)
     */
    QJsonObject save() const override;

    /**
     * @brief Load base class state (pool settings)
     */
    void load(const QJsonObject &p) override;

    /**
     * @brief Number of ports for given port type
     * @param portType Input or Output
     */
    unsigned int nPorts(PortType portType) const override;
    
    /**
     * @brief Data type for given port
     * @param portType Input or Output
     * @param portIndex Port index
    */
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Provides the edge detection output
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the edge map CVImageData
     * @note Returns nullptr if no input has been processed
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input data
     * 
     * When image data arrives, this method:
     * 1. Converts input to grayscale if necessary
     * 2. Applies cv::Canny() with current parameters
     * 3. Stores the binary edge map for output
     * 4. Notifies connected nodes
     * 
     * @param nodeData The input data (CVImageData or SyncData)
     * @param portIndex The input port index (0 = image, 1 = sync)
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    // Virtual method to indicate whether the node is resizable.
    // Default is true so most nodes support dynamic resizing unless
    // a derived model explicitly returns false.
    bool resizable() const override { return false; }

protected:
    /**
     * @brief Create worker instance - MUST BE IMPLEMENTED BY DERIVED CLASS
     * @return Pointer to worker QObject (ownership transferred to thread)
     */
    virtual QObject* createWorker();

    /**
     * @brief Connect worker signals - MUST BE IMPLEMENTED BY DERIVED CLASS
     * @param worker The worker object returned by createWorker()
     */
    virtual void connectWorker(QObject* worker);

    /**
     * @brief Dispatch pending work - MUST BE IMPLEMENTED BY DERIVED CLASS
     * 
     * Called when worker becomes available and mHasPending is true.
     * Implementation should:
     * 1. Get pending data
     * 2. Set mHasPending = false
     * 3. Call ensure_frame_pool()
     * 4. Invoke worker method with QMetaObject::invokeMethod
     * 5. Set mWorkerBusy = true
     */
    virtual void dispatchPendingWork();

    /**
     * @brief Process cached input if available
     * 
     * Called when sync connection is created or deleted.
     * Derived class should implement to handle cached input.
     */
    virtual void process_cached_input();

    /**
     * @brief Ensure frame pool exists with correct dimensions
     * @param width Frame width in pixels
     * @param height Frame height in pixels
     * @param type OpenCV type (CV_8UC1, CV_8UC3, etc.)
     */
    void ensure_frame_pool(int width, int height, int type);

    /**
     * @brief Reset frame pool (destroys and recreates on next use)
     */
    void reset_frame_pool();

    /**
     * @brief Mark worker as busy
     */
    void setWorkerBusy(bool busy) { mWorkerBusy = busy; }

    /**
     * @brief Check if worker is busy
     */
    bool isWorkerBusy() const { return mWorkerBusy; }

    /**
     * @brief Mark pending work flag
     */
    void setPendingWork(bool pending) { mHasPending = pending; }

    /**
     * @brief Check if pending work exists
     */
    bool hasPendingWork() const { return mHasPending; }

    /**
     * @brief Check if shutting down
     */
    bool isShuttingDown() const { return mShuttingDown.load(std::memory_order_acquire); }

    /**
     * @brief Get frame counter and increment
     */
    long getNextFrameId() { return mFrameCounter++; }

    /**
     * @brief Get current pool (mutex-locked)
     */
    std::shared_ptr<CVImagePool> getFramePool();

    /**
     * @brief Get sharing mode
     */
    FrameSharingMode getSharingMode() const { return meSharingMode; }

    /**
     * @brief Handle work completion from worker
     * 
     * Derived class should call this from their handleFrameReady slot
     * after updating output data.
     */
    void onWorkCompleted();

    // Protected members accessible to derived classes
    QThread mWorkerThread;
    QObject* mpWorker { nullptr };
    bool mWorkerBusy { false };
    bool mHasPending { false };
    long mFrameCounter { 0 };
    std::atomic<bool> mShuttingDown { false };

    // Pool management
    int miPoolSize { 3 };
    FrameSharingMode meSharingMode { FrameSharingMode::PoolMode };
    std::shared_ptr<CVImagePool> mpFramePool;
    int miPoolFrameWidth { 0 };
    int miPoolFrameHeight { 0 };
    int miActivePoolSize { 0 };
    QMutex mFramePoolMutex;

    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    // Sync signal support
    std::shared_ptr<SyncData> mpSyncData { nullptr };
    bool mbUseSyncSignal { false };

public Q_SLOTS:
    // Common handler for worker frameReady across derived models
    virtual void handleFrameReady(std::shared_ptr<CVImageData> img);

protected Q_SLOTS:
    void inputConnectionCreated(QtNodes::ConnectionId const&) override;
    void inputConnectionDeleted(QtNodes::ConnectionId const&) override;
};
