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

#include "CVRGBtoGrayModel.hpp"


#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QDebug>
#include "qtvariantproperty_p.h"

const QString CVRGBtoGrayModel::_category = QString("Image Conversion");

const QString CVRGBtoGrayModel::_model_name = QString( "CV RGB to Gray" );

void CVRGBtoGrayWorker::processFrame(cv::Mat input,
                                      FrameSharingMode mode,
                                      std::shared_ptr<CVImagePool> pool,
                                      long frameId,
                                      QString producerId)
{
    if(input.empty() || input.type() != CV_8UC3)
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
            cv::cvtColor(input, handle.matrix(), cv::COLOR_BGR2GRAY);
            if(!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if(!pooled)
    {
        cv::Mat result;
        cv::cvtColor(input, result, cv::COLOR_BGR2GRAY);
        if(result.empty())
        {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}

CVRGBtoGrayModel::
CVRGBtoGrayModel()
    : PBAsyncDataModel( _model_name ),
      _minPixmap( ":RGBtoGray.png" )
{

}

QObject*
CVRGBtoGrayModel::
createWorker()
{
    return new CVRGBtoGrayWorker();
}

void
CVRGBtoGrayModel::
connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVRGBtoGrayWorker*>(worker);
    if (w) {
        connect(w, &CVRGBtoGrayWorker::frameReady,
            this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void
CVRGBtoGrayModel::
dispatchPendingWork()
{
    if(!hasPendingWork() || isShuttingDown())
        return;
    
    cv::Mat input = mPendingFrame;
    setPendingWork(false);
    
    ensure_frame_pool(input.cols, input.rows, CV_8UC1);
    
    long frameId = getNextFrameId();
    QString producerId = getNodeId();
    
    std::shared_ptr<CVImagePool> poolCopy = getFramePool();
    
    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
        Qt::QueuedConnection,
        Q_ARG(cv::Mat, input.clone()),
        Q_ARG(FrameSharingMode, getSharingMode()),
        Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
        Q_ARG(long, frameId),
        Q_ARG(QString, producerId));
}

void
CVRGBtoGrayModel::
process_cached_input()
{
    if( !mpCVImageInData || mpCVImageInData->data().empty() )
        return;
    
    cv::Mat input = mpCVImageInData->data();
    
    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });
    
    if(isWorkerBusy())
    {
        mPendingFrame = input.clone();
        setPendingWork(true);
    }
    else
    {
        setWorkerBusy(true);
        
        ensure_frame_pool(input.cols, input.rows, CV_8UC1);
        
        long frameId = getNextFrameId();
        QString producerId = getNodeId();
        
        std::shared_ptr<CVImagePool> poolCopy = getFramePool();
        
        QMetaObject::invokeMethod(mpWorker, "processFrame",
            Qt::QueuedConnection,
            Q_ARG(cv::Mat, input.clone()),
            Q_ARG(FrameSharingMode, getSharingMode()),
            Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
            Q_ARG(long, frameId),
            Q_ARG(QString, producerId));
    }
}
