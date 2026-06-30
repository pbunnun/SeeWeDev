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

/**
 * @file ZenohBridge.cpp
 * @brief Runtime implementation of Zenoh transport integration for CVDev.
 *
 * Contains key sanitization, configuration parsing, session lifecycle, and
 * publish/subscribe plumbing used by distributed node-data transport.
 */

#include "ZenohBridge.hpp"
#include "NodeDataSerializer.hpp"
#include "DebugLogging.hpp"
#include <QDebug>
#include <QRegularExpression>
#include <QSysInfo>

namespace {

QString sanitizeZenohKeySegment(const QString& value, const QString& fallback)
{
    /// Ensure generated key segments stay stable and router-safe.
    QString sanitized = value.trimmed();
    if (sanitized.isEmpty()) {
        sanitized = fallback;
    }

    sanitized.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral("_"));
    sanitized.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("_"));
    sanitized.replace(QRegularExpression(QStringLiteral("_+")), QStringLiteral("_"));
    sanitized.remove(QRegularExpression(QStringLiteral("^_+|_+$")));

    return sanitized.isEmpty() ? fallback : sanitized;
}

#ifdef ZENOH_ENABLED
void insertZenohConfigJson5(z_owned_config_t& config, const char* key, const QString& value)
{
    if (!key || value.trimmed().isEmpty()) {
        return;
    }

    std::string keyStr(key);
    std::string valueStr = value.trimmed().toStdString();
    zc_config_insert_json5(z_loan_mut(config), keyStr.c_str(), valueStr.c_str());
}

QString toJsonString(const QString& value)
{
    QString escaped = value;
    escaped.replace('\\', "\\\\");
    escaped.replace('"', "\\\"");
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

QString extractFirstMatch(const QString& text, const QRegularExpression& regex)
{
    const QRegularExpressionMatch m = regex.match(text);
    if (!m.hasMatch() || m.lastCapturedIndex() < 1) {
        return QString();
    }
    return m.captured(1).trimmed();
}

QString extractBoolean(const QString& text, const QRegularExpression& regex)
{
    QString value = extractFirstMatch(text, regex).toLower();
    if (value == "true" || value == "false") {
        return value;
    }
    return QString();
}

bool applyJsonLikeZenohConfig(z_owned_config_t& config, const QString& zenohConfig)
{
    const QString cfg = zenohConfig.trimmed();
    if (!cfg.startsWith('{')) {
        return false;
    }

    const auto dotAll = QRegularExpression::DotMatchesEverythingOption;

    const QString mode = extractFirstMatch(
        cfg,
        QRegularExpression(QStringLiteral("mode\\s*:\\s*\"([^\"]+)\"")));
    if (!mode.isEmpty()) {
        insertZenohConfigJson5(config, "mode", toJsonString(mode));
    }

    const QString connectEndpoints = extractFirstMatch(
        cfg,
        QRegularExpression(QStringLiteral("connect\\s*:\\s*\\{.*?endpoints\\s*:\\s*\\[(.*?)\\]"), dotAll));
    if (!connectEndpoints.isEmpty()) {
        insertZenohConfigJson5(config, "connect/endpoints", QStringLiteral("[") + connectEndpoints + QStringLiteral("]"));
    }

    const QString listenEndpoints = extractFirstMatch(
        cfg,
        QRegularExpression(QStringLiteral("listen\\s*:\\s*\\{.*?endpoints\\s*:\\s*\\[(.*?)\\]"), dotAll));
    if (!listenEndpoints.isEmpty()) {
        insertZenohConfigJson5(config, "listen/endpoints", QStringLiteral("[") + listenEndpoints + QStringLiteral("]"));
    }

    const QString multicast = extractBoolean(
        cfg,
        QRegularExpression(QStringLiteral("multicast\\s*:\\s*\\{.*?enabled\\s*:\\s*(true|false)"), dotAll));
    if (!multicast.isEmpty()) {
        insertZenohConfigJson5(config, "scouting/multicast/enabled", multicast);
    }

    const QString sharedMemory = extractBoolean(
        cfg,
        QRegularExpression(QStringLiteral("shared_memory\\s*:\\s*\\{.*?enabled\\s*:\\s*(true|false)"), dotAll));
    if (!sharedMemory.isEmpty()) {
        insertZenohConfigJson5(config, "transport/shared_memory/enabled", sharedMemory);
    }

    return true;
}

void applyLegacyKvZenohConfig(z_owned_config_t& config, const QString& zenohConfig)
{
    const QStringList params = zenohConfig.split(';', Qt::SkipEmptyParts);
    for (const QString& param : params) {
        const QStringList kv = param.split('=', Qt::KeepEmptyParts);
        if (kv.size() != 2) {
            continue;
        }

        const QString key = kv[0].trimmed();
        QString value = kv[1].trimmed();
        if (key.isEmpty() || value.isEmpty()) {
            continue;
        }

        if (key == "mode" && !value.startsWith('"')) {
            value = toJsonString(value);
        }

        insertZenohConfigJson5(config, key.toUtf8().constData(), value);
    }
}
#endif


} // namespace

