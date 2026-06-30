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
 * @file CycloneDDSBridge.cpp
 * @brief Implementation of CycloneDDS singleton transport bridge.
 *
 * Implements DDS entity lifecycle (participant creation/destruction), topic-scoped
 * reader/writer pools (lazy creation), callback dispatch mechanism with subscriber ID
 * tracking, and Qt-integrated polling loop for data delivery. Includes conditional
 * compilation support for CYCLONEDDS_ENABLED builds.
 *
 * **Key Implementation Details:**
 * - Pimpl (Private Implementation) pattern isolates DDS types
 * - Mutex-protected callback dispatch table (topic -> subscriber ID -> callback)
 * - Qt polling timer (10ms interval) drains all active readers
 * - Lazy reader/writer creation per topic
 * - Safe unsubscribe with partial callback removal
 * - Graceful degradation when CycloneDDS is disabled
 *
 * **DDS Polling Architecture:**
 * - Timer fires every 10ms
 * - Collects active reader IDs (thread-safe snapshot)
 * - Calls handleReaderData() for each reader
 * - Callbacks dispatch on Qt event loop via QMetaObject::invokeMethod
 */

#include "CycloneDDSBridge.hpp"

#include "CycloneDDSBridge.hpp"
#include "DebugLogging.hpp"

#include <QSysInfo>
#include <QMetaObject>
#include <QCoreApplication>
#include <QDebug>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

#ifdef CYCLONEDDS_ENABLED
#include <dds/dds.h>
#include <dds/ddsc/dds_public_alloc.h>
#include "CycloneDDSRawPayload.h"
#endif

struct CycloneDDSBridge::Impl
{
    ///< Synchronizes concurrent publish/subscribe and entity updates.
    QMutex mutex;
    ///< Topic -> subscriber id -> callback dispatch table.
    QHash<QString, QHash<QString, RawDataCallback>> callbacksByTopic;

#ifdef CYCLONEDDS_ENABLED
    dds_entity_t participant{0};
    dds_entity_t publisher{0};
    dds_entity_t subscriber{0};

    QHash<QString, dds_entity_t> topicsByName;
    QHash<QString, dds_entity_t> writersByTopic;
    QHash<QString, dds_entity_t> readersByTopic;
    QHash<dds_entity_t, QString> topicByReader;
#endif
};

#ifdef CYCLONEDDS_ENABLED
namespace {

constexpr bool isValidEntity(dds_entity_t entity)
{
    return entity > 0;
}

}
#endif

CycloneDDSBridge& CycloneDDSBridge::instance()
{
    static CycloneDDSBridge sInstance;
    return sInstance;
}

CycloneDDSBridge::CycloneDDSBridge()
    : msComputerId(QSysInfo::machineHostName())
    , mpImpl(std::make_unique<Impl>())
{
    mpPollTimer = new QTimer(this);
    mpPollTimer->setInterval(10);
    connect(mpPollTimer, &QTimer::timeout, this, [this]() {
#ifdef CYCLONEDDS_ENABLED
        QList<int> readerIds;
        {
            QMutexLocker locker(&mpImpl->mutex);
            for (auto it = mpImpl->topicByReader.constBegin(); it != mpImpl->topicByReader.constEnd(); ++it) {
                readerIds.append(static_cast<int>(it.key()));
            }
        }

        for (int readerId : readerIds) {
            handleReaderData(readerId);
        }
#endif
    });

#ifndef CYCLONEDDS_ENABLED
    DEBUG_LOG_INFO() << "[CycloneDDSBridge] CycloneDDS not available - in-process bus only (Qt-local fallback)";
#endif
}

CycloneDDSBridge::~CycloneDDSBridge()
{
    shutdown();
}

