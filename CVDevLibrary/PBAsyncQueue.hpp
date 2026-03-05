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
/**
 * @file PBAsyncQueue.hpp
 * @brief A thread-safe bounded queue with timeout support and atomic shutdown.
 *
 * Features:
 * - Bounded capacity with configurable max size
 * - Timeout-aware enqueue/dequeue (graceful shutdown support)
 * - Atomic shutdown flag (thread-safe across all threads)
 * - Size/state queries without external locking
 * - Exception-safe move operations
 *
 * Usage:
 * @code
 *   PBAsyncQueue<MyData> queue(100);
 *
 *   // Producer: enqueue with 1s timeout
 *   if (queue.enqueue(data, 1000)) {
 *       DEBUG_LOG_INFO() << "Enqueued successfully";
 *   }
 *
 *   // Consumer: dequeue with 500ms timeout
 *   MyData item;
 *   if (queue.dequeue(item, 500)) {
 *       process(item);
 *   }
 *
 *   // Graceful shutdown
 *   queue.set_finished();  // Wakes all waiters
 * @endcode
 */

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <utility>
#include <optional>

#include "DebugLogging.hpp"

template<typename T>
class PBAsyncQueue {
public:
    /**
     * @brief Construct a bounded queue with specified maximum capacity.
     * @param max_size Maximum number of items the queue can hold
     * @throws std::invalid_argument if max_size is 0
     */
    explicit PBAsyncQueue(size_t max_size)
        : max_size_(max_size)
        , finished_(false)
    {
        if (max_size == 0) {
            throw std::invalid_argument("PBAsyncQueue: max_size must be > 0");
        }
        DEBUG_LOG_INFO() << "PBAsyncQueue initialized with max_size:"
                         << static_cast<qint64>(max_size_);
    }

    ~PBAsyncQueue() = default;

    /**
     * @brief Enqueue an item with optional timeout.
     *
     * @param item Item to enqueue (moved into queue)
     * @param timeout_ms Timeout in milliseconds (0 = infinite/blocking, default = infinite)
     * @return true if item was enqueued, false if queue is finished or timeout occurred
     *
     * Returns false immediately if queue is already finished.
     */
    bool enqueue(T item, int timeout_ms = 0)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // Check if finished before waiting
        if (finished_.load(std::memory_order_acquire)) {
            return false;
        }