// ============================================================================
// Singleton Instance
// ============================================================================

ZenohBridge& ZenohBridge::instance()
{
    static ZenohBridge instance;
    return instance;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

ZenohBridge::ZenohBridge()
    : QObject(nullptr)
    , msComputerId(QSysInfo::machineHostName())
    , mbInitialized(false)
{
#ifndef ZENOH_ENABLED
    DEBUG_LOG_INFO() << "[ZenohBridge] Zenoh not available - hybrid mode disabled (Qt-only)";
#endif
}

ZenohBridge::~ZenohBridge()
{
    shutdown();
}

void ZenohBridge::setComputerId(const QString& computerId)
{
    msComputerId = computerId;
}

QString ZenohBridge::getComputerId() const
{
    return msComputerId;
}

bool ZenohBridge::isInitialized() const
{
    return mbInitialized;
}

// ============================================================================
// Initialization / Shutdown
// ============================================================================

bool ZenohBridge::initialize(const QString& computerId, const QString& zenohConfig)
{
#ifdef ZENOH_ENABLED
    if (mbInitialized) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Already initialized";
        return true;
    }

    msComputerId = sanitizeZenohKeySegment(
        computerId.isEmpty() ? QSysInfo::machineHostName() : computerId,
        QStringLiteral("application"));

    // Create Zenoh config (Zenoh 1.x API)
    z_owned_config_t config;
    z_config_default(&config);
    
    if (!zenohConfig.isEmpty()) {
        // Accept both current JSON-like config and legacy key=value;key=value format.
        if (!applyJsonLikeZenohConfig(config, zenohConfig)) {
            applyLegacyKvZenohConfig(config, zenohConfig);
        }
    }

    // Open Zenoh session (Zenoh 1.x API)
    z_result_t result = z_open(&mZenohSession, z_move(config), NULL);
    
    if (result != Z_OK) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Failed to open Zenoh session, error code:" << result;
        return false;
    }

    mbInitialized = true;
    DEBUG_LOG_INFO() << "[ZenohBridge] Initialized with computer ID:" << msComputerId;
    return true;

#else
    Q_UNUSED(computerId);
    Q_UNUSED(zenohConfig);
    DEBUG_LOG_WARNING() << "[ZenohBridge] Zenoh not available - cannot initialize";
    return false;
#endif
}

void ZenohBridge::shutdown()
{
#ifdef ZENOH_ENABLED
    if (!mbInitialized) {
        return;
    }

    DEBUG_LOG_INFO() << "[ZenohBridge] Shutting down...";

    // Close all subscribers
    for (auto it = mSubscribers.begin(); it != mSubscribers.end(); ++it) {
        z_undeclare_subscriber(z_move(it.value()));
    }
    mSubscribers.clear();

    // Close all publishers
    for (auto it = mPublishers.begin(); it != mPublishers.end(); ++it) {
        z_undeclare_publisher(z_move(it.value()));
    }
    mPublishers.clear();

    // Close session (Zenoh 1.x API - use z_loan_mut for mutable reference)
    z_close(z_loan_mut(mZenohSession), NULL);

    mbInitialized = false;
    DEBUG_LOG_INFO() << "[ZenohBridge] Shutdown complete";
#endif
}

// ============================================================================
// Key Construction
// ============================================================================

QString ZenohBridge::makeOutputKey(const QString& nodeId, int portIdx, const QString& flowFilename) const
{
    const QString effectiveComputerId = sanitizeZenohKeySegment(msComputerId, QStringLiteral("computer"));
    const QString effectiveFlowFilename = sanitizeZenohKeySegment(flowFilename, QStringLiteral("Untitle"));

    return QString("cvdev/%1/%2/%3/output/%4/data")
        .arg(effectiveComputerId)
           .arg(effectiveFlowFilename)
           .arg(nodeId)
           .arg(portIdx);
}

