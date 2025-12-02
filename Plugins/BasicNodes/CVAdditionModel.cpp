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

#include "CVAdditionModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QDebug>

#include <QtWidgets/QFileDialog>
#include <QtNodes/internal/ConnectionIdUtils.hpp>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "qtvariantproperty_p.h"

void CVAdditionWorker::processFrames(cv::Mat a,
                                      cv::Mat b,
                                      cv::Mat mask,
                                      bool maskActive,
                                      FrameSharingMode mode,
                                      std::shared_ptr<CVImagePool> pool,
                                      long frameId,
                                      QString producerId)
{
    if(a.empty() || b.empty() || a.type() != b.type())
    {
        Q_EMIT frameReady(nullptr);
        return;
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    if(mode == FrameSharingMode::PoolMode && pool)
    {
        auto handle = pool->acquire(1, metadata);
        if(handle)
        {
            // Write directly to pool buffer - zero extra allocation
            if(maskActive && !mask.empty() && mask.type() == CV_8UC1)
                cv::add(a, b, handle.matrix(), mask);
            else
                cv::add(a, b, handle.matrix());
            
            if(!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if(!pooled)
    {
        cv::Mat result;
        if(maskActive && !mask.empty() && mask.type() == CV_8UC1)
            cv::add(a, b, result, mask);
        else
            cv::add(a, b, result);
        
        if(result.empty())
        {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}

const QString CVAdditionModel::_category = QString( "Image Operation" );

const QString CVAdditionModel::_model_name = QString( "CV Addition" );

CVAdditionModel::
CVAdditionModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":Addition.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mpSyncData = std::make_shared<SyncData>();
    mvCVImageInData.resize(3);
    mPendingFrames.resize(3);
    qRegisterMetaType<std::shared_ptr<CVImageData>>("std::shared_ptr<CVImageData>");
    qRegisterMetaType<std::shared_ptr<CVImagePool>>("std::shared_ptr<CVImagePool>");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<FrameSharingMode>("FrameSharingMode");

    // Pool size property
    IntPropertyType poolSizeProperty;
    poolSizeProperty.miMin = 1;
    poolSizeProperty.miMax = 128;
    poolSizeProperty.miValue = miPoolSize;
    QString propId = "pool_size";
    auto propPoolSize = std::make_shared< TypedProperty< IntPropertyType > >( "Pool Size", propId, QMetaType::Int, poolSizeProperty );
    mvProperty.push_back( propPoolSize );
    mMapIdToProperty[ propId ] = propPoolSize;

    // Sharing mode property
    EnumPropertyType sharingModeProperty;
    sharingModeProperty.mslEnumNames = { "Pool Mode", "Broadcast Mode" };
    sharingModeProperty.miCurrentIndex = ( meSharingMode == FrameSharingMode::PoolMode ) ? 0 : 1;
    propId = "sharing_mode";
    auto propSharingMode = std::make_shared< TypedProperty< EnumPropertyType > >( "Sharing Mode", propId, QtVariantPropertyManager::enumTypeId(), sharingModeProperty );
    mvProperty.push_back( propSharingMode );
    mMapIdToProperty[ propId ] = propSharingMode;
}

unsigned int
CVAdditionModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 4;  // 2 images + mask + sync
        break;

    case PortType::Out:
        result = 2;  // result + sync
        break;

    default:
        break;
    }

    return result;
}

void
CVAdditionModel::
late_constructor()
{
    // Initialize worker thread at startup
    if(start_late_constructor())
    {
        mpWorker = new CVAdditionWorker();
        mpWorker->moveToThread(&mWorkerThread);
        connect(mpWorker, &CVAdditionWorker::frameReady,
                this, &CVAdditionModel::handleFrameReady,
                Qt::QueuedConnection);
        mWorkerThread.start();
    }
}

NodeDataType
CVAdditionModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if(portType == PortType::In && portIndex == 3)
        return SyncData().type();
    else if(portType == PortType::Out && portIndex == 1)
        return SyncData().type();
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVAdditionModel::
outData(PortIndex port)
{
    if( isEnable() )
    {
        if( port == 0 && mpCVImageData->data().empty() == false )
            return mpCVImageData;
        else if( port == 1 )
            return mpSyncData;
    }
    return nullptr;
}

void
CVAdditionModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (!nodeData)
        return;

    if (portIndex < 3)  // Image inputs (0, 1, 2)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            const cv::Mat& frame = d->data();
            frame.copyTo( mvCVImageInData[portIndex] );
            
            // In sync mode, wait for sync signal before processing
            if (mbUseSyncSignal)
                return;
            
            // Non-sync mode: process immediately if inputs ready
            if( !mvCVImageInData[0].empty() && !mvCVImageInData[1].empty() &&
                ( !mbMaskActive || !mvCVImageInData[2].empty() ) )
            {
                process_cached_input();
            }
        }
    }
    else if (portIndex == 3)  // Sync signal input
    {
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
        if (d && d->data())
        {
            // Sync signal received - process cached inputs if available
            if( !mvCVImageInData[0].empty() && !mvCVImageInData[1].empty() &&
                ( !mbMaskActive || !mvCVImageInData[2].empty() ) )
            {
                process_cached_input();
            }
        }
    }
}