bool CycloneDDSBridge::initialize(const QString& computerId, int domainId, const QString& partition)
{
#ifdef CYCLONEDDS_ENABLED
    QMutexLocker locker(&mpImpl->mutex);

    if (mbInitialized) {
        locker.unlock();
        shutdown();
        locker.relock();
    }

    setComputerId(computerId);
    miDomainId = domainId;
    msPartition = partition.trimmed();

    mpImpl->participant = dds_create_participant(domainId, nullptr, nullptr);
    if (!isValidEntity(mpImpl->participant)) {
        DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Failed to create participant, rc:" << mpImpl->participant;
        mbInitialized = false;
        return false;
    }

    dds_qos_t *pubSubQos = dds_create_qos();
    QByteArray partitionUtf8 = msPartition.toUtf8();
    const char *partitionName = nullptr;
    if (!partitionUtf8.isEmpty()) {
        partitionName = partitionUtf8.constData();
        dds_qset_partition(pubSubQos, 1, &partitionName);
    }

    mpImpl->publisher = dds_create_publisher(mpImpl->participant, pubSubQos, nullptr);
    mpImpl->subscriber = dds_create_subscriber(mpImpl->participant, pubSubQos, nullptr);
    dds_delete_qos(pubSubQos);

    if (!isValidEntity(mpImpl->publisher) || !isValidEntity(mpImpl->subscriber)) {
        DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Failed to create pub/sub entities"
                            << "publisher rc:" << mpImpl->publisher
                            << "subscriber rc:" << mpImpl->subscriber;
        if (isValidEntity(mpImpl->publisher)) {
            dds_delete(mpImpl->publisher);
        }
        if (isValidEntity(mpImpl->subscriber)) {
            dds_delete(mpImpl->subscriber);
        }
        dds_delete(mpImpl->participant);
        mpImpl->participant = 0;
        mpImpl->publisher = 0;
        mpImpl->subscriber = 0;
        mbInitialized = false;
        return false;
    }

    mbInitialized = true;
    if (mpPollTimer && !mpPollTimer->isActive()) {
        mpPollTimer->start();
    }
    DEBUG_LOG_INFO() << "[CycloneDDSBridge] Initialized with computer ID:" << msComputerId
                     << "domain:" << miDomainId
                     << "partition:" << (msPartition.isEmpty() ? QStringLiteral("(default)") : msPartition);
    return true;
#else
    Q_UNUSED(computerId);
    Q_UNUSED(domainId);
    Q_UNUSED(partition);
    mbInitialized = false;
    DEBUG_LOG_WARNING() << "[CycloneDDSBridge] CYCLONEDDS_ENABLED is not defined - CycloneDDS transport unavailable";
    return false;
#endif
}

void CycloneDDSBridge::shutdown()
{
    QMutexLocker locker(&mpImpl->mutex);

    if (mpPollTimer && mpPollTimer->isActive()) {
        mpPollTimer->stop();
    }

    mpImpl->callbacksByTopic.clear();

#ifdef CYCLONEDDS_ENABLED
    for (auto it = mpImpl->readersByTopic.begin(); it != mpImpl->readersByTopic.end(); ++it) {
        if (isValidEntity(it.value())) {
            dds_delete(it.value());
        }
    }
    mpImpl->readersByTopic.clear();
    mpImpl->topicByReader.clear();

    for (auto it = mpImpl->writersByTopic.begin(); it != mpImpl->writersByTopic.end(); ++it) {
        if (isValidEntity(it.value())) {
            dds_delete(it.value());
        }
    }
    mpImpl->writersByTopic.clear();

    for (auto it = mpImpl->topicsByName.begin(); it != mpImpl->topicsByName.end(); ++it) {
        if (isValidEntity(it.value())) {
            dds_delete(it.value());
        }
    }
    mpImpl->topicsByName.clear();

    if (isValidEntity(mpImpl->publisher)) {
        dds_delete(mpImpl->publisher);
        mpImpl->publisher = 0;
    }
    if (isValidEntity(mpImpl->subscriber)) {
        dds_delete(mpImpl->subscriber);
        mpImpl->subscriber = 0;
    }
    if (isValidEntity(mpImpl->participant)) {
        dds_delete(mpImpl->participant);
        mpImpl->participant = 0;
    }
#endif

    if (mbInitialized) {
        DEBUG_LOG_INFO() << "[CycloneDDSBridge] Shutting down...";
        mbInitialized = false;
        DEBUG_LOG_INFO() << "[CycloneDDSBridge] Shutdown complete";
    }
}

