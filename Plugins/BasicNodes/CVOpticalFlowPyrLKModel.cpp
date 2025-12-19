//Copyright © 2025, NECTEC, all rights reserved
//Licensed under the Apache License, Version 2.0

#include "CVOpticalFlowPyrLKModel.hpp"

#include <QDebug>
#include <QTimer>
#include <opencv2/video.hpp>
#include <opencv2/imgproc.hpp>

const QString CVOpticalFlowPyrLKModel::_category = QString("Computer Vision");
const QString CVOpticalFlowPyrLKModel::_model_name = QString("CV Optical Flow PyrLK");

void CVOpticalFlowPyrLKWorker::processFrame(
    cv::Mat currentFrame,
    cv::Mat previousFrame,
    CVOpticalFlowPyrLKParameters params,
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
        cv::cvtColor(currentFrame, currGray, cv::COLOR_BGR2GRAY);
    else
        currGray = currentFrame;

    if (previousFrame.channels() == 3)
        cv::cvtColor(previousFrame, prevGray, cv::COLOR_BGR2GRAY);
    else
        prevGray = previousFrame;

    // Detect features on previous frame if requested
    std::vector<cv::Point2f> prevPoints;
    if (params.mbAutoDetectFeatures)
    {
        cv::goodFeaturesToTrack(prevGray,
                                 prevPoints,
                                 std::max(1, params.miMaxCorners),
                                 std::max(1e-6, params.mdQualityLevel),
                                 std::max(0.0, params.mdMinDistance),
                                 cv::Mat(),
                                 std::max(1, params.miBlockSize));
    }

    if (prevPoints.empty())
    {
        // Nothing to track; return original current frame as visualization
        auto outputImageData = std::make_shared<CVImageData>(cv::Mat());

        cv::Mat visual;
        if (currentFrame.channels() == 1)
            cv::cvtColor(currentFrame, visual, cv::COLOR_GRAY2BGR);
        else
            visual = currentFrame.clone();

        FrameMetadata metadata;
        metadata.producerId = producerId;
        metadata.frameId = frameId;

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
        return;
    }

    // Track features
    std::vector<cv::Point2f> currPoints;
    std::vector<uchar> status;
    std::vector<float> err;

    cv::Size winSize(std::max(1, params.miWinSizeWidth), std::max(1, params.miWinSizeHeight));
    cv::TermCriteria criteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS,
                              std::max(1, params.miMaxCount), std::max(1e-9, params.mdEpsilon));

    cv::calcOpticalFlowPyrLK(prevGray,
                              currGray,
                              prevPoints,
                              currPoints,
                              status,
                              err,
                              winSize,
                              std::max(0, params.miMaxLevel),
                              criteria,
                              params.miFlags,
                              std::max(0.0, params.mdMinEigThreshold));

    // Create visualization by drawing tracks
    cv::Mat visual;
    if (currentFrame.channels() == 1)
        cv::cvtColor(currentFrame, visual, cv::COLOR_GRAY2BGR);
    else
        visual = currentFrame.clone();

    if (params.mbDrawTracks)
    {
        const cv::Scalar color(params.miTrackColorB, params.miTrackColorG, params.miTrackColorR);
        const int thickness = std::max(1, params.miTrackThickness);
        const double scale = std::max(0.1, params.mdMotionScale);
        
        for (size_t i = 0; i < status.size(); ++i)
        {
            if (status[i])
            {
                cv::Point2f scaledEnd = prevPoints[i] + scale * (currPoints[i] - prevPoints[i]);
                
                if (params.mbDrawArrows)
                {
                    cv::arrowedLine(visual, prevPoints[i], scaledEnd, color, thickness, cv::LINE_AA, 0, 0.3);
                }
                else
                {
                    cv::line(visual, prevPoints[i], scaledEnd, color, thickness);
                }
                cv::circle(visual, currPoints[i], 3, color, -1);
            }
        }
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

CVOpticalFlowPyrLKModel::CVOpticalFlowPyrLKModel()
    : PBAsyncDataModel(_model_name),
      _minPixmap(":CVOpticalFlow PyrLKModel.png")
{
    // Initialize properties
    IntPropertyType intProp;
    DoublePropertyType doubleProp;

    QString propId = "auto_detect";
    auto propAuto = std::make_shared<TypedProperty<bool>>(
        "Auto Detect Features", propId, QMetaType::Bool, mParams.mbAutoDetectFeatures, "Detection");
    mvProperty.push_back(propAuto);
    mMapIdToProperty[propId] = propAuto;

    propId = "max_corners";
    intProp.miValue = mParams.miMaxCorners;
    intProp.miMin = 1;
    intProp.miMax = 5000;
    auto propMaxCorners = std::make_shared<TypedProperty<IntPropertyType>>(
        "Max Corners", propId, QMetaType::Int, intProp, "Detection");
    mvProperty.push_back(propMaxCorners);
    mMapIdToProperty[propId] = propMaxCorners;

    propId = "quality_level";
    doubleProp.mdValue = mParams.mdQualityLevel;
    doubleProp.mdMin = 1e-6;
    doubleProp.mdMax = 0.5;
    auto propQuality = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Quality Level", propId, QMetaType::Double, doubleProp, "Detection");
    mvProperty.push_back(propQuality);
    mMapIdToProperty[propId] = propQuality;

    propId = "min_distance";
    doubleProp.mdValue = mParams.mdMinDistance;
    doubleProp.mdMin = 0.0;
    doubleProp.mdMax = 200.0;
    auto propMinDist = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Min Distance", propId, QMetaType::Double, doubleProp, "Detection");
    mvProperty.push_back(propMinDist);
    mMapIdToProperty[propId] = propMinDist;

    propId = "block_size";
    intProp.miValue = mParams.miBlockSize;
    intProp.miMin = 1;
    intProp.miMax = 31;
    auto propBlock = std::make_shared<TypedProperty<IntPropertyType>>(
        "Block Size", propId, QMetaType::Int, intProp, "Detection");
    mvProperty.push_back(propBlock);
    mMapIdToProperty[propId] = propBlock;

    // Tracking
    propId = "win_size_width";
    intProp.miValue = mParams.miWinSizeWidth;
    intProp.miMin = 3;
    intProp.miMax = 200;
    auto propWinW = std::make_shared<TypedProperty<IntPropertyType>>(
        "Window Width", propId, QMetaType::Int, intProp, "Tracking");
    mvProperty.push_back(propWinW);
    mMapIdToProperty[propId] = propWinW;

    propId = "win_size_height";
    intProp.miValue = mParams.miWinSizeHeight;
    intProp.miMin = 3;
    intProp.miMax = 200;
    auto propWinH = std::make_shared<TypedProperty<IntPropertyType>>(
        "Window Height", propId, QMetaType::Int, intProp, "Tracking");
    mvProperty.push_back(propWinH);
    mMapIdToProperty[propId] = propWinH;

    propId = "max_level";
    intProp.miValue = mParams.miMaxLevel;
    intProp.miMin = 0;
    intProp.miMax = 10;
    auto propMaxLvl = std::make_shared<TypedProperty<IntPropertyType>>(
        "Max Level", propId, QMetaType::Int, intProp, "Tracking");
    mvProperty.push_back(propMaxLvl);
    mMapIdToProperty[propId] = propMaxLvl;

    propId = "max_count";
    intProp.miValue = mParams.miMaxCount;
    intProp.miMin = 1;
    intProp.miMax = 200;
    auto propMaxCnt = std::make_shared<TypedProperty<IntPropertyType>>(
        "Max Iterations", propId, QMetaType::Int, intProp, "Tracking");
    mvProperty.push_back(propMaxCnt);
    mMapIdToProperty[propId] = propMaxCnt;

    propId = "epsilon";
    doubleProp.mdValue = mParams.mdEpsilon;
    doubleProp.mdMin = 1e-6;
    doubleProp.mdMax = 1.0;
    auto propEps = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Epsilon", propId, QMetaType::Double, doubleProp, "Tracking");
    mvProperty.push_back(propEps);
    mMapIdToProperty[propId] = propEps;

    propId = "min_eig_threshold";
    doubleProp.mdValue = mParams.mdMinEigThreshold;
    doubleProp.mdMin = 0.0;
    doubleProp.mdMax = 1e-1;
    auto propEig = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Min Eig Threshold", propId, QMetaType::Double, doubleProp, "Tracking");
    mvProperty.push_back(propEig);
    mMapIdToProperty[propId] = propEig;

    propId = "flags";
    intProp.miValue = mParams.miFlags;
    intProp.miMin = 0;
    intProp.miMax = 1024;
    auto propFlags = std::make_shared<TypedProperty<IntPropertyType>>(
        "Flags", propId, QMetaType::Int, intProp, "Tracking");
    mvProperty.push_back(propFlags);
    mMapIdToProperty[propId] = propFlags;

    // Display
    propId = "draw_tracks";
    auto propDraw = std::make_shared<TypedProperty<bool>>(
        "Draw Tracks", propId, QMetaType::Bool, mParams.mbDrawTracks, "Display");
    mvProperty.push_back(propDraw);
    mMapIdToProperty[propId] = propDraw;

    propId = "motion_scale";
    doubleProp.mdValue = mParams.mdMotionScale;
    doubleProp.mdMin = 0.1;
    doubleProp.mdMax = 10.0;
    auto propScale = std::make_shared<TypedProperty<DoublePropertyType>>(
        "Motion Scale", propId, QMetaType::Double, doubleProp, "Display");
    mvProperty.push_back(propScale);
    mMapIdToProperty[propId] = propScale;

    propId = "draw_arrows";
    auto propArrows = std::make_shared<TypedProperty<bool>>(
        "Draw Arrows", propId, QMetaType::Bool, mParams.mbDrawArrows, "Display");
    mvProperty.push_back(propArrows);
    mMapIdToProperty[propId] = propArrows;

    propId = "track_color_b";
    intProp.miValue = mParams.miTrackColorB;
    intProp.miMin = 0;
    intProp.miMax = 255;
    auto propB = std::make_shared<TypedProperty<IntPropertyType>>(
        "Track Color B", propId, QMetaType::Int, intProp, "Display");
    mvProperty.push_back(propB);
    mMapIdToProperty[propId] = propB;

    propId = "track_color_g";
    intProp.miValue = mParams.miTrackColorG;
    intProp.miMin = 0;
    intProp.miMax = 255;
    auto propG = std::make_shared<TypedProperty<IntPropertyType>>(
        "Track Color G", propId, QMetaType::Int, intProp, "Display");
    mvProperty.push_back(propG);
    mMapIdToProperty[propId] = propG;

    propId = "track_color_r";
    intProp.miValue = mParams.miTrackColorR;
    intProp.miMin = 0;
    intProp.miMax = 255;
    auto propR = std::make_shared<TypedProperty<IntPropertyType>>(
        "Track Color R", propId, QMetaType::Int, intProp, "Display");
    mvProperty.push_back(propR);
    mMapIdToProperty[propId] = propR;

    propId = "track_thickness";
    intProp.miValue = mParams.miTrackThickness;
    intProp.miMin = 1;
    intProp.miMax = 20;
    auto propThick = std::make_shared<TypedProperty<IntPropertyType>>(
        "Track Thickness", propId, QMetaType::Int, intProp, "Display");
    mvProperty.push_back(propThick);
    mMapIdToProperty[propId] = propThick;
    
    qRegisterMetaType<CVOpticalFlowPyrLKParameters>("CVOpticalFlowPyrLKParameters");
}

QObject* CVOpticalFlowPyrLKModel::createWorker()
{
    return new CVOpticalFlowPyrLKWorker();
}

void CVOpticalFlowPyrLKModel::connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVOpticalFlowPyrLKWorker*>(worker);
    if (w)
    {
        connect(w, &CVOpticalFlowPyrLKWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void CVOpticalFlowPyrLKModel::dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;

    cv::Mat currentFrame = mPendingCurrentFrame;
    cv::Mat previousFrame = mPendingPreviousFrame;
    CVOpticalFlowPyrLKParameters params = mPendingParams;
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
                              Q_ARG(CVOpticalFlowPyrLKParameters, params),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

void CVOpticalFlowPyrLKModel::process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;

    cv::Mat currentFrame = mpCVImageInData->data();

    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });

    if (!mbHasPreviousFrame)
    {
        mPreviousFrame = currentFrame.clone();
        mbHasPreviousFrame = true;
        return;
    }

    if (isWorkerBusy())
    {
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

        CVOpticalFlowPyrLKParameters params = mParams;
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                      Qt::QueuedConnection,
                      Q_ARG(cv::Mat, currentFrame.clone()),
                      Q_ARG(cv::Mat, mPreviousFrame.clone()),
                      Q_ARG(CVOpticalFlowPyrLKParameters, params),
                      Q_ARG(FrameSharingMode, getSharingMode()),
                      Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                      Q_ARG(long, frameId),
                      Q_ARG(QString, producerId));
    }

    // Store current as previous for next iteration
    mPreviousFrame = currentFrame.clone();
}