        // Wait for space with timeout
        auto wait_result = timeout_ms <= 0
            ? cv_not_full_.wait_until(lock, std::chrono::system_clock::time_point::max(),
                                      [this]() {
                                          return !is_full_unsafe()
                                                 || finished_.load(std::memory_order_acquire);
                                      })
            : cv_not_full_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                    [this]() {
                                        return !is_full_unsafe()
                                               || finished_.load(std::memory_order_acquire);
                                    });

        if (!wait_result) {
            DEBUG_LOG_WARNING() << "Enqueue timeout after" << timeout_ms << "ms, queue size:"
                                << static_cast<qint64>(q_.size());
            return false;
        }

        // Check finished flag again after wait
        if (finished_.load(std::memory_order_acquire)) {
            return false;
        }

        // Queue must have space now
        if (is_full_unsafe()) {
            return false;
        }

        try {
            q_.push(std::move(item));
            DEBUG_LOG_INFO() << "Enqueued item, queue size:" << static_cast<qint64>(q_.size());
            cv_not_empty_.notify_one();
            return true;
        } catch (const std::exception& e) {
            DEBUG_LOG_WARNING() << "Enqueue failed with exception:" << e.what();
            return false;
        }
    }

    /**
     * @brief Dequeue an item with optional timeout.
     *
     * @param item Output parameter to receive dequeued item
     * @param timeout_ms Timeout in milliseconds (0 = infinite/blocking, default = infinite)
     * @return true if item was dequeued, false if queue is empty/finished or timeout occurred
     *
     * Returns false if queue is finished and empty.
     */
    bool dequeue(T& item, int timeout_ms = 0)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait for item with timeout
        auto wait_result = timeout_ms <= 0
            ? cv_not_empty_.wait_until(lock, std::chrono::system_clock::time_point::max(),
                                       [this]() {
                                           return !q_.empty()
                                                  || finished_.load(std::memory_order_acquire);
                                       })
            : cv_not_empty_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                     [this]() {
                                         return !q_.empty()
                                                || finished_.load(std::memory_order_acquire);
                                     });

        if (!wait_result || q_.empty()) {
            if (timeout_ms > 0 && q_.empty()) {
                DEBUG_LOG_WARNING() << "Dequeue timeout after" << timeout_ms
                                    << "ms, queue empty";
            }
            return false;
        }

        try {
            item = std::move(q_.front());
            q_.pop();
            DEBUG_LOG_INFO() << "Dequeued item, queue size:" << static_cast<qint64>(q_.size());
            cv_not_full_.notify_one();
            return true;
        } catch (const std::exception& e) {
            DEBUG_LOG_WARNING() << "Dequeue failed with exception:" << e.what();
            return false;
        }
    }

    /**
     * @brief Dequeue with std::optional (modern C++17 style).
     *
     * @param timeout_ms Timeout in milliseconds (0 = infinite)
     * @return std::optional<T> with item if successful, empty if finished/timeout/empty
     *
     * Preferred over dequeue(T&) for cleaner code.
     */
    std::optional<T> dequeue_optional(int timeout_ms = 0)
    {
        T item;
        if (dequeue(item, timeout_ms)) {
            return std::optional<T>(std::move(item));
        }
        return std::nullopt;
    }

    /**
     * @brief Mark the queue as finished (no more items will be enqueued).
     *
     * Wakes all threads waiting in enqueue() or dequeue() so they can gracefully exit.
     * After calling this, all pending enqueue() calls will return false immediately,
     * and dequeue() will return false once the queue is empty.
     *
     * Safe to call multiple times.
     */
    void set_finished()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            finished_.store(true, std::memory_order_release);
            DEBUG_LOG_INFO() << "Queue marked as finished, current size:"
                             << static_cast<qint64>(q_.size());
        }
        // Wake all waiters outside the lock to avoid deadlock
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }

    /**
     * @brief Check if the queue has been marked as finished.
     * @return true if set_finished() was called
     */
    [[nodiscard]] bool is_finished() const
    {
        return finished_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get current size of the queue.
     *
     * Note: Value is approximate and may change immediately after return
     * in multithreaded context.
     *
     * @return Number of items currently in queue
     */
    [[nodiscard]] size_t size() const
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return q_.size();
    }

    /**
     * @brief Check if queue is empty.
     *
     * Note: Result is approximate in multithreaded context.
     *
     * @return true if queue contains no items
     */
    [[nodiscard]] bool empty() const
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return q_.empty();
    }

    /**
     * @brief Check if queue is at maximum capacity.
     *
     * Note: Result is approximate in multithreaded context.
     *
     * @return true if queue is full
     */
    [[nodiscard]] bool full() const
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return is_full_unsafe();
    }

    /**
     * @brief Get maximum capacity of the queue.
     * @return Maximum size configured at construction
     */
    [[nodiscard]] size_t capacity() const
    {
        return max_size_;
    }

    /**
     * @brief Clear all items from the queue.
     *
     * Does NOT affect the finished flag.
     * Wakes one waiter in enqueue() if queue was full.
     */
    void clear()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        size_t cleared = q_.size();
        while (!q_.empty()) {
            q_.pop();
        }
        if (cleared > 0) {
            DEBUG_LOG_INFO() << "Cleared" << static_cast<qint64>(cleared)
                             << "items from queue";
            cv_not_full_.notify_all();
        }
    }

private:
    /**
     * @brief Internal check if queue is at maximum capacity (must hold mutex_).
     * @return true if q_.size() >= max_size_
     */
    [[nodiscard]] bool is_full_unsafe() const
    {
        return q_.size() >= max_size_;
    }

    std::queue<T> q_;
    const size_t max_size_;
    std::atomic<bool> finished_;                    // Atomic for thread-safe shutdown
    mutable std::mutex mutex_;                      // mutable for const query methods
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
};
