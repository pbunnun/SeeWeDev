// Copyright © 2025, NECTEC, all rights reserved

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "CVCannyEdgeModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QDebug>
#include <QtNodes/internal/ConnectionIdUtils.hpp>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVCannyEdgeModel::_category = QString("Image Conversion");

const QString CVCannyEdgeModel::_model_name = QString("CV Canny Edge");

void CVCannyEdgeWorker::
    processFrame(cv::Mat input,
                 int thresholdL,
                 int thresholdU,
                 int kernelSize,
                 bool enableGradient,
                 FrameSharingMode mode,
                 std::shared_ptr<CVImagePool> pool,
                 long frameId,
                 QString producerId)
{
    if (input.empty() || (input.depth() != CV_8U && input.depth() != CV_8S))
    {
        Q_EMIT frameReady(nullptr);
        return;
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    if (mode == FrameSharingMode::PoolMode && pool)
    {
        auto handle = pool->acquire(1, metadata);
        if (handle)
        {
            // Write directly to pool buffer - zero extra allocation
            cv::Canny(input, handle.matrix(), thresholdL, thresholdU, kernelSize, enableGradient);
            if(!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if (!pooled)
    {
        cv::Mat result;
        cv::Canny(input, result, thresholdL, thresholdU, kernelSize, enableGradient);
        if(result.empty())
        {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}

CVCannyEdgeModel::
    CVCannyEdgeModel()
    : PBAsyncDataModel(_model_name),
      _minPixmap(":CVCannyEdge.png")
{
    IntPropertyType intPropertyType;
    QString propId = "kernel_size";
    intPropertyType.miValue = mParams.miSizeKernel;
    auto propKernelSize = std::make_shared<TypedProperty<IntPropertyType>>("Kernel Size", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propKernelSize);
    mMapIdToProperty[propId] = propKernelSize;

    intPropertyType.miValue = mParams.miThresholdU;
    intPropertyType.miMax = 255;
    propId = "th_u";
    auto propThresholdU = std::make_shared<TypedProperty<IntPropertyType>>("Upper Threshold", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propThresholdU);
    mMapIdToProperty[propId] = propThresholdU;

    intPropertyType.miValue = mParams.miThresholdL;
    propId = "th_l";
    auto propThresholdL = std::make_shared<TypedProperty<IntPropertyType>>("Lower Threshold", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propThresholdL);
    mMapIdToProperty[propId] = propThresholdL;

    propId = "enable_gradient";
    auto propEnableGradient = std::make_shared<TypedProperty<bool>>("Use Edge Gradient", propId, QMetaType::Bool, mParams.mbEnableGradient, "Operation");
    mvProperty.push_back(propEnableGradient);
    mMapIdToProperty[propId] = propEnableGradient;
}

QObject*
CVCannyEdgeModel::
createWorker()
{
    return new CVCannyEdgeWorker();
}

void
CVCannyEdgeModel::
connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVCannyEdgeWorker*>(worker);
    if (w) {
        connect(w, &CVCannyEdgeWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void
CVCannyEdgeModel::
dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;

    cv::Mat input = mPendingFrame;
    CVCannyEdgeParameters params = mPendingParams;
    setPendingWork(false);

    ensure_frame_pool(input.cols, input.rows, input.type());

    long frameId = getNextFrameId();
    QString producerId = getNodeId();

    std::shared_ptr<CVImagePool> poolCopy = getFramePool();

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(int, params.miThresholdL),
                              Q_ARG(int, params.miThresholdU),
                              Q_ARG(int, params.miSizeKernel),
                              Q_ARG(bool, params.mbEnableGradient),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

QJsonObject
CVCannyEdgeModel::
    save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["kernelSize"] = mParams.miSizeKernel;
    cParams["thresholdU"] = mParams.miThresholdU;
    cParams["thresholdL"] = mParams.miThresholdL;
    cParams["enableGradient"] = mParams.mbEnableGradient;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void CVCannyEdgeModel::
    load(QJsonObject const &p)
{
    PBAsyncDataModel::load(p);

    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v = paramsObj["kernelSize"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["kernel_size"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miSizeKernel = v.toInt();
        }
        v = paramsObj["thresholdU"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["th_u"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miThresholdU = v.toInt();
        }
        v = paramsObj["thresholdL"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["th_l"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miThresholdL = v.toInt();
        }
        v = paramsObj["enableGradient"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["enable_gradient"];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mParams.mbEnableGradient = v.toBool();
        }
    }
}

void CVCannyEdgeModel::
    setModelProperty(QString &id, const QVariant &value)
{
    if (!mMapIdToProperty.contains(id))
        return;

    if (id == "kernel_size")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        int kSize = value.toInt();
        bool adjValue = false;
        if (kSize > 7)
        {
            kSize = 7;
            adjValue = true;
        }
        if (kSize < 3)
        {
            kSize = 3;
            adjValue = true;
        }
        if (kSize >= 3 && kSize <= 7 && kSize % 2 != 1)
        {
            kSize += 1;
            adjValue = true;
        }
        typedProp->getData().miValue = kSize;
        if (adjValue)
        {
            // Notify listeners that the entered kernel size was coerced
            // (made odd). Downstream UI will refresh to the corrected
            // value via this signal.
            Q_EMIT property_changed_signal(prop);
        }
        mParams.miSizeKernel = kSize;
    }
    else if (id == "th_u")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();

        mParams.miThresholdU = value.toInt();
    }
    else if (id == "th_l")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();

        mParams.miThresholdL = value.toInt();
    }
    else if (id == "enable_gradient")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typedProp->getData() = value.toBool();

        mParams.mbEnableGradient = value.toBool();
    }
    else
    {
        // Base class handles pool_size and sharing_mode
        // Need to call base class to handle pool_size and sharing_mode
        PBAsyncDataModel::setModelProperty(id, value);
        // No need to process_cached_input() here
        return;
    }
    // Process cached input if available
    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}

void CVCannyEdgeModel::
    process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;
    
    cv::Mat input = mpCVImageInData->data();
    
    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });
    
    if (isWorkerBusy())
    {
        // Store as pending - will be processed when worker finishes
        mPendingFrame = input.clone();
        mPendingParams = mParams;
        setPendingWork(true);
    }
    else
    {
        setWorkerBusy(true);
        
        ensure_frame_pool(input.cols, input.rows, input.type());
        
        long frameId = getNextFrameId();
        QString producerId = getNodeId();
        
        std::shared_ptr<CVImagePool> poolCopy = getFramePool();
        
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(int, mParams.miThresholdL),
                                  Q_ARG(int, mParams.miThresholdU),
                                  Q_ARG(int, mParams.miSizeKernel),
                                  Q_ARG(bool, mParams.mbEnableGradient),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}