void CVOpticalFlowPyrLKModel::setModelProperty(QString& id, const QVariant& value)
{
    if (!mMapIdToProperty.contains(id))
        return;

    if (id == "auto_detect")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typed->getData() = value.toBool();
        mParams.mbAutoDetectFeatures = value.toBool();
    }
    else if (id == "max_corners")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miMaxCorners = value.toInt();
    }
    else if (id == "quality_level")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdQualityLevel = value.toDouble();
    }
    else if (id == "min_distance")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdMinDistance = value.toDouble();
    }
    else if (id == "block_size")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miBlockSize = value.toInt();
    }
    else if (id == "win_size_width")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miWinSizeWidth = value.toInt();
    }
    else if (id == "win_size_height")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miWinSizeHeight = value.toInt();
    }
    else if (id == "max_level")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miMaxLevel = value.toInt();
    }
    else if (id == "max_count")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miMaxCount = value.toInt();
    }
    else if (id == "epsilon")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdEpsilon = value.toDouble();
    }
    else if (id == "min_eig_threshold")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdMinEigThreshold = value.toDouble();
    }
    else if (id == "flags")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miFlags = value.toInt();
    }
    else if (id == "draw_tracks")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typed->getData() = value.toBool();
        mParams.mbDrawTracks = value.toBool();
    }
    else if (id == "motion_scale")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdMotionScale = value.toDouble();
    }
    else if (id == "draw_arrows")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typed->getData() = value.toBool();
        mParams.mbDrawArrows = value.toBool();
    }
    else if (id == "track_color_b")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miTrackColorB = value.toInt();
    }
    else if (id == "track_color_g")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miTrackColorG = value.toInt();
    }
    else if (id == "track_color_r")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miTrackColorR = value.toInt();
    }
    else if (id == "track_thickness")
    {
        auto prop = mMapIdToProperty[id];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miTrackThickness = value.toInt();
    }
    else
    {
        PBAsyncDataModel::setModelProperty(id, value);
        return;
    }

    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}

