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

#include "CVCLAHEEqualizationModel.hpp"

#include <QtCore/QTimer>
#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVCLAHEEqualizationModel::_category = QString("Image Enhancement");
const QString CVCLAHEEqualizationModel::_model_name = QString("CV CLAHE Equalization");

void CVCLAHEEqualizationWorker::processFrame(cv::Mat input,
                                             double clipLimit,
                                             int tileSize,
                                             bool applyColorLuma,
                                             int colorSpaceIndex,
                                             bool convertTo8Bit,
                                             FrameSharingMode mode,
                                             std::shared_ptr<CVImagePool> pool,
                                             long frameId,
                                             QString producerId)
{
    if (input.empty()) { Q_EMIT frameReady(nullptr); return; }
    if (input.depth() != CV_8U && convertTo8Bit) {
        cv::Mat tmp;
        if (input.channels() == 1) {
            cv::normalize(input, tmp, 0, 255, cv::NORM_MINMAX);
            tmp.convertTo(tmp, CV_8U);
        } else {
            std::vector<cv::Mat> ch; cv::split(input, ch);
            for (auto &c : ch) {
                cv::Mat n; cv::normalize(c, n, 0, 255, cv::NORM_MINMAX);
                n.convertTo(n, CV_8U); c = n;
            }
            cv::merge(ch, tmp);
        }
        input = tmp;
    }
    if (input.depth() != CV_8U && !convertTo8Bit) {
        auto passthrough = std::make_shared<CVImageData>(input.clone());
        FrameMetadata md; md.producerId = producerId; md.frameId = frameId; passthrough->updateMove(std::move(input), md);
        Q_EMIT frameReady(passthrough); return;
    }

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clipLimit, cv::Size(tileSize, tileSize));
    cv::Mat result;
    if (input.channels() == 1) {
        clahe->apply(input, result);
    } else if (applyColorLuma) {
        int forwardCode = (colorSpaceIndex == 0) ? cv::COLOR_BGR2YCrCb : cv::COLOR_BGR2Lab;
        int inverseCode = (colorSpaceIndex == 0) ? cv::COLOR_YCrCb2BGR : cv::COLOR_Lab2BGR;
        cv::Mat converted; cv::cvtColor(input, converted, forwardCode);
        std::vector<cv::Mat> channels; cv::split(converted, channels);
        clahe->apply(channels[0], channels[0]);
        cv::merge(channels, converted);
        cv::cvtColor(converted, result, inverseCode);
    } else {
        std::vector<cv::Mat> channels; cv::split(input, channels);
        for (auto &c : channels) clahe->apply(c, c);
        cv::merge(channels, result);
    }

    FrameMetadata metadata; metadata.producerId = producerId; metadata.frameId = frameId;
    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    if (mode == FrameSharingMode::PoolMode && pool) {
        auto handle = pool->acquire(1, metadata);
        if (handle) {
            result.copyTo(handle.matrix());
            if (!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle))) pooled = true;
        }
    }
    if (!pooled) {
        if (result.empty()) { Q_EMIT frameReady(nullptr); return; }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}

