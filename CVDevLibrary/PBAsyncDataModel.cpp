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

#include "PBAsyncDataModel.hpp"
#include "Property.hpp"
#include "qtvariantproperty_p.h"
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QTimer>

PBAsyncDataModel::PBAsyncDataModel(const QString& modelName)
    : PBNodeDelegateModel(modelName)
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpCVImageInData = std::make_shared< CVImageData >( cv::Mat() );
    // Inherited model needs to set a mpSyncData data true or false itself later.
    // Don't forget to implement proper handling in when starting and 
    // fisnishing processing input data.
    mpSyncData = std::make_shared< SyncData >();

    qRegisterMetaType<std::shared_ptr<CVImageData>>("std::shared_ptr<CVImageData>");
    qRegisterMetaType<std::shared_ptr<CVImagePool>>("std::shared_ptr<CVImagePool>");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<FrameSharingMode>("FrameSharingMode");
    // Sharing mode property
    EnumPropertyType sharingModeProperty;
    sharingModeProperty.mslEnumNames = { "Pool Mode", "Broadcast Mode" };
    sharingModeProperty.miCurrentIndex = ( meSharingMode == FrameSharingMode::PoolMode ) ? 0 : 1;
    QString propId = "sharing_mode";
    auto propSharingMode = std::make_shared< TypedProperty< EnumPropertyType > >( 
        "Sharing Mode", propId, QtVariantPropertyManager::enumTypeId(), sharingModeProperty, "Image Memory" );
    mvProperty.push_back( propSharingMode );
    mMapIdToProperty[ propId ] = propSharingMode;

    // Pool size property
    IntPropertyType poolSizeProperty;
    poolSizeProperty.miMin = 1;
    poolSizeProperty.miMax = 128;
    poolSizeProperty.miValue = miPoolSize;
    propId = "pool_size";
    auto propPoolSize = std::make_shared< TypedProperty< IntPropertyType > >( 
        "Pool Size", propId, QMetaType::Int, poolSizeProperty, "Image Memory" );
    mvProperty.push_back( propPoolSize );
    mMapIdToProperty[ propId ] = propPoolSize;
}

PBAsyncDataModel::~PBAsyncDataModel()
{
    // Set shutdown flag
    mShuttingDown.store(true, std::memory_order_release);
    
    if (mpWorker)
    {
        // Disconnect all signals from worker to prevent callbacks during destruction
        disconnect(mpWorker, nullptr, this, nullptr);
    }
    
    // Request thread termination
    mWorkerThread.quit();
    
    // Wait with timeout
    if (!mWorkerThread.wait(3000))
    {
        // Force terminate if graceful shutdown fails
        mWorkerThread.terminate();
        mWorkerThread.wait();
    }
}

void PBAsyncDataModel::late_constructor()
{
    if( !start_late_constructor() )
        return;
    
    // Create worker via derived class factory method
    mpWorker = createWorker();
    
    if (!mpWorker)
        return;
    
    // Move to thread
    mpWorker->moveToThread(&mWorkerThread);
    
    // Connect signals via derived class
    connectWorker(mpWorker);
    
    // Start thread
    mWorkerThread.start();
}

void PBAsyncDataModel::setModelProperty(QString& id, const QVariant& value)
{
    if (!mMapIdToProperty.contains(id))
        return;
    
    PBNodeDelegateModel::setModelProperty(id, value);
    
    if (id == "pool_size")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
        int newSize = qMax(1, qMin(128, value.toInt()));
        if (miPoolSize == newSize)
            return;

        typedProp->getData().miValue = newSize;
        miPoolSize = newSize;
        reset_frame_pool();
    }
    else if (id == "sharing_mode")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >(prop);
        int newIndex = qBound(0, value.toInt(), 1);
        FrameSharingMode newMode = (newIndex == 0) ? FrameSharingMode::PoolMode : FrameSharingMode::BroadcastMode;
        if (meSharingMode == newMode)
            return;
        
        typedProp->getData().miCurrentIndex = newIndex;
        meSharingMode = newMode;
        {
            QMutexLocker locker(&mFramePoolMutex);
            if (mpFramePool)
                mpFramePool->setMode(meSharingMode);
        }
        if (meSharingMode != FrameSharingMode::PoolMode)
            reset_frame_pool();
    }
}

void PBAsyncDataModel::ensure_frame_pool(int width, int height, int type)
{
    if (width <= 0 || height <= 0)
        return;

    const int desiredSize = qMax(1, miPoolSize);
    QMutexLocker locker(&mFramePoolMutex);
    const bool shouldRecreate = !mpFramePool ||
        miPoolFrameWidth != width ||
        miPoolFrameHeight != height ||
        miActivePoolSize != desiredSize;

    if (shouldRecreate)
    {
        mpFramePool = std::make_shared<CVImagePool>(
            getNodeId(), width, height, type, static_cast<size_t>(desiredSize));
        miPoolFrameWidth = width;
        miPoolFrameHeight = height;
        miActivePoolSize = desiredSize;
    }

    if (mpFramePool)
        mpFramePool->setMode(meSharingMode);
}