QString ZenohBridge::makeInputKey(const QString& nodeId, int portIdx, const QString& flowFilename) const
{
    const QString effectiveComputerId = sanitizeZenohKeySegment(msComputerId, QStringLiteral("computer"));
    const QString effectiveFlowFilename = sanitizeZenohKeySegment(flowFilename, QStringLiteral("Untitle"));

    return QString("cvdev/%1/%2/%3/input/%4/data")
        .arg(effectiveComputerId)
           .arg(effectiveFlowFilename)
           .arg(nodeId)
           .arg(portIdx);
}

// ============================================================================
// Publish
// ============================================================================

bool ZenohBridge::publish(const QString& nodeId, int portIdx, std::shared_ptr<QtNodes::NodeData> data, const QString& flowFilename)
{
#ifdef ZENOH_ENABLED
    if (!mbInitialized) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Not initialized - cannot publish";
        return false;
    }

    if (!data) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Null data - cannot publish";
        return false;
    }

    // Construct key
    QString key = makeOutputKey(nodeId, portIdx, flowFilename);

    // Serialize data
    std::vector<uint8_t> payload = NodeDataSerializer::serialize(data);
    if (payload.empty()) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Serialization failed for key:" << key;
        return false;
    }

    // Get or create publisher (Zenoh 1.x API)
    if (!mPublishers.contains(key)) {
        // Store std::string to keep it alive while using c_str()
        std::string keyStr = key.toStdString();
        z_view_keyexpr_t keyexpr;
        z_view_keyexpr_from_str(&keyexpr, keyStr.c_str());
        
        z_owned_publisher_t pub;
        z_result_t result = z_declare_publisher(z_loan(mZenohSession),
                                                 &pub,
                                                 z_loan(keyexpr),
                                                 NULL);

        if (result != Z_OK) {
            DEBUG_LOG_WARNING() << "[ZenohBridge] Failed to create publisher for key:" << key;
            return false;
        }

        mPublishers[key] = pub;
        DEBUG_LOG_INFO() << "[ZenohBridge] Created publisher for key:" << key;
    }

    // Publish data (Zenoh 1.x API)
    z_publisher_put_options_t options;
    z_publisher_put_options_default(&options);
    
    z_owned_bytes_t bytes;
    z_bytes_copy_from_buf(&bytes, payload.data(), payload.size());
    
    z_publisher_put(z_loan(mPublishers[key]),
                    z_move(bytes),
                    &options);

    DEBUG_LOG_INFO() << "[ZenohBridge] Published data to key:" << key
                     << "bytes:" << static_cast<qulonglong>(payload.size());

    return true;

#else
    Q_UNUSED(nodeId);
    Q_UNUSED(portIdx);
    Q_UNUSED(data);
    return false;
#endif
}

bool ZenohBridge::publishRaw(const QString& key, const QByteArray& payload)
{
#ifdef ZENOH_ENABLED
    if (!mbInitialized) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Not initialized - cannot publish raw";
        return false;
    }

    if (payload.isEmpty()) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Empty payload - cannot publish";
        return false;
    }

    // Get or create publisher for this key (Zenoh 1.x API)
    if (!mPublishers.contains(key)) {
        // Store std::string to keep it alive while using c_str()
        std::string keyStr = key.toStdString();
        z_view_keyexpr_t keyexpr;
        z_view_keyexpr_from_str(&keyexpr, keyStr.c_str());
        
        z_owned_publisher_t pub;
        z_result_t result = z_declare_publisher(z_loan(mZenohSession),
                                                 &pub,
                                                 z_loan(keyexpr),
                                                 NULL);

        if (result != Z_OK) {
            DEBUG_LOG_WARNING() << "[ZenohBridge] Failed to create publisher for key:" << key;
            return false;
        }

        mPublishers[key] = pub;
        DEBUG_LOG_INFO() << "[ZenohBridge] Created publisher for key:" << key;
    }

    // Publish raw data (Zenoh 1.x API)
    z_publisher_put_options_t options;
    z_publisher_put_options_default(&options);
    
    z_owned_bytes_t bytes;
    z_bytes_copy_from_buf(&bytes, 
                          reinterpret_cast<const uint8_t*>(payload.data()),
                          payload.size());
    
    z_publisher_put(z_loan(mPublishers[key]),
                    z_move(bytes),
                    &options);

    DEBUG_LOG_INFO() << "[ZenohBridge] Published raw data to key:" << key
                     << "bytes:" << static_cast<qulonglong>(payload.size());

    return true;

#else
    Q_UNUSED(key);
    Q_UNUSED(payload);
    return false;
#endif
}

// ============================================================================
// Subscribe
// ============================================================================

#ifdef ZENOH_ENABLED
struct ZenohDataCallbackHolder {
    ZenohBridge::DataCallback callback;
};

struct ZenohRawCallbackHolder {
    ZenohBridge* bridge;
    QString key;
};

