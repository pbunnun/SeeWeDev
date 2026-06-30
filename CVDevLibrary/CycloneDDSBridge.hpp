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

#pragma once

/**
 * @file CycloneDDSBridge.hpp
 * @brief Singleton wrapper for CycloneDDS raw publish/subscribe integration.
 *
 * Provides a transport bridge to OMG DDS (implemented via Eclipse CycloneDDS)
 * for pub/sub operations on arbitrary byte-payload topics. Encapsulates all DDS
 * entity lifecycle management (participant, publisher, subscriber, reader/writer creation),
 * topic-scoped reader/writer pools, and callback dispatch via Qt polling.
 *
 * **Key Features:**
 * - Singleton instance per application
 * - Topic-based API (create readers/writers on-demand)
 * - Multi-subscriber support per topic (subscriber ID tracking)
 * - Thread-safe pub/sub operations (mutex-protected)
 * - Qt timer-driven polling for DDS reader data
 * - Graceful shutdown with entity cleanup
 * - Conditional compilation (CYCLONEDDS_ENABLED) for optional CycloneDDS support
 *
 * **Lifecycle:**
 * 1. Construct bridge (singleton, created once per app)
 * 2. Call initialize(computerId, domainId, partition) to create DDS entities
 * 3. Subscribe/publish as needed
 * 4. Call shutdown() to cleanup (or automatic on app exit)
 *
 * **DDS Configuration Parameters:**
 * - **computerId**: Unique computer/node identifier (appears in topic keys)
 * - **domainId**: DDS domain (0-232, default 0 for local/private networks)
 * - **partition**: Optional DDS partition for topic filtering
 *
 * **Thread Safety:**
 * - All public methods are thread-safe (use internal QMutex)
 * - Callbacks are dispatched from Qt event loop (safe for widget updates)
 */

#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QByteArray>
#include <QTimer>
#include <functional>
#include <memory>

/**
 * @class CycloneDDSBridge
 * @brief Singleton transport bridge to CycloneDDS (OMG Data Distribution Service).
 *
 * Manages DDS participant, publishers, and subscribers. Provides a simplified
 * topic-based API for publishing and subscribing to arbitrary byte payloads.
 *
 * **Usage Example:**
 * ```cpp
 * CycloneDDSBridge& bridge = CycloneDDSBridge::instance();
 * if (bridge.initialize("my_computer", 0)) {
 *     bridge.publishRaw("my/topic", data);
 *     bridge.subscribeRaw("my/topic", [](const QByteArray& payload) {
 *         // handle data
 *     });
 * }
 * ```
 *
 * **Architecture:**
 * - Pimpl (Private Implementation) pattern hides DDS entities in Impl struct
 * - All DDS entity handles stored/managed in Impl
 * - Callback dispatch table organized by topic and subscriber ID
 * - Qt polling loop drains reader queues every 10ms
 *
 * **Build Requirement:**
 * - Requires CycloneDDS library and CMake with CYCLONEDDS_ENABLED flag
 * - Falls back to Qt-local pub/sub if CYCLONEDDS_ENABLED is not defined
 */
class CycloneDDSBridge : public QObject
{
    Q_OBJECT

public:
    /// Callback signature for raw data delivery (QByteArray payload).
    using RawDataCallback = std::function<void(const QByteArray&)>;

    /// @name Singleton Access
    /// @{

    /**
     * @brief Returns the global CycloneDDS bridge instance.
     * @return Reference to singleton bridge (created on first call).
     */
    static CycloneDDSBridge& instance();

    /// @}

    /// @name Lifecycle Management
    /// @{

    /**
     * @brief Initializes DDS entities (participant, publishers, subscribers).
     *
     * Idempotent: calling initialize() again will shutdown and reinitialize.
     * After successful initialization, the bridge is ready for pub/sub operations.
     *
     * @param computerId Unique computer/node identifier (used in topic keys).
     * @param domainId DDS domain (0-232, default 0).
     * @param partition Optional DDS partition for topic filtering.
     * @return true if initialization succeeded, false on error (e.g., DDS unavailable).
     */
    bool initialize(const QString& computerId, int domainId = 0, const QString& partition = QString());

    /**
     * @brief Shuts down all DDS entities and clears subscriptions.
     *
     * Stops the polling timer, closes all active readers/writers, and destroys
     * the DDS participant. After shutdown, reinitialize() must be called before
     * pub/sub operations can resume.
     *
     * Safe to call even if not initialized.
     */
    void shutdown();

    /**
     * @brief Checks if bridge is fully initialized and ready for pub/sub.
     * @return true if initialize() succeeded and shutdown() has not been called.
     */
    bool isInitialized() const;

