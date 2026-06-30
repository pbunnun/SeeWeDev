//Copyright © 2025 - 2026, NECTEC, all rights reserved

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
 * @file ZenohBridge.hpp
 * @brief Zenoh pub/sub bridge for distributed CVDev dataflow.
 *
 * This file provides the ZenohBridge class for integrating Zenoh pub/sub
 * messaging with CVDev's Qt-based node dataflow system.
 *
 * **Key Features:**
 * - **Global Transport Mode:** Application-wide switch via TransportMode
 *   (QtOnly or ZenohOnly)
 * - **Qt Fallback Safety:** In ZenohOnly mode, output propagation falls back
 *   to Qt signals if Zenoh is unavailable or not initialized
 * - **Session Management:** Shared Zenoh session across all nodes
 * - **Key Naming:** Hierarchical keys:
 *   - Output port: cvdev/{computer_id}/{daemon_id}/{node_id}/output/{port_idx}/data (with scoping)
 *   - Input port:  cvdev/{computer_id}/{daemon_id}/{node_id}/input/{port_idx}/data (with scoping)
 * - **Custom Channel Support:** Raw pub/sub for bridge nodes (e.g. "camera_feed", "lidar_scan")
 * - **Serialization:** Automatic binary serialization via NodeDataSerializer
 *
 * **Architecture:**
 * @code
 * ┌─────────────────────────────────────────────────────────────┐
 * │                         CVDev Application                    │
 * ├─────────────────────────────────────────────────────────────┤
 * │  Node A (QtOnly mode)  →  Qt Signal  →  Node B               │
 * │                                                              │
 * │  Node C (ZenohOnly mode)  →  Zenoh Pub/Sub  →  Node D        │
 * │                                                              │
 * │  Bridge nodes can use custom keys (cvdev/channel/{name})     │
 * └─────────────────────────────────────────────────────────────┘
 *                              ↓
 *                      ZenohBridge (singleton)
 *                              ↓
 *                   Zenoh Router (local or remote)
 * @endcode
 *
 * **Usage Pattern (Current):**
 * @code
 * // Initialization in MainWindow
 * ZenohBridge::instance().setComputerId("my_robot");  // or reads from settings
 * ZenohBridge::instance().initialize("my_robot");
 * TransportModeManager::instance().setTransportMode(TransportMode::QtOnly); // or ZenohOnly
 * 
 * // Nodes propagate outputs via PBNodeDelegateModel::emitOutputPort(portIdx):
 * // - QtOnly: emits dataUpdated(portIdx)
 * // - ZenohOnly: publishes via ZenohBridge::publish(nodeId, portIdx, data)
 * // - ZenohOnly + bridge unavailable: falls back to Qt propagation
 * 
 * // Node subscribes to Zenoh topic
 * ZenohBridge::instance().subscribe(nodeId, portIdx, 
 *     [this](std::shared_ptr<NodeData> data) {
 *         this->setInData(data, portIdx);
 *     });
 * @endcode
 *
 * **Key Naming Convention:**
 * @code
 * // Node-to-node transport (ZenohOnly mode) with flow scoping:
 * cvdev/{computer_id}/{daemon_id}/{node_id}/output/{port_idx}/data  // publisher side
 * cvdev/{computer_id}/{daemon_id}/{node_id}/input/{port_idx}/data   // external injection
 *
 *
 * // ZenohPublish / ZenohSubscribe bridge nodes:
 * Plain channel name or custom key expression, e.g., "camera_feed"
 *
 * Examples (computer_id = "my_robot"):
 * cvdev/my_robot/process.flow/ImageLoader_1/output/0/data  (with flow filename)
 * cvdev/my_robot/ImageLoader_1/output/0/data               (without flow filename)
 * camera_feed                                               (bridge channel)
 * @endcode
 *
 * **Performance Considerations:**
 * - Qt signals (same process): 10-100 nanoseconds latency
 * - Zenoh (same machine): 1-10 milliseconds latency
 * - Use Qt for local dataflow, Zenoh for distributed scenarios
 * - Image serialization adds 5-20ms overhead (JPEG compression)
 *
 * @see NodeDataSerializer for data serialization
 * @see PBNodeDelegateModel for node integration
 */

#ifdef ZENOH_ENABLED
#include <zenoh.h>
#endif

#include <QObject>
#include <QString>
#include <QMap>
#include <QMutex>
#include <memory>
#include <functional>
#include <QtNodes/NodeData>

#include "TransportMode.hpp"

