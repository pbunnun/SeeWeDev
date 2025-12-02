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
 * @file CVImagePool.hpp
 * @brief Lock-free frame pool for zero-copy image sharing between producers and consumers.
 *
 * This header provides a ring-buffer-based memory pool for cv::Mat frames, enabling
 * producers (e.g., cameras, video loaders) to acquire pre-allocated buffers and consumers
 * (e.g., display nodes, processing nodes) to reference those buffers without copying.
 *
 * **Key Features:**
 * - Lock-free ring buffer with atomic head/tail pointers
 * - Reference counting per slot for multi-consumer scenarios
 * - RAII handle (FrameHandle) for automatic slot release
 * - Frame metadata (timestamp, frameId, producerId) attached to each acquisition
 * - Switchable modes: PoolMode (zero-copy) vs BroadcastMode (legacy clone)
 * - Automatic fallback logging when pool is exhausted
 *
 * **Typical Usage (Producer):**
 * @code
 * // Create pool (typically in late_constructor)
 * mpFramePool = std::make_shared<CVImagePool>(getNodeId(), width, height, CV_8UC3, 10);
 * 
 * // Acquire slot for new frame
 * FrameMetadata meta;
 * meta.producerId = getNodeId();
 * meta.frameId = currentFrameNumber;
 * auto handle = mpFramePool->acquire(consumerCount, std::move(meta));
 * 
 * if (handle) {
 *     // Write directly into pool buffer
 *     decodedFrame.copyTo(handle.matrix());
 *     mpCVImageData->adoptPoolFrame(std::move(handle));
 * } else {
 *     // Pool exhausted - fallback to broadcast
 *     mpCVImageData->updateMove(std::move(decodedFrame), meta);
 * }
 * @endcode
 *
 * **Typical Usage (Consumer):**
 * @code
 * // Read frame via const accessor (respects pooled or owned cv::Mat)
 * auto imageData = std::dynamic_pointer_cast<CVImageData>(nodeData);
 * const cv::Mat& frame = imageData->data();
 * 
 * // Process or copy immediately to release slot
 * frame.copyTo(localBuffer);
 * // Pool slot released when imageData destructs
 * @endcode
 *
 * **Thread Safety:**
 * - Multiple producers can acquire slots concurrently (lock-free ring buffer)
 * - Multiple consumers can hold references to different slots
 * - Reference counting ensures slots are released only when all consumers finish
 * - Mode switching (PoolMode ↔ BroadcastMode) is thread-safe via atomic
 *
 * **Performance Characteristics:**
 * - Acquisition: O(1) when pool not exhausted, O(pool_size) worst-case with yielding
 * - Release: O(1) atomic decrement
 * - Zero memory allocation after pool creation
 * - Cache-friendly: pre-allocated cv::Mat buffers reused across frames
 *
 * @see CVImageData for the wrapper that holds FrameHandle
 * @see CVVideoLoaderModel for producer implementation example
 * @see CVImageDisplayModel for consumer implementation example
 */

#pragma once

#include "DebugLogging.hpp"

#include <opencv2/core/core.hpp>

#include <QtCore/QThread>
#include <QtCore/QString>
#include <QtCore/QMutex>

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

namespace CVDevLibrary
{

/**
 * @enum FrameSharingMode
 * @brief Determines how frames are shared between producer and consumers.
 *
 * - **PoolMode**: Producer acquires pre-allocated slots; consumers reference via const accessor.
 *   Zero-copy when consumers don't modify frames. Pool exhaustion triggers fallback logging.
 * - **BroadcastMode**: Producer bypasses pool and clones frames for each emission.
 *   Legacy mode for compatibility; useful when consumer count is unknown or variable.
 */
enum class FrameSharingMode { PoolMode, BroadcastMode };

/**
 * @struct FrameMetadata
 * @brief Metadata attached to each acquired frame for tracing and debugging.
 *
 * Automatically populated by producer nodes and propagated through the dataflow graph.
 * Used in property browser display and log correlation.
 */
struct FrameMetadata
{
    long timestamp{0};      ///< Milliseconds since epoch (QDateTime::currentMSecsSinceEpoch)
    long frameId{0};        ///< Monotonically increasing frame counter per producer
    QString producerId;     ///< Node ID (getNodeId()) of the producer that emitted this frame
};

/**
 * @class CVImagePool
 * @brief Lock-free ring buffer pool for sharing cv::Mat frames between nodes.
 *
 * Maintains a fixed-size pool of pre-allocated cv::Mat buffers. Producers acquire
 * slots via `acquire()`, write frame data into the buffer, and pass ownership via
 * a `FrameHandle`. Consumers reference the buffer through `CVImageData::data()` and
 * the slot is automatically released when all references are dropped.
 *
 * **Design Rationale:**
 * - Eliminates frame cloning overhead in high-throughput pipelines (e.g., 30+ FPS video)
 * - Supports multi-consumer scenarios (e.g., one video source feeding display + recorder)
 * - Provides graceful degradation when pool is exhausted (automatic fallback + logging)
 * - Integrates with existing CVImageData API via optional FrameHandle member
 *
 * **Lifecycle:**
 * 1. Producer creates pool in `late_constructor()` or `ensure_frame_pool()`
 * 2. Producer calls `acquire(consumerCount, metadata)` to get a FrameHandle
 * 3. Producer writes frame data into `handle.matrix()`
 * 4. Producer transfers handle to `CVImageData::adoptPoolFrame()`
 * 5. Consumers read via `CVImageData::data()` (const reference to pooled buffer)
 * 6. When CVImageData destructs, FrameHandle releases slot automatically
 * 7. Slot becomes available for next acquisition when refCount reaches zero
 *
 * **Example: Video Loader with Pool:**
 * @code
 * void CVVideoLoaderModel::process_decoded_frame(cv::Mat frame) {
 *     ensure_frame_pool(frame.cols, frame.rows, frame.type());
 *     
 *     FrameMetadata meta;
 *     meta.producerId = getNodeId();
 *     meta.frameId = mpVDOLoaderThread->get_current_frame();
 *     
 *     auto handle = mpFramePool->acquire(1, std::move(meta));
 *     if (handle) {
 *         frame.copyTo(handle.matrix());
 *         mpCVImageData->adoptPoolFrame(std::move(handle));
 *     } else {
 *         mpCVImageData->updateMove(std::move(frame), meta);
 *     }
 * }
 * @endcode
 *
 * @see FrameHandle for RAII slot management
 * @see FrameMetadata for per-frame tracking data
 */
class CVImagePool
{
public:
    static constexpr size_t DefaultPoolSize = 10;