QJsonObject CVOpticalFlowPyrLKModel::save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["autoDetect"] = mParams.mbAutoDetectFeatures;
    cParams["maxCorners"] = mParams.miMaxCorners;
    cParams["qualityLevel"] = mParams.mdQualityLevel;
    cParams["minDistance"] = mParams.mdMinDistance;
    cParams["blockSize"] = mParams.miBlockSize;
    cParams["winSizeWidth"] = mParams.miWinSizeWidth;
    cParams["winSizeHeight"] = mParams.miWinSizeHeight;
    cParams["maxLevel"] = mParams.miMaxLevel;
    cParams["maxCount"] = mParams.miMaxCount;
    cParams["epsilon"] = mParams.mdEpsilon;
    cParams["flags"] = mParams.miFlags;
    cParams["minEigThreshold"] = mParams.mdMinEigThreshold;
    cParams["drawTracks"] = mParams.mbDrawTracks;
    cParams["motionScale"] = mParams.mdMotionScale;
    cParams["drawArrows"] = mParams.mbDrawArrows;
    cParams["trackColorB"] = mParams.miTrackColorB;
    cParams["trackColorG"] = mParams.miTrackColorG;
    cParams["trackColorR"] = mParams.miTrackColorR;
    cParams["trackThickness"] = mParams.miTrackThickness;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void CVOpticalFlowPyrLKModel::load(const QJsonObject& json)
{
    PBAsyncDataModel::load(json);

    QJsonObject paramsObj = json["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v = paramsObj["autoDetect"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["auto_detect"]; auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop); typed->getData() = v.toBool(); mParams.mbAutoDetectFeatures = v.toBool(); }

        v = paramsObj["maxCorners"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["max_corners"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miMaxCorners = v.toInt(); }

        v = paramsObj["qualityLevel"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["quality_level"]; auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop); typed->getData().mdValue = v.toDouble(); mParams.mdQualityLevel = v.toDouble(); }

        v = paramsObj["minDistance"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["min_distance"]; auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop); typed->getData().mdValue = v.toDouble(); mParams.mdMinDistance = v.toDouble(); }

        v = paramsObj["blockSize"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["block_size"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miBlockSize = v.toInt(); }

        v = paramsObj["winSizeWidth"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["win_size_width"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miWinSizeWidth = v.toInt(); }

        v = paramsObj["winSizeHeight"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["win_size_height"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miWinSizeHeight = v.toInt(); }

        v = paramsObj["maxLevel"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["max_level"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miMaxLevel = v.toInt(); }

        v = paramsObj["maxCount"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["max_count"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miMaxCount = v.toInt(); }

        v = paramsObj["epsilon"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["epsilon"]; auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop); typed->getData().mdValue = v.toDouble(); mParams.mdEpsilon = v.toDouble(); }

        v = paramsObj["flags"]; if (!v.isNull()) { mParams.miFlags = v.toInt(); auto prop = mMapIdToProperty["flags"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = mParams.miFlags; }

        v = paramsObj["minEigThreshold"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["min_eig_threshold"]; auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop); typed->getData().mdValue = v.toDouble(); mParams.mdMinEigThreshold = v.toDouble(); }

        v = paramsObj["drawTracks"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["draw_tracks"]; auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop); typed->getData() = v.toBool(); mParams.mbDrawTracks = v.toBool(); }

        v = paramsObj["motionScale"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["motion_scale"]; auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop); typed->getData().mdValue = v.toDouble(); mParams.mdMotionScale = v.toDouble(); }

        v = paramsObj["drawArrows"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["draw_arrows"]; auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop); typed->getData() = v.toBool(); mParams.mbDrawArrows = v.toBool(); }

        v = paramsObj["trackColorB"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["track_color_b"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miTrackColorB = v.toInt(); }

        v = paramsObj["trackColorG"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["track_color_g"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miTrackColorG = v.toInt(); }

        v = paramsObj["trackColorR"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["track_color_r"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miTrackColorR = v.toInt(); }

        v = paramsObj["trackThickness"]; if (!v.isNull()) {
            auto prop = mMapIdToProperty["track_thickness"]; auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop); typed->getData().miValue = v.toInt(); mParams.miTrackThickness = v.toInt(); }
    }


    }


