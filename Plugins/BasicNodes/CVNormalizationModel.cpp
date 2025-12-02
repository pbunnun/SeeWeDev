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

#include "CVNormalizationModel.hpp"
#include <QDebug>
#include <QTimer>
#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVNormalizationModel::_category = QString("Image Conversion");
const QString CVNormalizationModel::_model_name = QString("CV Normalization");

void CVNormalizationWorker::processFrame(
    cv::Mat frame,
    double rangeMin,
    double rangeMax,
    int normType,
    FrameSharingMode mode,
    std::shared_ptr<CVImagePool> pool,
    long frameId,
    QString producerId)
{
    if (frame.empty())
    {
        Q_EMIT frameReady(nullptr);
        return;
    }

    cv::Mat normalized;
    cv::normalize(frame, normalized, rangeMin, rangeMax, normType);

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto outputImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    if (mode == FrameSharingMode::PoolMode && pool && !normalized.empty())
    {
        auto handle = pool->acquire(1, metadata);
        if (handle)
        {
            normalized.copyTo(handle.matrix());
            if (!handle.matrix().empty() && outputImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if (!pooled && !normalized.empty())
    {
        outputImageData->updateMove(std::move(normalized), metadata);
    }

    Q_EMIT frameReady(outputImageData);
}

CVNormalizationModel::CVNormalizationModel()
    : PBAsyncDataModel(_model_name),
      _minPixmap(":Normalization.png")
{
    DoublePropertyType doublePropertyType;
    
    QString propId = "range_max";
    doublePropertyType.mdValue = mParams.mdRangeMax;
    doublePropertyType.mdMax = 255.0;
    auto propRangeMax = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Maximum", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propRangeMax);
    mMapIdToProperty[propId] = propRangeMax;

    propId = "range_min";
    doublePropertyType.mdValue = mParams.mdRangeMin;
    doublePropertyType.mdMax = 255.0;
    auto propRangeMin = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Minimum", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propRangeMin);
    mMapIdToProperty[propId] = propRangeMin;

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({
        "NORM_L1", "NORM_L2", "NORM_INF", "NORM_L2SQR", "NORM_MINMAX", 
        "NORM_HAMMING", "NORM_HAMMING2", "NORM_RELATIVE", "NORM_TYPE_MASK"
    });
    enumPropertyType.miCurrentIndex = 4;
    propId = "norm_type";
    auto propNormType = std::make_shared<TypedProperty<EnumPropertyType>>(
        "Norm Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back(propNormType);
    mMapIdToProperty[propId] = propNormType;
}

QObject* CVNormalizationModel::createWorker()
{
    return new CVNormalizationWorker();
}

void CVNormalizationModel::connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVNormalizationWorker*>(worker);
    if (w)
    {
        connect(w, &CVNormalizationWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void CVNormalizationModel::dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;

    cv::Mat frame = mPendingFrame;
    NormalizationParameters params = mPendingParams;
    setPendingWork(false);

    ensure_frame_pool(frame.cols, frame.rows, frame.type());

    long frameId = getNextFrameId();
    QString producerId = getNodeId();
    std::shared_ptr<CVImagePool> poolCopy = getFramePool();

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, frame.clone()),
                              Q_ARG(double, params.mdRangeMin),
                              Q_ARG(double, params.mdRangeMax),
                              Q_ARG(int, params.miNormType),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

void CVNormalizationModel::process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;

    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });

    cv::Mat frame = mpCVImageInData->data();

    if (isWorkerBusy())
    {
        mPendingFrame = frame.clone();
        mPendingParams = mParams;
        setPendingWork(true);
    }
    else
    {
        setWorkerBusy(true);

        ensure_frame_pool(frame.cols, frame.rows, frame.type());

        long frameId = getNextFrameId();
        QString producerId = getNodeId();
        std::shared_ptr<CVImagePool> poolCopy = getFramePool();

        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, frame.clone()),
                                  Q_ARG(double, mParams.mdRangeMin),
                                  Q_ARG(double, mParams.mdRangeMax),
                                  Q_ARG(int, mParams.miNormType),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}

void CVNormalizationModel::setModelProperty(QString& id, const QVariant& value)
{
    if (!mMapIdToProperty.contains(id))
        return;

    if (id == "range_max")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdRangeMax = value.toDouble();
    }
    else if (id == "range_min")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdRangeMin = value.toDouble();
    }
    else if (id == "norm_type")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        
        switch(value.toInt())
        {
        case 0: mParams.miNormType = cv::NORM_L1; break;
        case 1: mParams.miNormType = cv::NORM_L2; break;
        case 2: mParams.miNormType = cv::NORM_INF; break;
        case 3: mParams.miNormType = cv::NORM_L2SQR; break;
        case 4: mParams.miNormType = cv::NORM_MINMAX; break;
        case 5: mParams.miNormType = cv::NORM_HAMMING; break;
        case 6: mParams.miNormType = cv::NORM_HAMMING2; break;
        case 7: mParams.miNormType = cv::NORM_RELATIVE; break;
        case 8: mParams.miNormType = cv::NORM_TYPE_MASK; break;
        }
    }
    else
    {
        PBAsyncDataModel::setModelProperty(id, value);
        return;
    }

    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}

QJsonObject CVNormalizationModel::save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["range_max"] = mParams.mdRangeMax;
    cParams["range_min"] = mParams.mdRangeMin;
    cParams["norm_type"] = mParams.miNormType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void CVNormalizationModel::load(const QJsonObject& json)
{
    PBAsyncDataModel::load(json);

    QJsonObject paramsObj = json["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v = paramsObj["range_max"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["range_max"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdRangeMax = v.toDouble();
        }
        
        v = paramsObj["range_min"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["range_min"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdRangeMin = v.toDouble();
        }
        
        v = paramsObj["norm_type"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["norm_type"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miNormType = v.toInt();
        }
    }
}