/**
 * @class ZenohBridge
 * @brief Singleton class managing Zenoh pub/sub for distributed dataflow.
 *
 * Provides a unified interface for publishing and subscribing to CVDev
 * node data over Zenoh, enabling distributed execution across processes
 * or machines while maintaining compatibility with Qt-based local dataflow.
 *
 * **Singleton Access:**
 * @code
 * ZenohBridge& bridge = ZenohBridge::instance();
 * @endcode
 *
 * **Lifecycle:**
 * @code
 * // Application startup
 * ZenohBridge::instance().initialize("session_abc123");
 * 
 * // Runtime usage
 * bridge.publish(nodeId, portIdx, data);
 * bridge.subscribe(nodeId, portIdx, callback);
 * 
 * // Application shutdown
 * ZenohBridge::instance().shutdown();
 * @endcode
 */
class ZenohBridge : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Callback function type for receiving Zenoh data.
     *
     * Invoked when data arrives on a subscribed topic. The callback
     * receives deserialized NodeData ready for processing.
     *
     * @param data Deserialized NodeData from Zenoh payload
     *
     * **Example:**
     * @code
     * auto callback = [this](std::shared_ptr<NodeData> data) {
     *     if (auto image = std::dynamic_pointer_cast<CVImageData>(data)) {
     *         processImage(image->data());
     *     }
     * };
     * 
     * ZenohBridge::instance().subscribe(nodeId, 0, callback);
     * @endcode
     */
    using DataCallback = std::function<void(std::shared_ptr<QtNodes::NodeData>)>;

    /**
     * @brief Returns the singleton instance.
     *
     * @return ZenohBridge& Singleton instance reference
     *
     * **Usage:**
     * @code
     * ZenohBridge& bridge = ZenohBridge::instance();
     * bridge.initialize("session_id");
     * @endcode
     */
    static ZenohBridge& instance();

    /**
     * @brief Initializes the Zenoh session with session ID.
     *
     * Connects to the Zenoh router (local or remote) and prepares
     * for pub/sub operations. Must be called before any publish/subscribe.
     *
    * @param computerId Stable computer identifier (used in key prefix).
     *        Defaults to the computer hostname if empty. User-configurable via
     *        the "Zenoh Configuration" dialog.
     * @param zenohConfig Optional Zenoh config (default: peer mode, localhost)
     * @return bool True if initialization succeeded, false otherwise
     *
     * **Example:**
     * @code
    * // Simple initialization — computer_id from settings or hostname
     * if (!ZenohBridge::instance().initialize("my_robot")) {
     *     qWarning() << "Zenoh initialization failed - using Qt mode only";
     * }
     *
     * // Custom configuration (connect to router at specific address)
     * QString config = "mode=client;connect=tcp/192.168.1.100:7447";
     * ZenohBridge::instance().initialize("my_robot", config);
     * @endcode
     *
     * **Config Examples:**
     * @code
     * // Peer mode (default) - automatic discovery
     * "" or "mode=peer"
     * 
     * // Client mode - connect to specific router
     * "mode=client;connect=tcp/192.168.1.100:7447"
     * 
     * // Router mode - act as router for other peers/clients
     * "mode=router"
     * @endcode
     *
     * @note If Zenoh is not available (ZENOH_ENABLED not defined), returns false
     * @note Session ID becomes part of all topic keys
     */
    bool initialize(const QString& computerId, const QString& zenohConfig = "");

    /**
    * @brief Sets the computer ID used as the first segment in all Zenoh keys.
     *
     * Defaults to the computer hostname at construction. Users can override this
     * via the "Zenoh Configuration" dialog so keys are stable across sessions.
     *
    * @param computerId User-defined identifier (e.g., "my_robot", "factory_line_1")
     *
     * **Key formats after setting:**
     * @code
    * cvdev/{computer_id}/{daemon_id}/{nodeId}/output/{portIdx}/data
    * cvdev/{computer_id}/{daemon_id}/{nodeId}/input/{portIdx}/data
    * cvdev/{computer_id}/{channelName}          // ZenohPublish/Subscribe
     * @endcode
     */
    void setComputerId(const QString& computerId);

    /**
    * @brief Returns the current computer ID used in Zenoh key prefixes.
     *
    * @return QString Computer ID (default: computer hostname)
     */
    QString getComputerId() const;

    /**
     * @brief Shuts down the Zenoh session and cleans up resources.
     *
     * Closes all publishers, subscribers, and the Zenoh session.
     * Safe to call even if not initialized.
     *
     * **Example:**
     * @code
     * // Application shutdown
     * ZenohBridge::instance().shutdown();
     * @endcode
     */
    void shutdown();

    /**
     * @brief Publishes node data to Zenoh topic.
     *
     * Serializes the data and publishes it to the topic constructed
     * from nodeId and portIdx. Automatically creates publisher if needed.
     *
     * @param nodeId Unique node identifier (e.g., "GaussianBlur_5")
     * @param portIdx Output port index (0-based)
     * @param data NodeData to publish (will be serialized)
     * @param flowFilename Optional flow filename for key scoping (empty = omit from key)
     * @return bool True if publish succeeded, false if Zenoh not initialized
     *
     * **Example:**
     * @code
     * // Node publishes output image in ZenohOnly mode
     * auto imageData = std::make_shared<CVImageData>(processedMat);
     * ZenohBridge::instance().publish(
     *     QString::number(id()),  // Node ID
     *     0,                      // Output port 0
     *     imageData,
     *     "MyFlow.flow"           // Optional: scoped to this flow
     * );
     * @endcode
     *
     * **Topic Construction:**
     * @code
    * Key: cvdev/{computer_id}/{daemon_id}/{nodeId}/output/{portIdx}/data (no flow filename)
     * 
     * Example: cvdev/robot/Blur_5/output/0/data
     *          cvdev/robot/process.flow/Blur_5/output/0/data
     * @endcode
     *
     * @note Serialization happens automatically via NodeDataSerializer
     * @note Publisher is created lazily on first publish
     */
    bool publish(const QString& nodeId, int portIdx, std::shared_ptr<QtNodes::NodeData> data, const QString& flowFilename = QString());

    /**
     * @brief Publishes raw serialized data to a custom Zenoh key.
     *
     * Low-level publish method for bridge nodes and custom use cases.
     * Unlike publish(), this method accepts pre-serialized data and
     * a custom key expression, allowing flexible topic naming.
     *
    * @param key Full Zenoh key expression (e.g., "cvdev/channel/my_channel")
     * @param payload Pre-serialized binary data (from NodeDataSerializer)
     * @return bool True if publish succeeded, false if Zenoh not initialized
     *
     * **Example:**
     * @code
     * // Publish to custom channel
     * QByteArray serialized = NodeDataSerializer::serialize(data);
    * QString key = "cvdev/channel/camera_feed";
     * ZenohBridge::instance().publishRaw(key, serialized);
     * @endcode
     */
    bool publishRaw(const QString& key, const QByteArray& payload);

    /**
     * @brief Subscribes to node data from Zenoh topic.
     *
     * Creates a subscriber for the specified node/port and invokes
     * the callback when data arrives. Automatically deserializes payload.
     *
     * @param nodeId Unique node identifier to subscribe to
     * @param portIdx Input port index (0-based)
     * @param callback Function called when data arrives
     * @param flowFilename Optional flow filename for key scoping (empty = omit from key)
     * @return bool True if subscribe succeeded, false if Zenoh not initialized
     *
     * **Example:**
     * @code
     * // Node subscribes to input from another node
     * ZenohBridge::instance().subscribe(
     *     "ImageLoader_1",  // Subscribe to ImageLoader node
     *     0,                // Its output port 0
     *     [this](std::shared_ptr<NodeData> data) {
     *         // Receive and process
     *         if (auto image = std::dynamic_pointer_cast<CVImageData>(data)) {
     *             this->setInData(image, 0);  // Set as our input
     *         }
     *     },
     *     "MyFlow.flow"     // Optional: scoped to this flow
     * );
     * @endcode
     *
     * **Callback Invocation:**
     * @code
     * // Callback runs on Zenoh thread - use Qt signals for thread safety
     * auto callback = [this](std::shared_ptr<NodeData> data) {
     *     QMetaObject::invokeMethod(this, [this, data]() {
     *         // Now on main thread
     *         processData(data);
     *     }, Qt::QueuedConnection);
     * };
     * @endcode
     *
     * @warning Callback invoked on Zenoh thread - use Qt::QueuedConnection for GUI updates
     * @note Deserialization happens automatically via NodeDataSerializer
     */
    bool subscribe(const QString& nodeId, int portIdx, DataCallback callback, const QString& flowFilename = QString());

    /**
     * @brief Subscribes to raw data from a custom Zenoh key.
     *
     * Low-level subscribe method for bridge nodes and custom use cases.
     * Receives raw binary payloads without automatic deserialization.
     *
    * @param key Full Zenoh key expression (e.g., "cvdev/channel/my_channel")
     * @param callback Function called when raw data arrives (receives QByteArray)
     * @return bool True if subscribe succeeded, false if Zenoh not initialized
     *
     * **Example:**
     * @code
     * // Subscribe to custom channel
    * QString key = "cvdev/channel/camera_feed";
     * ZenohBridge::instance().subscribeRaw(key, 
     *     [this](const QByteArray& payload) {
     *         auto data = NodeDataSerializer::deserialize(payload);
     *         if (data) this->outputData(data);
     *     }
     * );
     * @endcode
     */
    /** @brief Callback type for raw transport payload subscribers. */
    using RawDataCallback = std::function<void(const QByteArray&)>;
    bool subscribeRaw(const QString& key, RawDataCallback callback);
    bool subscribeRaw(const QString& key, const QString& subscriberId, RawDataCallback callback);

    /**
     * @brief Unsubscribes from a node/port topic.
     *
     * Removes the subscriber for the specified node/port combination.
     *
     * @param nodeId Node identifier
     * @param portIdx Port index
     *
     * **Example:**
     * @code
     * // Node disconnects or changes mode
     * ZenohBridge::instance().unsubscribe("ImageLoader_1", 0);
     * @endcode
     */
    void unsubscribe(const QString& nodeId, int portIdx, const QString& flowFilename = QString());

    /**
     * @brief Unsubscribes from a custom Zenoh key.
     *
     * Removes the subscriber for the specified custom key.
     *
     * @param key Full Zenoh key expression
     *
     * **Example:**
     * @code
     * QString key = "cvdev/" + sessionId() + "/channel/camera_feed";
     * ZenohBridge::instance().unsubscribeRaw(key);
     * @endcode
     */
    void unsubscribeRaw(const QString& key);
    void dispatchRawPayload(const QString& key, const QByteArray& payload);
    void unsubscribeRaw(const QString& key, const QString& subscriberId);

    /**
     * @brief Checks if Zenoh is initialized and ready.
     *
     * @return bool True if Zenoh session is active
     *
     * **Example:**
     * @code
     * if (ZenohBridge::instance().isInitialized()) {
     *     // Safe to publish/subscribe
     *     bridge.publish(nodeId, port, data);
     * } else {
     *     // Fall back to Qt signals
     *     emit dataUpdated(port);
     * }
     * @endcode
     */
    bool isInitialized() const;