QJsonObject
CVAdditionModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["pool_size"] = miPoolSize;
    cParams["sharing_mode"] = (meSharingMode == FrameSharingMode::PoolMode) ? 0 : 1;

    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVAdditionModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        if( paramsObj.contains("pool_size") )
        {
            auto prop = mMapIdToProperty["pool_size"];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            int newSize = qMax( 1, qMin(128, paramsObj["pool_size"].toInt()) );
            typedProp->getData().miValue = newSize;

            miPoolSize = newSize;
        }

        if( paramsObj.contains("sharing_mode") )
        {
            auto prop = mMapIdToProperty["sharing_mode"];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            int mode = paramsObj["sharing_mode"].toInt();
            typedProp->getData().miCurrentIndex = ( mode == 0 ) ? 0 : 1;

            meSharingMode = (mode == 0) ? FrameSharingMode::PoolMode : FrameSharingMode::BroadcastMode;
        }
    }
}

void
CVAdditionModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;
    
    auto prop = mMapIdToProperty[id];
    if( id == "pool_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        int newSize = qMax( 1, qMin(128, value.toInt()) );
        if( miPoolSize == newSize )
            return;

        typedProp->getData().miValue = newSize;
        miPoolSize = newSize;
        reset_frame_pool();
        ensure_frame_pool(0, 0, 0); // Will recreate on next use
    }
    else if( id == "sharing_mode" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        int enumCount = static_cast<int>( typedProp->getData().mslEnumNames.size() );
        if( enumCount <= 0 )
            enumCount = 1; // Prevent division by zero
        int newIndex = qBound( 0, value.toInt(), enumCount - 1 );
        FrameSharingMode newMode = ( newIndex == 0 ) ? FrameSharingMode::PoolMode : FrameSharingMode::BroadcastMode;
        if( meSharingMode == newMode && typedProp->getData().miCurrentIndex == newIndex )
            return;
        
        typedProp->getData().miCurrentIndex = newIndex;
        meSharingMode = newMode;
        {
            QMutexLocker locker( &mFramePoolMutex );
            if(mpFramePool)
                mpFramePool->setMode( meSharingMode );
        }
        if( meSharingMode == FrameSharingMode::PoolMode )
            ensure_frame_pool(0, 0, 0); // Will recreate on next use
    }
}

void
CVAdditionModel::
processData(const std::vector< cv::Mat >&in, std::shared_ptr<CVImageData> &out)
{
    if(in[0].type() != in[1].type())
    {
        return;
    }

    cv::Mat result;
    if(mbMaskActive && !in[2].empty() && in[2].type() == CV_8UC1)
    {
        cv::add(in[0], in[1], result, in[2]);
    }
    else
    {
        cv::add(in[0], in[1], result);
    }
    
    if( !result.empty() )
    {
        // Prepare metadata
        FrameMetadata metadata;
        metadata.producerId = getNodeId();
        metadata.frameId = mFrameCounter++;

        // Create new CVImageData for this frame
        auto newImageData = std::make_shared<CVImageData>(cv::Mat());

        bool pooled = false;
        if( meSharingMode == FrameSharingMode::PoolMode )
        {
            ensure_frame_pool( result.cols, result.rows, result.type() );
            std::shared_ptr<CVImagePool> poolCopy;
            {
                QMutexLocker locker( &mFramePoolMutex );
                poolCopy = mpFramePool;
            }
            if( poolCopy )
            {
                auto metadataForPool = metadata;
                auto handle = poolCopy->acquire( 1, std::move( metadataForPool ) );
                if( handle )
                {
                    result.copyTo( handle.matrix() );
                    if( newImageData->adoptPoolFrame( std::move( handle ) ) )
                        pooled = true;
                }
            }
        }

        if( !pooled )
        {
            newImageData->updateMove( std::move( result ), metadata );
        }

        out = std::move(newImageData);
        Q_EMIT dataUpdated(0);
    }
}

void
CVAdditionModel::
inputConnectionCreated(QtNodes::ConnectionId const& conx)
{
    auto idx = QtNodes::getPortIndex(PortType::In, conx);
    if( idx == 2 )
        mbMaskActive = true;
    else if( idx == 3 )
        mbUseSyncSignal = true;
}