CVCLAHEEqualizationModel::CVCLAHEEqualizationModel()
    : PBAsyncDataModel(_model_name), _minPixmap(":CLAHEEqualization.png")
{
    qRegisterMetaType<CVCLAHEEqualizationParameters>("CVCLAHEEqualizationParameters");
    
    // Clip limit (double)
    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdClipLimit;
    doublePropertyType.mdMin = 0.1;
    doublePropertyType.mdMax = 40.0;
    QString propId = "clip_limit";
    auto propClip = std::make_shared<TypedProperty<DoublePropertyType>>("Clip Limit", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propClip);
    mMapIdToProperty[propId] = propClip;

    // Tile size (int)
    IntPropertyType intProp;
    intProp.miValue = mParams.miTileSize;
    propId = "tile_size";
    auto propTile = std::make_shared<TypedProperty<IntPropertyType>>("Tile Size", propId, QMetaType::Int, intProp, "Operation");
    mvProperty.push_back(propTile);
    mMapIdToProperty[propId] = propTile;

    // Apply color luma (bool)
    propId = "apply_color_luma";
    auto propApplyColor = std::make_shared<TypedProperty<bool>>("Apply On Color Luma", propId, QMetaType::Bool, mParams.mbApplyColorLuma, "Operation");
    mvProperty.push_back(propApplyColor);
    mMapIdToProperty[propId] = propApplyColor;

    // Color space enum
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

QObject* CVCLAHEEqualizationModel::createWorker() { return new CVCLAHEEqualizationWorker(); }

void CVCLAHEEqualizationModel::connectWorker(QObject* worker)
{
    if (auto *w = qobject_cast<CVCLAHEEqualizationWorker*>(worker)) {
        connect(w, &CVCLAHEEqualizationWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void CVCLAHEEqualizationModel::dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown()) return;
    cv::Mat input = mPendingFrame; CVCLAHEEqualizationParameters params = mPendingParams; setPendingWork(false);
    ensure_frame_pool(input.cols, input.rows, input.type());
    long frameId = getNextFrameId(); QString producerId = getNodeId(); std::shared_ptr<CVImagePool> poolCopy = getFramePool();
    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame", Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(double, params.mdClipLimit),
                              Q_ARG(int, params.miTileSize),
                              Q_ARG(bool, params.mbApplyColorLuma),
                              Q_ARG(int, params.miColorSpaceIndex),
                              Q_ARG(bool, params.mbConvertTo8Bit),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

QJsonObject CVCLAHEEqualizationModel::save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();
    QJsonObject cParams; cParams["clipLimit"] = mParams.mdClipLimit; cParams["tileSize"] = mParams.miTileSize; cParams["applyColorLuma"] = mParams.mbApplyColorLuma; cParams["colorSpaceIndex"] = mParams.miColorSpaceIndex; cParams["convertTo8Bit"] = mParams.mbConvertTo8Bit; modelJson["cParams"] = cParams; return modelJson;
}

void CVCLAHEEqualizationModel::load(const QJsonObject &p)
{
    PBAsyncDataModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject(); if (!paramsObj.isEmpty()) {
        QJsonValue v = paramsObj["clipLimit"]; if (!v.isNull()) { auto prop = mMapIdToProperty["clip_limit"]; auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop); typedProp->getData().mdValue = v.toDouble(); mParams.mdClipLimit = v.toDouble(); }
        v = paramsObj["tileSize"]; if (!v.isNull()) { auto prop = mMapIdToProperty["tile_size"]; auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typedProp->getData().miValue = v.toInt(); mParams.miTileSize = v.toInt(); }
        v = paramsObj["applyColorLuma"]; if (!v.isNull()) { auto prop = mMapIdToProperty["apply_color_luma"]; auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop); typedProp->getData() = v.toBool(); mParams.mbApplyColorLuma = v.toBool(); }
        v = paramsObj["colorSpaceIndex"]; if (!v.isNull()) { auto prop = mMapIdToProperty["color_space"]; auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop); typedProp->getData().miCurrentIndex = v.toInt(); mParams.miColorSpaceIndex = v.toInt(); }
        v = paramsObj["convertTo8Bit"]; if (!v.isNull()) { auto prop = mMapIdToProperty["convert_to_8bit"]; auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop); typedProp->getData() = v.toBool(); mParams.mbConvertTo8Bit = v.toBool(); }
    }
}

void CVCLAHEEqualizationModel::setModelProperty(QString &id, const QVariant &value)
{
    if (!mMapIdToProperty.contains(id)) return;
    if (id == "clip_limit") {
        auto prop = mMapIdToProperty[id]; auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop); double clip = value.toDouble(); if (clip < 0.1) clip = 0.1; if (clip > 40.0) clip = 40.0; typedProp->getData().mdValue = clip; mParams.mdClipLimit = clip; }
    else if (id == "tile_size") {
        auto prop = mMapIdToProperty[id]; auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); int t = value.toInt(); if (t < 2) t = 2; if (t > 64) t = 64; typedProp->getData().miValue = t; mParams.miTileSize = t; }
    else if (id == "apply_color_luma") {
        auto prop = mMapIdToProperty[id]; auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop); typedProp->getData() = value.toBool(); mParams.mbApplyColorLuma = value.toBool(); }
    else if (id == "color_space") {
        auto prop = mMapIdToProperty[id]; auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop); typedProp->getData().miCurrentIndex = value.toInt(); mParams.miColorSpaceIndex = value.toInt(); }
    else if (id == "convert_to_8bit") {
        auto prop = mMapIdToProperty[id]; auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop); typedProp->getData() = value.toBool(); mParams.mbConvertTo8Bit = value.toBool(); }
    else { PBAsyncDataModel::setModelProperty(id, value); return; }
    if (mpCVImageInData && !isShuttingDown()) process_cached_input();
}

void CVCLAHEEqualizationModel::process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty()) return;
    cv::Mat input = mpCVImageInData->data();
    QTimer::singleShot(0, this, [this]() { mpSyncData->data() = false; Q_EMIT dataUpdated(1); });
    if (isWorkerBusy()) { mPendingFrame = input.clone(); mPendingParams = mParams; setPendingWork(true); }
    else {
        setWorkerBusy(true);
        ensure_frame_pool(input.cols, input.rows, input.type());
        long frameId = getNextFrameId(); QString producerId = getNodeId(); std::shared_ptr<CVImagePool> poolCopy = getFramePool();
        QMetaObject::invokeMethod(mpWorker, "processFrame", Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(double, mParams.mdClipLimit),
                                  Q_ARG(int, mParams.miTileSize),
                                  Q_ARG(bool, mParams.mbApplyColorLuma),
                                  Q_ARG(int, mParams.miColorSpaceIndex),
                                  Q_ARG(bool, mParams.mbConvertTo8Bit),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}