private:
    ZenohBridge();
    ~ZenohBridge();

    // Prevent copying
    ZenohBridge(const ZenohBridge&) = delete;
    ZenohBridge& operator=(const ZenohBridge&) = delete;

    /**
     * @brief Constructs an output-port Zenoh key for node-to-node publishing.
     *
    * Format: cvdev/{computer_id}/{daemon_id}/{nodeId}/output/{portIdx}/data (when daemon_id provided)
     *
     * @param nodeId Unique node identifier
     * @param portIdx Output port index
     * @param flowFilename Optional flow filename for key scoping (empty = omit from key)
     *
     * Used by publish() and subscribe() so publishers and subscribers share the same key.
     */
    QString makeOutputKey(const QString& nodeId, int portIdx, const QString& flowFilename = QString()) const;

    /**
     * @brief Constructs an input-port Zenoh key for external data injection.
     *
    * Format: cvdev/{computer_id}/{daemon_id}/{nodeId}/input/{portIdx}/data (when daemon_id provided)
     *
     * @param nodeId Unique node identifier
     * @param portIdx Input port index
     * @param flowFilename Optional flow filename for key scoping (empty = omit from key)
     *
     * Intended for external systems that push data directly into a node's input port.
     */
    QString makeInputKey(const QString& nodeId, int portIdx, const QString& flowFilename = QString()) const;

#ifdef ZENOH_ENABLED
    z_owned_session_t mZenohSession;       ///< Zenoh session handle
    QMap<QString, z_owned_publisher_t> mPublishers;  ///< Topic → Publisher map
    QMap<QString, z_owned_subscriber_t> mSubscribers; ///< Topic → Subscriber map
#endif
    QMutex mRawSubscriberMutex;
    QMap<QString, QMap<QString, RawDataCallback>> mRawSubscribersByKey; ///< Topic -> subscriberId -> callback

    QString msComputerId;               ///< Computer ID used in all key prefixes (default: hostname)
    bool mbInitialized;                    ///< Initialization state
};