// Static callback wrapper for Zenoh (C API requires static function) - Zenoh 1.x API
static void zenohDataHandler(z_loaned_sample_t* sample, void* arg)
{
    // Extract callback from user data
    auto* callbackHolder = static_cast<ZenohDataCallbackHolder*>(arg);
    if (!callbackHolder) {
        return;
    }

    // Extract payload (Zenoh 1.x API)
    const z_loaned_bytes_t* payload_bytes = z_sample_payload(sample);
    
    // Get payload data
    z_bytes_reader_t reader = z_bytes_get_reader(payload_bytes);
    size_t payload_len = z_bytes_reader_remaining(&reader);
    
    std::vector<uint8_t> payload(payload_len);
    z_bytes_reader_read(&reader, payload.data(), payload_len);

    // Deserialize
    auto data = NodeDataSerializer::deserialize(payload);

    if (data) {
        // Invoke user callback
        callbackHolder->callback(data);
    }
}

static void zenohDataHandlerDrop(void* arg)
{
    delete static_cast<ZenohDataCallbackHolder*>(arg);
}

// Static callback wrapper for raw data subscriptions - Zenoh 1.x API
static void zenohRawDataHandler(z_loaned_sample_t* sample, void* arg)
{
    // Extract callback from user data
    auto* callbackHolder = static_cast<ZenohRawCallbackHolder*>(arg);
    if (!callbackHolder || !callbackHolder->bridge) {
        return;
    }

    // Extract payload (Zenoh 1.x API)
    const z_loaned_bytes_t* payload_bytes = z_sample_payload(sample);
    
    // Get payload data
    z_bytes_reader_t reader = z_bytes_get_reader(payload_bytes);
    size_t payload_len = z_bytes_reader_remaining(&reader);
    
    QByteArray payload(payload_len, Qt::Uninitialized);
    z_bytes_reader_read(&reader, reinterpret_cast<uint8_t*>(payload.data()), payload_len);

    callbackHolder->bridge->dispatchRawPayload(callbackHolder->key, payload);
}

static void zenohRawDataHandlerDrop(void* arg)
{
    delete static_cast<ZenohRawCallbackHolder*>(arg);
}
#endif

bool ZenohBridge::subscribe(const QString& nodeId, int portIdx, DataCallback callback, const QString& flowFilename)
{
#ifdef ZENOH_ENABLED
    if (!mbInitialized) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Not initialized - cannot subscribe";
        return false;
    }

    // Construct key
    QString key = makeOutputKey(nodeId, portIdx, flowFilename);

    // Check if already subscribed
    if (mSubscribers.contains(key)) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Already subscribed to key:" << key;
        return false;
    }

    // Keep callback userdata alive independently from container reallocation/removal.
    auto* callbackHolder = new ZenohDataCallbackHolder{callback};

    // Create Zenoh subscriber (Zenoh 1.x API)
    z_owned_closure_sample_t closure;
    z_closure_sample(&closure,
                     zenohDataHandler,
                     zenohDataHandlerDrop,
                     callbackHolder);

    // Store std::string to keep it alive while using c_str()
    std::string keyStr = key.toStdString();
    z_view_keyexpr_t keyexpr;
    z_view_keyexpr_from_str(&keyexpr, keyStr.c_str());

    z_owned_subscriber_t sub;
    z_result_t result = z_declare_subscriber(z_loan(mZenohSession),
                                              &sub,
                                              z_loan(keyexpr),
                                              z_move(closure),
                                              NULL);

    if (result != Z_OK) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Failed to create subscriber for key:" << key;
        delete callbackHolder;
        return false;
    }

    mSubscribers[key] = sub;
    DEBUG_LOG_INFO() << "[ZenohBridge] Subscribed to key:" << key;
    return true;

#else
    Q_UNUSED(nodeId);
    Q_UNUSED(portIdx);
    Q_UNUSED(callback);
    return false;
#endif
}

// ============================================================================
// Unsubscribe
// ============================================================================

void ZenohBridge::unsubscribe(const QString& nodeId, int portIdx, const QString& flowFilename)
{
#ifdef ZENOH_ENABLED
    if (!mbInitialized) {
        return;
    }

    QString key = makeOutputKey(nodeId, portIdx, flowFilename);

    if (mSubscribers.contains(key)) {
        z_undeclare_subscriber(z_move(mSubscribers[key]));
        mSubscribers.remove(key);
        DEBUG_LOG_INFO() << "[ZenohBridge] Unsubscribed from key:" << key;
    }
#else
    Q_UNUSED(nodeId);
    Q_UNUSED(portIdx);
    Q_UNUSED(flowFilename);
#endif
}