void PBAsyncDataModel::reset_frame_pool()
{
    QMutexLocker locker(&mFramePoolMutex);
    mpFramePool.reset();
    miPoolFrameWidth = 0;
    miPoolFrameHeight = 0;
    miActivePoolSize = 0;
}

std::shared_ptr<CVImagePool> PBAsyncDataModel::getFramePool()
{
    QMutexLocker locker(&mFramePoolMutex);
    return mpFramePool;
}

void PBAsyncDataModel::onWorkCompleted()
{
    mWorkerBusy = false;
    if (mHasPending)
        dispatchPendingWork();
}

void PBAsyncDataModel::handleFrameReady(std::shared_ptr<CVImageData> img)
{
    if (isShuttingDown()) {
        setWorkerBusy(false);
        return;
    }
    if (img) {
        mpCVImageData = img;
        Q_EMIT dataUpdated(0);
        QTimer::singleShot(0, this, [this]() {
            mpSyncData->data() = true;
            Q_EMIT dataUpdated(1);
        });
    }
    onWorkCompleted();
}

QJsonObject PBAsyncDataModel::save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["pool_size"] = miPoolSize;
    cParams["sharing_mode"] = (meSharingMode == FrameSharingMode::PoolMode) ? "pool" : "broadcast";
    modelJson["cParams"] = cParams;
    return modelJson;
}

void PBAsyncDataModel::load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);
    
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v = paramsObj["pool_size"];
        if (!v.isUndefined())
        {
            auto prop = mMapIdToProperty["pool_size"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            miPoolSize = v.toInt();
        }
        
        v = paramsObj["sharing_mode"];
        if (!v.isUndefined())
        {
            QString mode = v.toString();
            meSharingMode = (mode == "pool") ? FrameSharingMode::PoolMode : FrameSharingMode::BroadcastMode;
            auto prop = mMapIdToProperty["sharing_mode"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = (meSharingMode == FrameSharingMode::PoolMode) ? 0 : 1;
        }
    }
}

void PBAsyncDataModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (isShuttingDown()) {
        return;
    }

    if (portIndex == 0) {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d) {
            mpCVImageInData = d;
            if (!mbUseSyncSignal) {
                process_cached_input();
            }
        }
    } else if (portIndex == 1) {
        // Sync input acts purely as a trigger. We do NOT adopt the incoming
        // SyncData instance (mpSyncData is our dedicated output signal object).
        // When a 'true' sync arrives and we have a cached image, we start
        // processing. A 'false' sync is ignored.
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
        if (d && d->data()) {
            if ( mpCVImageInData && !mpCVImageInData->data().empty() ) {
                process_cached_input();
            }
        }
    }
}

std::shared_ptr<NodeData> PBAsyncDataModel::outData(PortIndex port)
{
    if (port == 0 && mpCVImageData) {
        return mpCVImageData;
    } else if (port == 1 && mpSyncData) {
        return mpSyncData;
    }
    return nullptr;
}

unsigned int PBAsyncDataModel::nPorts(PortType portType) const
{
    switch (portType) {
    case PortType::In:
        return 2; // image + sync signal
    case PortType::Out:
        return 2; // image + sync signal
    default:
        return 0;
    }
}

NodeDataType PBAsyncDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In) {
        if (portIndex == 0) {
            return CVImageData().type();
        } else if (portIndex == 1) {
            return SyncData().type();
        }
    } else if (portType == PortType::Out) {
        if (portIndex == 0) {
            return CVImageData().type();
        } else if (portIndex == 1) {
            return SyncData().type();
        }
    }
    return NodeDataType();
}

// Default implementations for derived classes to override
QObject* PBAsyncDataModel::createWorker()
{
    return nullptr;
}

void PBAsyncDataModel::connectWorker(QObject*)
{
    // Default: do nothing
}

void PBAsyncDataModel::dispatchPendingWork()
{
    // Default: do nothing
}

void PBAsyncDataModel::process_cached_input()
{
    // Default: do nothing
}  

void PBAsyncDataModel::inputConnectionCreated(QtNodes::ConnectionId const& connection)
{
    if (QtNodes::getPortIndex(PortType::In, connection) == 1) {
        mbUseSyncSignal = true;
    }
}

void PBAsyncDataModel::inputConnectionDeleted(QtNodes::ConnectionId const& connection)
{
    if (QtNodes::getPortIndex(PortType::In, connection) == 0) {
        mpCVImageInData.reset();
    } else if (QtNodes::getPortIndex(PortType::In, connection) == 1) {
        // Do not reset mpSyncData to nullptr here; deferred UI updates
        // (e.g., QTimer::singleShot lambdas) may still write to the sync
        // data immediately after a connection change. Keeping a valid
        // SyncData instance prevents dereferencing a null pointer.
        // Simply disable sync mode.
        mbUseSyncSignal = false;
    }
}