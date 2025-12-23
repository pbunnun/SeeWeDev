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

#include "CVBilateralFilterModel.hpp"

#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

// Static member definitions
const QString CVBilateralFilterModel::_category = QString("Image Modification");
const QString CVBilateralFilterModel::_model_name = QString("CV Bilateral Filter");

void CVBilateralFilterWorker::processFrame(cv::Mat input,
                                            CVBilateralFilterParameters params,
                                            FrameSharingMode mode,
                                            std::shared_ptr<CVImagePool> pool,
                                            long frameId,
                                            QString producerId)
{
    if (input.empty()) {
        Q_EMIT frameReady(nullptr);
        return;
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
                cv::bilateralFilter(input, handle.matrix(), 
                                   params.miDiameter, 
                                   params.mdSigmaColor, 
                                   params.mdSigmaSpace);
                if (!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                    pooled = true;
            } catch (const cv::Exception& e) {
                qWarning() << "CVBilateralFilter error:" << e.what();
            }
        }
    }

    if (!pooled) {
        cv::Mat result;
        try {
            cv::bilateralFilter(input, result, 
                               params.miDiameter, 
                               params.mdSigmaColor, 
                               params.mdSigmaSpace);
        } catch (const cv::Exception& e) {
            qWarning() << "CVBilateralFilter error:" << e.what();
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

CVBilateralFilterModel::CVBilateralFilterModel()
    : PBAsyncDataModel("CV Bilateral Filter"),
    _minPixmap(":/CVBilateralFilterModel.png")
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mpSyncData = std::make_shared<SyncData>();

    // Register meta types for worker thread communication
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<CVBilateralFilterParameters>("CVBilateralFilterParameters");
    qRegisterMetaType<std::shared_ptr<CVImageData>>("std::shared_ptr<CVImageData>");
    qRegisterMetaType<std::shared_ptr<CVImagePool>>("std::shared_ptr<CVImagePool>");
    qRegisterMetaType<FrameSharingMode>("FrameSharingMode");

    // Add diameter property
    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miDiameter;
    intPropertyType.miMin = 0;
    intPropertyType.miMax = 31;
    QString propId = "diameter";
    auto propDiameter = std::make_shared<TypedProperty<IntPropertyType>>("Diameter", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propDiameter);
    mMapIdToProperty[propId] = propDiameter;

    // Add sigma color property
    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdSigmaColor;
    doublePropertyType.mdMin = 0.0;
    doublePropertyType.mdMax = 200.0;
    propId = "sigma_color";
    auto propSigmaColor = std::make_shared<TypedProperty<DoublePropertyType>>("Sigma Color", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propSigmaColor);
    mMapIdToProperty[propId] = propSigmaColor;

    // Add sigma space property
    doublePropertyType.mdValue = mParams.mdSigmaSpace;
    propId = "sigma_space";
    auto propSigmaSpace = std::make_shared<TypedProperty<DoublePropertyType>>("Sigma Space", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propSigmaSpace);
    mMapIdToProperty[propId] = propSigmaSpace;
}

QJsonObject CVBilateralFilterModel::save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();
    QJsonObject cParams = modelJson["cParams"].toObject();
    cParams["diameter"] = mParams.miDiameter;
    cParams["sigma_color"] = mParams.mdSigmaColor;
    cParams["sigma_space"] = mParams.mdSigmaSpace;
    modelJson["cParams"] = cParams;
    return modelJson;
}

void CVBilateralFilterModel::load(const QJsonObject &p)
{
    PBAsyncDataModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        QJsonValue v = paramsObj["diameter"];
        if (!v.isUndefined()) {
            auto prop = mMapIdToProperty["diameter"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            mParams.miDiameter = v.toInt();
        }
        
        v = paramsObj["sigma_color"];
        if (!v.isUndefined()) {
            auto prop = mMapIdToProperty["sigma_color"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdSigmaColor = v.toDouble();
        }
        
        v = paramsObj["sigma_space"];
        if (!v.isUndefined()) {
            auto prop = mMapIdToProperty["sigma_space"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdSigmaSpace = v.toDouble();
        }
    }
}

std::shared_ptr<NodeData> CVBilateralFilterModel::outData(PortIndex port)
{
    if (port == 0 && mpCVImageData) {
        return mpCVImageData;
    } else if (port == 1 && mpSyncData) {
        return mpSyncData;
    }
    return nullptr;
}

// setInData is handled by PBAsyncDataModel (image cache + sync trigger)

void CVBilateralFilterModel::setModelProperty(QString &id, const QVariant &value)
{
    if( !mMapIdToProperty.contains(id))
        return;
    
    if (id == "diameter") {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miDiameter = value.toInt();
    } else if (id == "sigma_color") {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdSigmaColor = value.toDouble();
    } else if (id == "sigma_space") {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdSigmaSpace = value.toDouble();
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

QObject* CVBilateralFilterModel::createWorker()
{
    return new CVBilateralFilterWorker();
}

void CVBilateralFilterModel::connectWorker(QObject* worker)
{
    auto w = qobject_cast<CVBilateralFilterWorker*>(worker);
    if (w) {
        connect(w, &CVBilateralFilterWorker::frameReady, this, &PBAsyncDataModel::handleFrameReady);
    }
}

void CVBilateralFilterModel::dispatchPendingWork()
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
                                 Q_ARG(CVBilateralFilterParameters, mPendingParams),
                                 Q_ARG(FrameSharingMode, getSharingMode()),
                                 Q_ARG(std::shared_ptr<CVImagePool>, pool),
                                 Q_ARG(long, frameId),
                                 Q_ARG(QString, producerId));

        mPendingFrame = cv::Mat(); // Clear pending
    }
}

void CVBilateralFilterModel::process_cached_input()
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
                             Q_ARG(CVBilateralFilterParameters, mParams),
                             Q_ARG(FrameSharingMode, getSharingMode()),
                             Q_ARG(std::shared_ptr<CVImagePool>, pool),
                             Q_ARG(long, frameId),
                             Q_ARG(QString, producerId));
}

