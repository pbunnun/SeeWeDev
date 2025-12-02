// Copyright © 2025, NECTEC, all rights reserved
// Licensed under the Apache License, Version 2.0

#include "CVOpticalFlowFarnebackModel.hpp"
#include <QDebug>
#include <QTimer>
#include <opencv2/video.hpp>
#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVOpticalFlowFarnebackModel::_category = QString("Computer Vision");
const QString CVOpticalFlowFarnebackModel::_model_name = QString("CV Optical Flow Farneback");

void CVOpticalFlowFarnebackWorker::processFrame(
    cv::Mat currentFrame,
    cv::Mat previousFrame,
    CVOpticalFlowFarnebackParameters params,
    FrameSharingMode mode,
    std::shared_ptr<CVImagePool> pool,
    long frameId,
    QString producerId)
{
    if (currentFrame.empty() || previousFrame.empty())
    {
        Q_EMIT frameReady(nullptr);
        return;
    }

    // Convert to grayscale if needed
    cv::Mat currGray, prevGray;
    if (currentFrame.channels() == 3)
    {
        cv::cvtColor(currentFrame, currGray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        currGray = currentFrame;
    }

    if (previousFrame.channels() == 3)
    {
        cv::cvtColor(previousFrame, prevGray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        prevGray = previousFrame;
    }

    // Compute optical flow
    cv::Mat flow;
    cv::calcOpticalFlowFarneback(
        prevGray, currGray, flow,
        params.mdPyrScale, params.miLevels, params.miWinsize,
        params.miIterations, params.miPolyN, params.mdPolySigma,
        params.miFlags);

    // Create visualization
    cv::Mat visual;

    if (params.mbShowMagnitude)
    {
        // Magnitude visualization
        cv::Mat magnitude, angle;
        cv::Mat flowParts[2];
        cv::split(flow, flowParts);
        cv::cartToPolar(flowParts[0], flowParts[1], magnitude, angle, true);

        // Normalize and scale
        cv::normalize(magnitude, magnitude, 0, 255, cv::NORM_MINMAX);
        magnitude.convertTo(magnitude, CV_8U);

        // Apply colormap
        cv::applyColorMap(magnitude, visual, params.miColorMapType);
    }
    else
    {
        // Direction visualization using HSV
        cv::Mat flowParts[2];
        cv::split(flow, flowParts);
        cv::Mat magnitude, angle;
        cv::cartToPolar(flowParts[0], flowParts[1], magnitude, angle, true);

        // Create HSV image
        cv::Mat hsv = cv::Mat::zeros(flow.size(), CV_8UC3);
        cv::Mat hsvChannels[3];

        // Hue = angle
        angle.convertTo(hsvChannels[0], CV_8U, 255.0 / 360.0);
        // Saturation = max
        hsvChannels[1] = cv::Mat::ones(flow.size(), CV_8U) * 255;
        // Value = magnitude
        cv::normalize(magnitude, magnitude, 0, 255, cv::NORM_MINMAX);
        magnitude.convertTo(hsvChannels[2], CV_8U);

        cv::merge(hsvChannels, 3, hsv);
        cv::cvtColor(hsv, visual, cv::COLOR_HSV2BGR);
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto outputImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    if (mode == FrameSharingMode::PoolMode && pool && !visual.empty())
    {
        auto handle = pool->acquire(1, metadata);
        if (handle)
        {
            visual.copyTo(handle.matrix());
            if (!handle.matrix().empty() && outputImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if (!pooled && !visual.empty())
    {
        outputImageData->updateMove(std::move(visual), metadata);
    }

    Q_EMIT frameReady(outputImageData);
}

CVOpticalFlowFarnebackModel::CVOpticalFlowFarnebackModel()
    : PBAsyncDataModel(_model_name),
      _minPixmap(":CVOpticalFlowFarneback.png")
{
    // Initialize properties
    IntPropertyType intPropertyType;
    DoublePropertyType doublePropertyType;
    EnumPropertyType enumPropertyType;

    QString propId = "pyr_scale";
    doublePropertyType.mdValue = mParams.mdPyrScale;
    doublePropertyType.mdMin = 0.1;
    doublePropertyType.mdMax = 0.99;
    auto propPyrScale = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Pyramid Scale", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propPyrScale);
    mMapIdToProperty[propId] = propPyrScale;

    propId = "levels";
    intPropertyType.miValue = mParams.miLevels;
    intPropertyType.miMin = 1;
    intPropertyType.miMax = 10;
    auto propLevels = std::make_shared<TypedProperty<IntPropertyType>>(
        "Pyramid Levels", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propLevels);
    mMapIdToProperty[propId] = propLevels;

    propId = "winsize";
    intPropertyType.miValue = mParams.miWinsize;
    intPropertyType.miMin = 3;
    intPropertyType.miMax = 100;
    auto propWinsize = std::make_shared<TypedProperty<IntPropertyType>>(
        "Window Size", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propWinsize);
    mMapIdToProperty[propId] = propWinsize;

    propId = "iterations";
    intPropertyType.miValue = mParams.miIterations;
    intPropertyType.miMin = 1;
    intPropertyType.miMax = 20;
    auto propIterations = std::make_shared<TypedProperty<IntPropertyType>>(
        "Iterations", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propIterations);
    mMapIdToProperty[propId] = propIterations;

    propId = "poly_n";
    intPropertyType.miValue = mParams.miPolyN;
    intPropertyType.miMin = 5;
    intPropertyType.miMax = 7;
    auto propPolyN = std::make_shared<TypedProperty<IntPropertyType>>(
        "Poly N", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propPolyN);
    mMapIdToProperty[propId] = propPolyN;

    propId = "poly_sigma";
    doublePropertyType.mdValue = mParams.mdPolySigma;
    doublePropertyType.mdMin = 1.0;
    doublePropertyType.mdMax = 2.0;
    auto propPolySigma = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Poly Sigma", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propPolySigma);
    mMapIdToProperty[propId] = propPolySigma;

    propId = "show_magnitude";
    auto propShowMag = std::make_shared<TypedProperty<bool>>(
        "Show Magnitude", propId, QMetaType::Bool, mParams.mbShowMagnitude, "Display");
    mvProperty.push_back(propShowMag);
    mMapIdToProperty[propId] = propShowMag;

    propId = "colormap_type";
    enumPropertyType.miCurrentIndex = mParams.miColorMapType;
    enumPropertyType.mslEnumNames = QStringList({"Autumn", "Bone", "Jet", "Winter",
                                                 "Rainbow", "Ocean", "Summer", "Spring",
                                                 "Cool", "HSV", "Pink", "Hot"});
    auto propColorMap = std::make_shared<TypedProperty<EnumPropertyType>>(
        "Color Map", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display");
    mvProperty.push_back(propColorMap);
    mMapIdToProperty[propId] = propColorMap;

    qRegisterMetaType<CVOpticalFlowFarnebackParameters>("CVOpticalFlowFarnebackParameters");
}

QObject *CVOpticalFlowFarnebackModel::createWorker()
{
    return new CVOpticalFlowFarnebackWorker();
}

void CVOpticalFlowFarnebackModel::connectWorker(QObject *worker)
{
    auto *w = qobject_cast<CVOpticalFlowFarnebackWorker *>(worker);
    if (w)
    {
        connect(w, &CVOpticalFlowFarnebackWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void CVOpticalFlowFarnebackModel::dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;

    cv::Mat currentFrame = mPendingCurrentFrame;
    cv::Mat previousFrame = mPendingPreviousFrame;
    CVOpticalFlowFarnebackParameters params = mPendingParams;
    setPendingWork(false);

    ensure_frame_pool(currentFrame.cols, currentFrame.rows, CV_8UC3);

    long frameId = getNextFrameId();
    QString producerId = getNodeId();

    std::shared_ptr<CVImagePool> poolCopy = getFramePool();

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, currentFrame.clone()),
                              Q_ARG(cv::Mat, previousFrame.clone()),
                              Q_ARG(CVOpticalFlowFarnebackParameters, params),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

void CVOpticalFlowFarnebackModel::process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;

    cv::Mat currentFrame = mpCVImageInData->data();

    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]()
                       {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1); });

    // Need previous frame for optical flow
    if (!mbHasPreviousFrame)
    {
        // Store first frame and don't process yet
        mPreviousFrame = currentFrame.clone();
        mbHasPreviousFrame = true;
        return;
    }

    if (isWorkerBusy())
    {
        // Store as pending - will be processed when worker finishes
        mPendingCurrentFrame = currentFrame.clone();
        mPendingPreviousFrame = mPreviousFrame.clone();
        mPendingParams = mParams;
        setPendingWork(true);
    }
    else
    {
        setWorkerBusy(true);

        ensure_frame_pool(currentFrame.cols, currentFrame.rows, CV_8UC3);

        long frameId = getNextFrameId();
        QString producerId = getNodeId();

        std::shared_ptr<CVImagePool> poolCopy = getFramePool();

        
        CVOpticalFlowFarnebackParameters params = mParams;
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                      Qt::QueuedConnection,
                      Q_ARG(cv::Mat, currentFrame.clone()),
                      Q_ARG(cv::Mat, mPreviousFrame.clone()),
                      Q_ARG(CVOpticalFlowFarnebackParameters, params),
                      Q_ARG(FrameSharingMode, getSharingMode()),
                      Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                      Q_ARG(long, frameId),
                      Q_ARG(QString, producerId));
    }

    // Store current as previous for next iteration
    mPreviousFrame = currentFrame.clone();
}

void CVOpticalFlowFarnebackModel::setModelProperty(QString &id, const QVariant &value)
{
    if (!mMapIdToProperty.contains(id))
        return;

    if (id == "pyr_scale")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdPyrScale = value.toDouble();
    }
    else if (id == "levels")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miLevels = value.toInt();
    }
    else if (id == "winsize")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miWinsize = value.toInt();
    }
    else if (id == "iterations")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miIterations = value.toInt();
    }
    else if (id == "poly_n")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miPolyN = value.toInt();
    }
    else if (id == "poly_sigma")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdPolySigma = value.toDouble();
    }
    else if (id == "show_magnitude")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typedProp->getData() = value.toBool();
        mParams.mbShowMagnitude = value.toBool();
    }
    else if (id == "colormap_type")
    {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        mParams.miColorMapType = value.toInt();
    }
    else
    {
        // Base class handles pool_size and sharing_mode
        PBAsyncDataModel::setModelProperty(id, value);
        return;
    }

    // Process cached input if available
    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}

