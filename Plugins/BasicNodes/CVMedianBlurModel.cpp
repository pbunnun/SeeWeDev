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

#include "CVMedianBlurModel.hpp"

#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>
#include <QTimer>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

// Static member definitions
const QString CVMedianBlurModel::_category = QString("Image Modification");
const QString CVMedianBlurModel::_model_name = QString("CV Median Blur");

void CVMedianBlurWorker::processFrame(cv::Mat input,
                                       CVMedianBlurParameters params,
                                       FrameSharingMode mode,
                                       std::shared_ptr<CVImagePool> pool,
                                       long frameId,
                                       QString producerId)
{
    if (input.empty()) {
        Q_EMIT frameReady(nullptr);
        return;
    }

    // Validate kernel size (must be odd and > 1)
    int ksize = params.miKernelSize;
    if (ksize < 3 || ksize % 2 == 0) {
        ksize = 3; // Default to smallest valid size
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;

    if (mode == FrameSharingMode::PoolMode && pool) {
        auto handle = pool->acquire(1, metadata);
        if (handle) {
            // Write directly to pool buffer - zero copy
            try {
                cv::medianBlur(input, handle.matrix(), ksize);
                if (!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                    pooled = true;
            } catch (const cv::Exception& e) {
                qWarning() << "CVMedianBlur error:" << e.what();
            }
        }
    }

    if (!pooled) {
        cv::Mat result;
        try {
            cv::medianBlur(input, result, ksize);
        } catch (const cv::Exception& e) {
            qWarning() << "CVMedianBlur error:" << e.what();
            Q_EMIT frameReady(nullptr);
            return;
        }

        if (result.empty()) {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }

    Q_EMIT frameReady(newImageData);
}

CVMedianBlurModel::CVMedianBlurModel()
    : PBAsyncDataModel("CV Median Blur"),
    _minPixmap(":/CVMedianBlurModel.png")
{
    // Add kernel size property (odd numbers only, 3-31)
    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miKernelSize;
    intPropertyType.miMin = 3;
    intPropertyType.miMax = 31;
    QString propId = "kernel_size";
    auto propKernelSize = std::make_shared<TypedProperty<IntPropertyType>>("Kernel Size", propId, QMetaType::Int, intPropertyType);
    mvProperty.push_back(propKernelSize);
    mMapIdToProperty[propId] = propKernelSize;
}

QJsonObject CVMedianBlurModel::save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();
    QJsonObject cParams = modelJson["cParams"].toObject();
    cParams["kernel_size"] = mParams.miKernelSize;
    modelJson["cParams"] = cParams;
    return modelJson;
}

void CVMedianBlurModel::load(const QJsonObject &p)
{
    PBAsyncDataModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        QJsonValue v = paramsObj["kernel_size"];
        if (!v.isUndefined()) {
            int ksize = v.toInt();
            // Enforce odd number constraint
            if (ksize % 2 == 0) {
                ksize = (ksize < 3) ? 3 : ksize + 1; // Make it odd
            }
            if (ksize < 3) ksize = 3;
            if (ksize > 31) ksize = 31;
            
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = ksize;
            mParams.miKernelSize = ksize;
        }
    }
}

void CVMedianBlurModel::setModelProperty(QString &id, const QVariant &value)
{
    if( !mMapIdToProperty.contains(id) )
        return;
    
    if (id == "kernel_size") 
    {
        int ksize = value.toInt();
        // Enforce odd number constraint
        if (ksize % 2 == 0) {
            ksize = (ksize < 3) ? 3 : ksize + 1; // Make it odd
        }
        if (ksize < 3) ksize = 3;
        if (ksize > 31) ksize = 31;
        
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = ksize;
        mParams.miKernelSize = ksize;
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

QObject* CVMedianBlurModel::createWorker()
{
    return new CVMedianBlurWorker();
}

void CVMedianBlurModel::connectWorker(QObject* worker)
{
    auto w = qobject_cast<CVMedianBlurWorker*>(worker);
    if (w) {
        connect(w, &CVMedianBlurWorker::frameReady, this, &PBAsyncDataModel::handleFrameReady);
    }
}

void CVMedianBlurModel::dispatchPendingWork()
{
    if (!mPendingFrame.empty() && mpWorker) {
        setPendingWork(false);

        ensure_frame_pool(mPendingFrame.cols, mPendingFrame.rows, mPendingFrame.type());

        auto pool = getFramePool();
        long frameId = getNextFrameId();
        QString producerId = QString::number(reinterpret_cast<std::uintptr_t>(this));

        setWorkerBusy(true);

        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                 Qt::QueuedConnection,
                                 Q_ARG(cv::Mat, mPendingFrame),
                                 Q_ARG(CVMedianBlurParameters, mPendingParams),
                                 Q_ARG(FrameSharingMode, getSharingMode()),
                                 Q_ARG(std::shared_ptr<CVImagePool>, pool),
                                 Q_ARG(long, frameId),
                                 Q_ARG(QString, producerId));

        mPendingFrame = cv::Mat(); // Clear pending
    }
}

void CVMedianBlurModel::process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty()) {
        return;
    }

    if (isWorkerBusy()) {
        // Store pending frame
        mPendingFrame = mpCVImageInData->data().clone();
        mPendingParams = mParams;
        setPendingWork(true);
        return;
    }

    // Worker available - dispatch immediately
    cv::Mat input = mpCVImageInData->data();
    ensure_frame_pool(input.cols, input.rows, input.type());

    auto pool = getFramePool();
    long frameId = getNextFrameId();
    QString producerId = QString::number(reinterpret_cast<std::uintptr_t>(this));

    setWorkerBusy(true);

    QMetaObject::invokeMethod(mpWorker, "processFrame",
                             Qt::QueuedConnection,
                             Q_ARG(cv::Mat, input),
                             Q_ARG(CVMedianBlurParameters, mParams),
                             Q_ARG(FrameSharingMode, getSharingMode()),
                             Q_ARG(std::shared_ptr<CVImagePool>, pool),
                             Q_ARG(long, frameId),
                             Q_ARG(QString, producerId));
}