void
CVAdditionModel::
inputConnectionDeleted(QtNodes::ConnectionId const& conx)
{
    auto idx = QtNodes::getPortIndex(PortType::In, conx);
    if(idx < 3)
        mvCVImageInData[idx].release();
    if( idx == 2 )
    {
        mbMaskActive = false;
        if( !mvCVImageInData[0].empty() && !mvCVImageInData[1].empty() )
        {
            processData(mvCVImageInData, mpCVImageData);
        }
    }
    else if( idx == 3 )
        mbUseSyncSignal = false;
}

void
CVAdditionModel::
ensure_frame_pool(int width, int height, int type)
{
    if( width <= 0 || height <= 0 )
        return;

    const int desiredSize = qMax( 1, miPoolSize );
    QMutexLocker locker( &mFramePoolMutex );
    const bool shouldRecreate = !mpFramePool ||
        miPoolFrameWidth != width ||
        miPoolFrameHeight != height ||
        miActivePoolSize != desiredSize;

    if( shouldRecreate )
    {
        mpFramePool = std::make_shared<CVImagePool>( getNodeId(), width, height, type, static_cast<size_t>( desiredSize ) );
        miPoolFrameWidth = width;
        miPoolFrameHeight = height;
        miActivePoolSize = desiredSize;
    }

    if( mpFramePool )
        mpFramePool->setMode( meSharingMode );
}

void
CVAdditionModel::
reset_frame_pool()
{
    QMutexLocker locker( &mFramePoolMutex );
    mpFramePool.reset();
    miPoolFrameWidth = 0;
    miPoolFrameHeight = 0;
    miActivePoolSize = 0;
}

void CVAdditionModel::process_cached_input()
{
    if(mShuttingDown.load(std::memory_order_acquire))
        return;
    
    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });
    
    ensure_frame_pool(mvCVImageInData[0].cols, mvCVImageInData[0].rows, mvCVImageInData[0].type());
    std::shared_ptr<CVImagePool> poolCopy;
    {
        QMutexLocker locker(&mFramePoolMutex);
        poolCopy = mpFramePool;
    }
    cv::Mat a = mvCVImageInData[0].clone();
    cv::Mat b = mvCVImageInData[1].clone();
    cv::Mat mask;
    if(mbMaskActive && !mvCVImageInData[2].empty())
        mask = mvCVImageInData[2].clone();
    long frameId = mFrameCounter++;
    if(mWorkerBusy)
    {
        mPendingFrames[0] = std::move(a);
        mPendingFrames[1] = std::move(b);
        mPendingFrames[2] = std::move(mask);
        mHasPending = true;
    }
    else
    {
        mWorkerBusy = true;
        QMetaObject::invokeMethod(mpWorker,
                                  "processFrames",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, a),
                                  Q_ARG(cv::Mat, b),
                                  Q_ARG(cv::Mat, mask),
                                  Q_ARG(bool, mbMaskActive),
                                  Q_ARG(FrameSharingMode, meSharingMode),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, getNodeId()));
    }
}

void CVAdditionModel::dispatch_pending()
{
    if(!mHasPending || !mpWorker || mShuttingDown.load(std::memory_order_acquire))
        return;
    std::shared_ptr<CVImagePool> poolCopy;
    {
        QMutexLocker locker(&mFramePoolMutex);
        poolCopy = mpFramePool;
    }
    cv::Mat a = mPendingFrames[0].clone();
    cv::Mat b = mPendingFrames[1].clone();
    cv::Mat mask = mPendingFrames[2].clone();
    mHasPending = false;
    long frameId = mFrameCounter++;
    mWorkerBusy = true;
    QMetaObject::invokeMethod(mpWorker,
                              "processFrames",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, a),
                              Q_ARG(cv::Mat, b),
                              Q_ARG(cv::Mat, mask),
                              Q_ARG(bool, mbMaskActive),
                              Q_ARG(FrameSharingMode, meSharingMode),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, getNodeId()));
}

void CVAdditionModel::handleFrameReady(std::shared_ptr<CVImageData> img)
{
    if(mShuttingDown.load(std::memory_order_acquire))
    {
        mWorkerBusy = false;
        return;
    }
    
    if(img)
    {
        mpCVImageData = img;
        Q_EMIT dataUpdated(0);
        QTimer::singleShot(0, this, [this]() {
            mpSyncData->data() = true;
            Q_EMIT dataUpdated(1);
        });
    }
    mWorkerBusy = false;
    if(mHasPending)
        dispatch_pending();
}

CVAdditionModel::~CVAdditionModel()
{
    mShuttingDown.store(true, std::memory_order_release);
    
    if(mpWorker)
    {
        // Prevent new work from being dispatched
        disconnect(mpWorker, nullptr, this, nullptr);
        
        // Request thread termination
        mWorkerThread.quit();
        
        // Wait with timeout to prevent indefinite hang
        if(!mWorkerThread.wait(3000))
        {
            // Force terminate if graceful shutdown fails
            mWorkerThread.terminate();
            mWorkerThread.wait();
        }
        
        // Delete worker object
        delete mpWorker;
        mpWorker = nullptr;
    }
}
