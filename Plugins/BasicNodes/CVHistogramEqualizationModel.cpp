// Copyright © 2025, NECTEC, all rights reserved
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "CVHistogramEqualizationModel.hpp"

#include <QtCore/QTimer>
#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVHistogramEqualizationModel::_category = QString("Image Enhancement");
const QString CVHistogramEqualizationModel::_model_name = QString("CV Histogram Equalization");

void CVHistogramEqualizationWorker::processFrame(cv::Mat input,
                                                 bool applyColorLuma,
                                                 int colorSpaceIndex,
                                                 bool convertTo8Bit,
                                                 FrameSharingMode mode,
                                                 std::shared_ptr<CVImagePool> pool,
                                                 long frameId,
                                                 QString producerId)
{
    if (input.empty())
    {
        Q_EMIT frameReady(nullptr);
        return;
    }

    // Optional normalization & conversion to 8U for non-8U inputs
    if (input.depth() != CV_8U && convertTo8Bit)
    {
        cv::Mat tmp;
        if (input.channels() == 1)
        {
            cv::normalize(input, tmp, 0, 255, cv::NORM_MINMAX);
            tmp.convertTo(tmp, CV_8U);
        }
        else
        {
            std::vector<cv::Mat> ch;
            cv::split(input, ch);
            for (auto &c : ch)
            {
                cv::Mat n;
                cv::normalize(c, n, 0, 255, cv::NORM_MINMAX);
                n.convertTo(n, CV_8U);
                c = n;
            }
            cv::merge(ch, tmp);
        }
        input = tmp;
    }
    if (input.depth() != CV_8U && !convertTo8Bit)
    {
        auto passthrough = std::make_shared<CVImageData>(input.clone());
        FrameMetadata md;
        md.producerId = producerId;
        md.frameId = frameId;
        passthrough->updateMove(std::move(input), md);
        Q_EMIT frameReady(passthrough);
        return;
    }

    cv::Mat result;
    auto processGray = [&](const cv::Mat &gray)
    { cv::equalizeHist(gray, result); };

    if (input.channels() == 1)
    {
        processGray(input);
    }
    else if (applyColorLuma)
    {
        cv::Mat converted;
        int forwardCode = (colorSpaceIndex == 0) ? cv::COLOR_BGR2YCrCb : cv::COLOR_BGR2Lab;
        int inverseCode = (colorSpaceIndex == 0) ? cv::COLOR_YCrCb2BGR : cv::COLOR_Lab2BGR;
        cv::cvtColor(input, converted, forwardCode);
        std::vector<cv::Mat> channels;
        cv::split(converted, channels);
        cv::equalizeHist(channels[0], channels[0]);
        cv::merge(channels, converted);
        cv::cvtColor(converted, result, inverseCode);
    }
    else
    {
        // Per-channel equalization (may shift colors)
        std::vector<cv::Mat> channels;
        cv::split(input, channels);
        for (auto &c : channels)
            if (c.depth() == CV_8U)
                cv::equalizeHist(c, c);
        cv::merge(channels, result);
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
            result.copyTo(handle.matrix());
            if (!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if (!pooled)
    {
        if (result.empty())
        {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}

CVHistogramEqualizationModel::CVHistogramEqualizationModel()
    : PBAsyncDataModel(_model_name), _minPixmap(":/CVCreateHistogramModel.png")
{
    qRegisterMetaType<CVHistogramEqualizationParameters>("CVHistogramEqualizationParameters");
    
    // Bool: apply color luminance
    QString propId = "apply_color_luma";
    auto propApplyColor = std::make_shared<TypedProperty<bool>>("Apply On Color Luma", propId, QMetaType::Bool, mParams.mbApplyColorLuma, "Operation");
    mvProperty.push_back(propApplyColor);
    mMapIdToProperty[propId] = propApplyColor;

    // Enum: color space selection
    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"YCrCb", "Lab"});
    enumPropertyType.miCurrentIndex = mParams.miColorSpaceIndex;
    propId = "color_space";
    auto propColorSpace = std::make_shared<TypedProperty<EnumPropertyType>>("Color Space", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back(propColorSpace);
    mMapIdToProperty[propId] = propColorSpace;

    // Convert non-8U toggle
    propId = "convert_to_8bit";
    auto propConvert = std::make_shared<TypedProperty<bool>>("Convert Non-8U", propId, QMetaType::Bool, mParams.mbConvertTo8Bit, "Operation");
    mvProperty.push_back(propConvert);
    mMapIdToProperty[propId] = propConvert;
}

QObject *CVHistogramEqualizationModel::createWorker() 
{ 
    return new CVHistogramEqualizationWorker(); 
}

void CVHistogramEqualizationModel::connectWorker(QObject *worker)
{
    if (auto *w = qobject_cast<CVHistogramEqualizationWorker *>(worker))
    {
        connect(w, &CVHistogramEqualizationWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void CVHistogramEqualizationModel::dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;
    cv::Mat input = mPendingFrame;
    CVHistogramEqualizationParameters params = mPendingParams;
    setPendingWork(false);
    ensure_frame_pool(input.cols, input.rows, input.type());
    long frameId = getNextFrameId();
    QString producerId = getNodeId();
    std::shared_ptr<CVImagePool> poolCopy = getFramePool();
    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame", Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(bool, params.mbApplyColorLuma),
                              Q_ARG(int, params.miColorSpaceIndex),
                              Q_ARG(bool, params.mbConvertTo8Bit),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

QJsonObject CVHistogramEqualizationModel::save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();
    QJsonObject cParams;
    cParams["applyColorLuma"] = mParams.mbApplyColorLuma;
    cParams["colorSpaceIndex"] = mParams.miColorSpaceIndex;
    cParams["convertTo8Bit"] = mParams.mbConvertTo8Bit;
    modelJson["cParams"] = cParams;
    return modelJson;
}

void CVHistogramEqualizationModel::load(const QJsonObject &p)
{
    PBAsyncDataModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v = paramsObj["applyColorLuma"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["apply_color_luma"];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();
            mParams.mbApplyColorLuma = v.toBool();
        }
        v = paramsObj["colorSpaceIndex"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["color_space"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miColorSpaceIndex = v.toInt();
        }
        v = paramsObj["convertTo8Bit"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["convert_to_8bit"];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();
            mParams.mbConvertTo8Bit = v.toBool();
        }
    }
}

void CVHistogramEqualizationModel::setModelProperty(QString &id, const QVariant &value)
{
    if (!mMapIdToProperty.contains(id))
        return;
    if (id == "apply_color_luma")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typedProp->getData() = value.toBool();
        mParams.mbApplyColorLuma = value.toBool();
    }
    else if (id == "color_space")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        mParams.miColorSpaceIndex = value.toInt();
    }
    else if (id == "convert_to_8bit")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typedProp->getData() = value.toBool();
        mParams.mbConvertTo8Bit = value.toBool();
    }
    else
    {
        PBAsyncDataModel::setModelProperty(id, value);
        return;
    }
    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}

void CVHistogramEqualizationModel::process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;
    cv::Mat input = mpCVImageInData->data();
    QTimer::singleShot(0, this, [this]()
                       { mpSyncData->data() = false; Q_EMIT dataUpdated(1); });
    if (isWorkerBusy())
    {
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
        QMetaObject::invokeMethod(mpWorker, "processFrame", Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(bool, mParams.mbApplyColorLuma),
                                  Q_ARG(int, mParams.miColorSpaceIndex),
                                  Q_ARG(bool, mParams.mbConvertTo8Bit),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}