QJsonObject CVOpticalFlowFarnebackModel::save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["pyrScale"] = mParams.mdPyrScale;
    cParams["levels"] = mParams.miLevels;
    cParams["winsize"] = mParams.miWinsize;
    cParams["iterations"] = mParams.miIterations;
    cParams["polyN"] = mParams.miPolyN;
    cParams["polySigma"] = mParams.mdPolySigma;
    cParams["flags"] = mParams.miFlags;
    cParams["showMagnitude"] = mParams.mbShowMagnitude;
    cParams["colorMapType"] = mParams.miColorMapType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void CVOpticalFlowFarnebackModel::load(const QJsonObject &json)
{
    PBAsyncDataModel::load(json);

    QJsonObject paramsObj = json["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v = paramsObj["pyrScale"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["pyr_scale"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdPyrScale = v.toDouble();
        }
        v = paramsObj["levels"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["levels"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            mParams.miLevels = v.toInt();
        }
        v = paramsObj["winsize"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["winsize"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            mParams.miWinsize = v.toInt();
        }
        v = paramsObj["iterations"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["iterations"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            mParams.miIterations = v.toInt();
        }
        v = paramsObj["polyN"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["poly_n"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            mParams.miPolyN = v.toInt();
        }
        v = paramsObj["polySigma"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["poly_sigma"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdPolySigma = v.toDouble();
        }
        v = paramsObj["flags"];
        if (!v.isNull())
        {
            mParams.miFlags = v.toInt();
        }
        v = paramsObj["showMagnitude"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["show_magnitude"];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();
            mParams.mbShowMagnitude = v.toBool();
        }
        v = paramsObj["colorMapType"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["colormap_type"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miColorMapType = v.toInt();
        }
    }
}