bool CycloneDDSBridge::publishRaw(const QString& topicName, const QByteArray& payload)
{
    QMutexLocker locker(&mpImpl->mutex);

    if (!mbInitialized) {
        DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Not initialized - cannot publish";
        return false;
    }

    if (topicName.trimmed().isEmpty()) {
        DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Empty topic name - cannot publish";
        return false;
    }

#ifdef CYCLONEDDS_ENABLED
    dds_entity_t topic = mpImpl->topicsByName.value(topicName, 0);
    if (!isValidEntity(topic)) {
        QByteArray topicUtf8 = topicName.toUtf8();
        topic = dds_create_topic(mpImpl->participant,
                                 &cvdev_RawPayload_desc,
                                 topicUtf8.constData(),
                                 nullptr,
                                 nullptr);
        if (!isValidEntity(topic)) {
            DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Failed to create topic for publish:" << topicName << "rc:" << topic;
            return false;
        }
        mpImpl->topicsByName.insert(topicName, topic);
    }

    dds_entity_t writer = mpImpl->writersByTopic.value(topicName, 0);
    if (!isValidEntity(writer)) {
        dds_qos_t *qos = dds_create_qos();
        dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(100));
        writer = dds_create_writer(mpImpl->publisher, topic, qos, nullptr);
        dds_delete_qos(qos);
        if (!isValidEntity(writer)) {
            DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Failed to create writer for topic:" << topicName << "rc:" << writer;
            return false;
        }
        mpImpl->writersByTopic.insert(topicName, writer);
    }

    cvdev_RawPayload sample{};
    sample.payload._length = static_cast<uint32_t>(payload.size());
    sample.payload._maximum = static_cast<uint32_t>(payload.size());
    sample.payload._buffer = reinterpret_cast<uint8_t*>(const_cast<char*>(payload.constData()));
    sample.payload._release = false;

    const dds_return_t rc = dds_write(writer, &sample);
    if (rc < 0) {
        DEBUG_LOG_WARNING() << "[CycloneDDSBridge] dds_write failed for topic:" << topicName << "rc:" << rc;
        return false;
    }

    DEBUG_LOG_INFO() << "[CycloneDDSBridge] Published to topic:" << topicName << "bytes:" << payload.size();
    return true;
#else
    Q_UNUSED(payload);
    return false;
#endif
}

bool CycloneDDSBridge::subscribeRaw(const QString& topicName, RawDataCallback callback)
{
    return subscribeRaw(topicName, QStringLiteral("__default__"), callback);
}

bool CycloneDDSBridge::subscribeRaw(const QString& topicName, const QString& subscriberId, RawDataCallback callback)
{
    QMutexLocker locker(&mpImpl->mutex);

    if (!mbInitialized) {
        DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Not initialized - cannot subscribe";
        return false;
    }

    if (topicName.trimmed().isEmpty() || subscriberId.trimmed().isEmpty() || !callback) {
        DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Invalid topic or null callback - cannot subscribe";
        return false;
    }

    auto existing = mpImpl->callbacksByTopic.find(topicName);
    if (existing != mpImpl->callbacksByTopic.end()) {
        if (existing->contains(subscriberId)) {
            DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Duplicate raw subscriber id for topic:" << topicName << subscriberId;
            return false;
        }

        existing->insert(subscriberId, callback);
        DEBUG_LOG_INFO() << "[CycloneDDSBridge] Added shared raw subscriber for topic:" << topicName << subscriberId;
        return true;
    }

#ifdef CYCLONEDDS_ENABLED
    dds_entity_t topic = mpImpl->topicsByName.value(topicName, 0);
    if (!isValidEntity(topic)) {
        QByteArray topicUtf8 = topicName.toUtf8();
        topic = dds_create_topic(mpImpl->participant,
                                 &cvdev_RawPayload_desc,
                                 topicUtf8.constData(),
                                 nullptr,
                                 nullptr);
        if (!isValidEntity(topic)) {
            DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Failed to create topic for subscribe:" << topicName << "rc:" << topic;
            return false;
        }
        mpImpl->topicsByName.insert(topicName, topic);
    }

    dds_qos_t *qos = dds_create_qos();
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(100));
    dds_entity_t reader = dds_create_reader(mpImpl->subscriber, topic, qos, nullptr);
    dds_delete_qos(qos);

    if (!isValidEntity(reader)) {
        DEBUG_LOG_WARNING() << "[CycloneDDSBridge] Failed to create reader for topic:" << topicName << "rc:" << reader;
        return false;
    }

    mpImpl->readersByTopic.insert(topicName, reader);
    mpImpl->topicByReader.insert(reader, topicName);
