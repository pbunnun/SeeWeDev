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

#include "CVColorSpaceModel.hpp"

#include <QDebug>
#include <QTimer>
#include <QMetaObject>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVColorSpaceModel::_category = QString("Image Conversion");

const QString CVColorSpaceModel::_model_name = QString("CV Color Space");

CVColorSpaceModel::
    CVColorSpaceModel()
    : PBAsyncDataModel(_model_name),
      _minPixmap(":CVColorSpace.png")
{
    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"GRAY", "BGR", "RGB", "HSV"});
    enumPropertyType.miCurrentIndex = 1;
    QString propId = "color_space_input";
    auto propCVColorSpaceInput = std::make_shared<TypedProperty<EnumPropertyType>>("Input Color Space", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back(propCVColorSpaceInput);
    mMapIdToProperty[propId] = propCVColorSpaceInput;

    enumPropertyType.mslEnumNames = QStringList({"GRAY", "BGR", "RGB", "HSV"});
    enumPropertyType.miCurrentIndex = 2;
    propId = "color_space_output";
    auto propCVColorSpaceOutput = std::make_shared<TypedProperty<EnumPropertyType>>("Output Color Space", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back(propCVColorSpaceOutput);
    mMapIdToProperty[propId] = propCVColorSpaceOutput;
}

QObject *
CVColorSpaceModel::
    createWorker()
{
    return new CVColorSpaceWorker();
}

void CVColorSpaceModel::
    connectWorker(QObject *worker)
{
    auto *w = qobject_cast<CVColorSpaceWorker *>(worker);
    if (w)
    {
        connect(w, &CVColorSpaceWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void CVColorSpaceModel::
    dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown() || mPendingFrame.empty())
        return;

    cv::Mat input = mPendingFrame;
    CVColorSpaceParameters params = mPendingParams;
    setPendingWork(false);

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(CVColorSpaceParameters, params));
}

void CVColorSpaceModel::
    process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;

    cv::Mat input = mpCVImageInData->data();

    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]()
                       {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1); });

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
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(CVColorSpaceParameters, mParams));
    }
}

QJsonObject
CVColorSpaceModel::
    save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["colorSpaceInput"] = mParams.miCVColorSpaceInput;
    cParams["colorSpaceOutput"] = mParams.miCVColorSpaceOutput;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void CVColorSpaceModel::
    load(QJsonObject const &p)
{
    PBAsyncDataModel::load(p);

    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v = paramsObj["colorSpaceInput"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["color_space_input"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miCVColorSpaceInput = v.toInt();
        }
        v = paramsObj["colorSpaceOutput"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["color_space_output"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miCVColorSpaceOutput = v.toInt();
        }
    }
}

void CVColorSpaceModel::
    setModelProperty(QString &id, const QVariant &value)
{
    if (!mMapIdToProperty.contains(id))
        return;

    auto prop = mMapIdToProperty[id];
    if (id == "color_space_input")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        mParams.miCVColorSpaceInput = value.toInt();
    }
    else if (id == "color_space_output")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        mParams.miCVColorSpaceOutput = value.toInt();
    }
    else
    {
        // Base class handles pool_size and sharing_mode
        // Need to call base class to handle pool_size and sharing_mode
        PBAsyncDataModel::setModelProperty(id, value);
        // No need to process_cached_input() here
        return;
    }
    
    // Process cached input if available and not shutting down
    if (mpCVImageInData && !isShuttingDown())
    {
        process_cached_input();
    }
}