    /// @}

    /// @name Computer ID Management
    /// @{

    /**
     * @brief Sets the computer ID used in topic key construction.
     *
     * Can be called at any time. Updates affect newly created topic subscriptions.
     *
     * @param computerId Identifier string (trimmed for consistency).
     */
    void setComputerId(const QString& computerId);

    /**
     * @brief Returns the currently configured computer ID.
     * @return Computer ID string (defaults to QSysInfo::machineHostName()).
     */
    QString getComputerId() const;

    /// @}

    /// @name Publish Operations
    /// @{

    /**
     * @brief Publishes a raw byte payload to a DDS topic.
     *
     * Creates a DDS writer for the topic if not already created. Subsequent
     * calls to the same topic reuse the existing writer. Non-blocking operation.
     *
     * @param topicName DDS topic name (e.g., "cvdev/computer_id/topic/key").
     * @param payload Data bytes to publish.
     * @return true if publish succeeded or was queued, false on error.
     */
    bool publishRaw(const QString& topicName, const QByteArray& payload);

    /// @}

    /// @name Subscribe Operations
    /// @{

    /**
     * @brief Subscribes to a DDS topic without subscriber ID.
     *
     * Creates a DDS reader for the topic if not already created. Callback
     * is invoked from the Qt polling loop (safe for widget updates).
     *
     * @param topicName DDS topic name.
     * @param callback Invoked when data arrives (from Qt event loop).
     * @return true if subscription succeeded, false on error.
     */
    bool subscribeRaw(const QString& topicName, RawDataCallback callback);

    /**
     * @brief Subscribes to a DDS topic with a unique subscriber ID.
     *
     * Multiple subscribers can register for the same topic with different IDs.
     * Each receives callbacks independently. Useful for distinguishing message
     * sources or tracking subscription origin.
     *
     * @param topicName DDS topic name.
     * @param subscriberId Unique subscriber identifier (for this topic).
     * @param callback Invoked when data arrives.
     * @return true if subscription succeeded, false on error.
     */
    bool subscribeRaw(const QString& topicName, const QString& subscriberId, RawDataCallback callback);

    /// @}

    /// @name Unsubscribe Operations
    /// @{

    /**
     * @brief Unsubscribes all callbacks from a topic (generic subscription).
     *
     * Removes all callbacks registered without a subscriber ID for this topic.
     * Does not affect subscriber-ID-scoped subscriptions.
     *
     * @param topicName DDS topic name to unsubscribe from.
     */
    void unsubscribeRaw(const QString& topicName);

    /**
     * @brief Unsubscribes a specific subscriber ID from a topic.
     *
     * Removes callbacks registered with this subscriber ID. Other subscriber IDs
     * on the same topic remain active.
     *
     * @param topicName DDS topic name.
     * @param subscriberId Subscriber identifier (must match subscription call).
     */
    void unsubscribeRaw(const QString& topicName, const QString& subscriberId);

    /// @}

    /// @name Internal Data Delivery (Qt Polling)
    /// @{

    /**
     * @brief Drains one DDS reader and dispatches available messages.
     *
     * Called from Qt polling loop (10ms interval). Retrieves all samples from
     * the reader's queue and invokes registered callbacks on the Qt event loop.
     *
     * **Note**: This is called automatically by the polling timer. Manual calls
     * are only needed for custom polling strategies.
     *
     * @param readerEntity DDS reader handle (opaque integer).
     */
    void handleReaderData(int readerEntity);

    /// @}

private:
    /// Pimpl struct holding DDS entities and internal state.
    struct Impl;

    /**
     * @brief Constructs the bridge (private for singleton pattern).
     *
     * Initializes Qt polling timer and default computerId. Does NOT initialize
     * DDS entities (call initialize() after construction).
     */
    CycloneDDSBridge();

    /**
     * @brief Destroys the bridge and cleans up all resources.
     *
     * Calls shutdown() if not already called. Safe to destroy from any thread
     * (mutex protects cleanup).
     */
    ~CycloneDDSBridge() override;

    /// @name Private Member Variables
    /// @{

    /** Computer/node identifier for this bridge instance. */
    QString msComputerId;

    /** Flag indicating if DDS entities have been initialized. */
    bool mbInitialized{false};

    /** DDS domain ID (0-232). */
    int miDomainId{0};

    /** Optional DDS partition for topic filtering. */
    QString msPartition;

    /** Qt timer that drives the 10ms polling loop for reader data. */
    QTimer *mpPollTimer{nullptr};

    /** Pimpl holding DDS entity handles and callback dispatch table. */
    std::unique_ptr<Impl> mpImpl;

    /// @}
};