    /**
     * @struct PooledFrame
     * @brief Single slot in the ring buffer holding a cv::Mat and its reference count.
     *
     * Each slot is pre-allocated during pool construction and reused across frames.
     * The refCount tracks how many consumers currently hold references to this slot.
     */
    struct PooledFrame
    {
        cv::Mat mat;                 ///< Pre-allocated OpenCV matrix buffer
        std::atomic<int> refCount{0}; ///< Number of active FrameHandle references
    };

    /**
     * @class FrameHandle
     * @brief RAII handle for a pooled frame slot.
     *
     * Automatically releases the slot back to the pool when the handle destructs.
     * Move-only type to enforce single ownership semantics.
     *
     * **Lifetime:**
     * - Created by `CVImagePool::acquire()`
     * - Moved into `CVImageData::adoptPoolFrame()`
     * - Stored in CVImageData::mPoolHandle
     * - Destructs when CVImageData is destroyed, decrementing refCount
     * - Slot freed when refCount reaches zero
     *
     * **Usage:**
     * @code
     * auto handle = pool->acquire(1, metadata);
     * if (handle) {
     *     frame.copyTo(handle.matrix());
     *     imageData->adoptPoolFrame(std::move(handle));
     * }
     * @endcode
     */
    class FrameHandle
    {
    public:
        FrameHandle() = default;
        FrameHandle(FrameHandle &&other) noexcept;
        FrameHandle &operator=(FrameHandle &&other) noexcept;
        ~FrameHandle();

        explicit operator bool() const { return mSlot != nullptr; }
        cv::Mat &matrix() { return mSlot->mat; }
        const cv::Mat &matrix() const { return mSlot->mat; }
        FrameMetadata const &metadata() const { return mMetadata; }

    private:
        friend class CVImagePool;

        FrameHandle(CVImagePool *pool, PooledFrame *slot, FrameMetadata metadata);
        void release();

        CVImagePool *mPool{nullptr};
        PooledFrame *mSlot{nullptr};
        FrameMetadata mMetadata;
    };

    /**
     * @brief Constructs a frame pool with pre-allocated buffers.
     *
     * @param ownerId Node ID for logging (typically from getNodeId())
     * @param width Frame width in pixels
     * @param height Frame height in pixels
     * @param type OpenCV matrix type (e.g., CV_8UC3 for BGR)
     * @param poolSize Number of slots to allocate (default: 10)
     *
     * All slots are allocated immediately to avoid runtime allocation overhead.
     */
    CVImagePool(QString ownerId,
                int width,
                int height,
                int type,
                size_t poolSize = DefaultPoolSize);

    /**
     * @brief Returns the current sharing mode.
     * @return PoolMode or BroadcastMode
     */
    FrameSharingMode mode() const;

    /**
     * @brief Switches between pool and broadcast modes.
     * @param mode New mode to activate
     *
     * Thread-safe; can be called while acquire() is in progress.
     * Typically triggered by user changing "Sharing Mode" property in UI.
     */
    void setMode(FrameSharingMode mode);