bool ZenohBridge::subscribeRaw(const QString& key, RawDataCallback callback)
{
    return subscribeRaw(key, QStringLiteral("__default__"), callback);
}

bool ZenohBridge::subscribeRaw(const QString& key, const QString& subscriberId, RawDataCallback callback)
{
#ifdef ZENOH_ENABLED
    if (!mbInitialized) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Not initialized - cannot subscribe raw";
        return false;
    }

    if (key.trimmed().isEmpty() || subscriberId.trimmed().isEmpty() || !callback) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Invalid key, subscriberId, or callback - cannot subscribe raw";
        return false;
    }

    {
        QMutexLocker locker(&mRawSubscriberMutex);
        auto existingByKey = mRawSubscribersByKey.find(key);
        if (existingByKey != mRawSubscribersByKey.end()) {
            if (existingByKey->contains(subscriberId)) {
                DEBUG_LOG_WARNING() << "[ZenohBridge] Duplicate raw subscriber id for key:" << key << subscriberId;
                return false;
            }

            existingByKey->insert(subscriberId, callback);
            DEBUG_LOG_INFO() << "[ZenohBridge] Added shared raw subscriber for key:" << key << subscriberId;
            return true;
        }
    }

    // Check if already subscribed
    if (mSubscribers.contains(key)) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Already subscribed to key:" << key;
        return false;
    }

    // Keep callback userdata alive independently from container reallocation/removal.
    auto* callbackHolder = new ZenohRawCallbackHolder{this, key};

    // Create Zenoh subscriber with raw data handler (Zenoh 1.x API)
    z_owned_closure_sample_t closure;
    z_closure_sample(&closure,
                     zenohRawDataHandler,
                     zenohRawDataHandlerDrop,
                     callbackHolder);

    // Store std::string to keep it alive while using c_str()
    std::string keyStr = key.toStdString();
    z_view_keyexpr_t keyexpr;
    z_view_keyexpr_from_str(&keyexpr, keyStr.c_str());

    z_owned_subscriber_t sub;
    z_result_t result = z_declare_subscriber(z_loan(mZenohSession),
                                              &sub,
                                              z_loan(keyexpr),
                                              z_move(closure),
                                              NULL);

    if (result != Z_OK) {
        DEBUG_LOG_WARNING() << "[ZenohBridge] Failed to create subscriber for key:" << key;
        delete callbackHolder;
        return false;
    }

    mSubscribers[key] = sub;
    {
        QMutexLocker locker(&mRawSubscriberMutex);
        mRawSubscribersByKey[key].insert(subscriberId, callback);
    }
    DEBUG_LOG_INFO() << "[ZenohBridge] Subscribed to raw key:" << key;
    return true;

#else
    Q_UNUSED(key);
    Q_UNUSED(callback);
    return false;
#endif
}

void ZenohBridge::unsubscribeRaw(const QString& key)
{
    unsubscribeRaw(key, QStringLiteral("__default__"));
}

void ZenohBridge::unsubscribeRaw(const QString& key, const QString& subscriberId)
{
#ifdef ZENOH_ENABLED
    if (!mbInitialized) {
        return;
    }

    bool removeTransportSubscriber = false;
    {
        QMutexLocker locker(&mRawSubscriberMutex);
        auto byKeyIt = mRawSubscribersByKey.find(key);
        if (byKeyIt != mRawSubscribersByKey.end()) {
            byKeyIt->remove(subscriberId);
            if (byKeyIt->isEmpty()) {
                mRawSubscribersByKey.erase(byKeyIt);
                removeTransportSubscriber = true;
            }
        }
    }

    if (removeTransportSubscriber && mSubscribers.contains(key)) {
        z_undeclare_subscriber(z_move(mSubscribers[key]));
        mSubscribers.remove(key);
        DEBUG_LOG_INFO() << "[ZenohBridge] Unsubscribed from raw key:" << key;
    }
#else
    Q_UNUSED(key);
    Q_UNUSED(subscriberId);
#endif
}

void ZenohBridge::dispatchRawPayload(const QString& key, const QByteArray& payload)
{
    QList<RawDataCallback> callbacks;
    {
        QMutexLocker locker(&mRawSubscriberMutex);
        const auto it = mRawSubscribersByKey.constFind(key);
        if (it == mRawSubscribersByKey.constEnd()) {
            return;
        }
        callbacks = it.value().values();
    }

    for (const RawDataCallback& callback : callbacks) {
        if (callback) {
            callback(payload);
        }
    }
}

// ============================================================================