#endif

    mpImpl->callbacksByTopic[topicName].insert(subscriberId, callback);
    DEBUG_LOG_INFO() << "[CycloneDDSBridge] Subscribed to topic:" << topicName;
    return true;
}

void CycloneDDSBridge::unsubscribeRaw(const QString& topicName)
{
    unsubscribeRaw(topicName, QStringLiteral("__default__"));
}

void CycloneDDSBridge::unsubscribeRaw(const QString& topicName, const QString& subscriberId)
{
    QMutexLocker locker(&mpImpl->mutex);

    bool removeReader = false;
    auto callbacksIt = mpImpl->callbacksByTopic.find(topicName);
    if (callbacksIt != mpImpl->callbacksByTopic.end()) {
        callbacksIt->remove(subscriberId);
        if (callbacksIt->isEmpty()) {
            mpImpl->callbacksByTopic.erase(callbacksIt);
            removeReader = true;
        }
    }

#ifdef CYCLONEDDS_ENABLED
    if (removeReader && mpImpl->readersByTopic.contains(topicName)) {
        const dds_entity_t reader = mpImpl->readersByTopic.take(topicName);
        mpImpl->topicByReader.remove(reader);
        if (isValidEntity(reader)) {
            dds_delete(reader);
        }
    }
#endif

    if (!topicName.isEmpty()) {
        DEBUG_LOG_INFO() << "[CycloneDDSBridge] Unsubscribed from topic:" << topicName;
    }
}

void CycloneDDSBridge::handleReaderData(int readerEntity)
{
#ifdef CYCLONEDDS_ENABLED
    QMutexLocker locker(&mpImpl->mutex);

    const dds_entity_t reader = static_cast<dds_entity_t>(readerEntity);
    const QString topicName = mpImpl->topicByReader.value(reader);
    if (topicName.isEmpty()) {
        return;
    }

    const auto callbackIt = mpImpl->callbacksByTopic.constFind(topicName);
    if (callbackIt == mpImpl->callbacksByTopic.constEnd()) {
        return;
    }

    const QList<RawDataCallback> callbacks = callbackIt.value().values();
    if (callbacks.isEmpty()) {
        return;
    }

    constexpr size_t kMaxSamples = 16;
    cvdev_RawPayload samples[kMaxSamples]{};
    void *samplePtrs[kMaxSamples];
    dds_sample_info_t sampleInfos[kMaxSamples]{};

    for (size_t i = 0; i < kMaxSamples; ++i) {
        samplePtrs[i] = &samples[i];
    }

    const dds_return_t taken = dds_take(reader, samplePtrs, sampleInfos, kMaxSamples, static_cast<uint32_t>(kMaxSamples));
    if (taken <= 0) {
        return;
    }

    for (dds_return_t i = 0; i < taken; ++i) {
        if (!sampleInfos[i].valid_data || !samplePtrs[i]) {
            continue;
        }

        auto *sample = static_cast<cvdev_RawPayload*>(samplePtrs[i]);
        const char *bytes = reinterpret_cast<const char*>(sample->payload._buffer);
        QByteArray payload = (bytes && sample->payload._length > 0)
            ? QByteArray(bytes, static_cast<int>(sample->payload._length))
            : QByteArray();

        QObject *dispatchTarget = QCoreApplication::instance();
        if (dispatchTarget) {
            QMetaObject::invokeMethod(dispatchTarget, [callbacks, payload]() {
                for (const RawDataCallback& callback : callbacks) {
                    if (callback) {
                        callback(payload);
                    }
                }
            }, Qt::QueuedConnection);
        } else {
            for (const RawDataCallback& callback : callbacks) {
                if (callback) {
                    callback(payload);
                }
            }
        }

        dds_sample_free(sample, &cvdev_RawPayload_desc, DDS_FREE_CONTENTS);
    }
#else
    Q_UNUSED(readerEntity);
#endif
}

bool CycloneDDSBridge::isInitialized() const
{
    return mbInitialized;
}

void CycloneDDSBridge::setComputerId(const QString& computerId)
{
    const QString trimmed = computerId.trimmed();
    msComputerId = trimmed.isEmpty() ? QSysInfo::machineHostName() : trimmed;
}

QString CycloneDDSBridge::getComputerId() const
{
    return msComputerId;
}