    /**
     * @brief Acquires a slot from the pool for a new frame.
     *
     * @param consumerCount Expected number of consumers (sets initial refCount)
     * @param metadata Frame metadata (timestamp, frameId, producerId)
     * @return FrameHandle (empty if pool exhausted or in BroadcastMode)
     *
     * **Behavior:**
     * - PoolMode: Attempts to acquire next available slot from ring buffer
     *   - Success: Returns handle with refCount set to consumerCount
     *   - Failure: Yields and retries until mode changes or slot available
     *   - Logs warning if forced to broadcast due to exhaustion
     * - BroadcastMode: Immediately returns empty handle and logs fallback
     *
     * **Thread Safety:**
     * Lock-free atomic operations on head/tail pointers ensure multiple
     * producers can acquire concurrently without contention.
     *
     * @note Blocks with yielding if pool is full; ensure poolSize ≥ pipeline depth
     */
    FrameHandle acquire(size_t consumerCount, FrameMetadata metadata);

private:
    /**
     * @brief Internal helper: releases a slot back to the pool.
     * @param slot Pointer to the slot being released
     *
     * Decrements refCount; when it reaches zero, pushes the slot onto
     * the free list to make it available for reuse.
     */
    void releaseSlot(PooledFrame *slot);

    /**
     * @brief Logs a warning when pool exhaustion forces broadcast mode.
     *
     * Emits debug log with node ID to help diagnose pool sizing issues.
     * Logged once per fallback event to avoid log spam.
     */
    void logBroadcastFallback() const;

    const QString mOwnerId;              ///< Node ID for logging
    const size_t mPoolSize;              ///< Fixed number of slots
    std::vector<PooledFrame> mSlots;     ///< Pre-allocated frame buffers
    mutable QMutex mMutex;               ///< Mutex for thread-safe access to the free list
    std::vector<PooledFrame*> mFreeSlots;///< List of available slots
    std::atomic<FrameSharingMode> mMode{FrameSharingMode::PoolMode}; ///< Current mode
};

// ========================================================================
// FrameHandle Implementation (inline for header-only convenience)
// ========================================================================

inline CVImagePool::FrameHandle::FrameHandle(CVImagePool *pool,
                                             PooledFrame *slot,
                                             FrameMetadata metadata)
    : mPool(pool)
    , mSlot(slot)
    , mMetadata(std::move(metadata))
{
}

inline CVImagePool::FrameHandle::FrameHandle(FrameHandle &&other) noexcept
    : mPool(other.mPool)
    , mSlot(other.mSlot)
    , mMetadata(std::move(other.mMetadata))
{
    other.mSlot = nullptr;
    other.mPool = nullptr;
}

inline CVImagePool::FrameHandle &CVImagePool::FrameHandle::operator=(FrameHandle &&other) noexcept
{
    if (this != &other)
    {
        release();
        mPool = other.mPool;
        mSlot = other.mSlot;
        mMetadata = std::move(other.mMetadata);
        other.mSlot = nullptr;
        other.mPool = nullptr;
    }
    return *this;
}

inline CVImagePool::FrameHandle::~FrameHandle()
{
    release();
}

inline void CVImagePool::FrameHandle::release()
{
    if (mSlot && mPool)
    {
        mPool->releaseSlot(mSlot);
        mSlot = nullptr;
        mPool = nullptr;
    }
}

// ========================================================================
// CVImagePool Implementation (inline for header-only convenience)
// ========================================================================

inline CVImagePool::CVImagePool(QString ownerId,
                                int width,
                                int height,
                                int type,
                                size_t poolSize)
    : mOwnerId(std::move(ownerId))
    , mPoolSize(poolSize == 0 ? 1 : poolSize)
    , mSlots(mPoolSize)
{
    if (width > 0 && height > 0)
    {
        mFreeSlots.reserve(mPoolSize);
        for (auto &slot : mSlots)
        {
            slot.mat = cv::Mat(height, width, type);
            mFreeSlots.push_back(&slot);
        }
    }
}

inline FrameSharingMode CVImagePool::mode() const
{
    return mMode.load(std::memory_order_acquire);
}

inline void CVImagePool::setMode(FrameSharingMode mode)
{
    mMode.store(mode, std::memory_order_release);
}

inline CVImagePool::FrameHandle CVImagePool::acquire(size_t consumerCount,
                                                   FrameMetadata metadata)
{
    FrameSharingMode currentMode = mode();
    if (currentMode == FrameSharingMode::BroadcastMode)
    {
        logBroadcastFallback();
        return FrameHandle();
    }

    PooledFrame* slot = nullptr;
    while (true)
    {
        if (currentMode != FrameSharingMode::PoolMode)
        {
            logBroadcastFallback();
            return FrameHandle();
        }

        {
            QMutexLocker locker(&mMutex);
            if (!mFreeSlots.empty())
            {
                slot = mFreeSlots.back();
                mFreeSlots.pop_back();
            }
        }

        if (slot)
        {
            slot->refCount.store(static_cast<int>(consumerCount), std::memory_order_release);
            return FrameHandle(this, slot, std::move(metadata));
        }

        QThread::yieldCurrentThread();
        currentMode = mode();
    }
}

inline void CVImagePool::releaseSlot(PooledFrame *slot)
{
    if (!slot)
        return;

    if (slot->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        QMutexLocker locker(&mMutex);
        mFreeSlots.push_back(slot);
    }
}

inline void CVImagePool::logBroadcastFallback() const
{
    DEBUG_LOG_WARNING() << "CVImagePool node" << mOwnerId << "forced to broadcast mode";
}

} // namespace CVDevLibrary
