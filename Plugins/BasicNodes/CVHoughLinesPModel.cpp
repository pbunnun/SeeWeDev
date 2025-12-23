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

#include "CVHoughLinesPModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QDebug>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVHoughLinesPModel::_category = QString("Image Processing");

const QString CVHoughLinesPModel::_model_name = QString("CV Hough Lines P");

const std::string CVHoughLinesPModel::color[3] = {"B", "G", "R"};

void CVHoughLinesPWorker::
    processFrame(cv::Mat input,
                 CVHoughLinesPParameters params,
                 FrameSharingMode mode,
                 std::shared_ptr<CVImagePool> pool,
                 long frameId,
                 QString producerId)
{
    if (input.empty() || input.type() != CV_8UC1)
    {
        Q_EMIT frameReady(nullptr, nullptr);
        return;
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    // Detect line segments using probabilistic Hough Transform
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(input, lines, params.mdRho, params.mdTheta, params.miThreshold, params.mdMinLineLength, params.mdMaxLineGap);

    // Create output image
    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    
    if (mode == FrameSharingMode::PoolMode && pool)
    {
        auto handle = pool->acquire(3, metadata); // 3 channels for BGR
        if (handle)
        {
            cv::cvtColor(input, handle.matrix(), cv::COLOR_GRAY2BGR);
            
            // Draw line segments
            if (params.mbDisplayLines)
            {
                for (const cv::Vec4i& line : lines)
                {
                    cv::line(handle.matrix(),
                        cv::Point(line[0], line[1]),
                        cv::Point(line[2], line[3]),
                        cv::Scalar(params.mucLineColor[0], params.mucLineColor[1], params.mucLineColor[2]),
                        params.miLineThickness, params.miLineType);
                }
            }
            
            if (!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    
    if (!pooled)
    {
        cv::Mat result;
        cv::cvtColor(input, result, cv::COLOR_GRAY2BGR);
        
        // Draw line segments
        if (params.mbDisplayLines)
        {
            for (const cv::Vec4i& line : lines)
            {
                cv::line(result,
                    cv::Point(line[0], line[1]),
                    cv::Point(line[2], line[3]),
                    cv::Scalar(params.mucLineColor[0], params.mucLineColor[1], params.mucLineColor[2]),
                    params.miLineThickness, params.miLineType);
            }
        }
        
        if (result.empty())
        {
            Q_EMIT frameReady(nullptr, nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }

    auto countData = std::make_shared<IntegerData>(static_cast<int>(lines.size()));
    Q_EMIT frameReady(newImageData, countData);
}

CVHoughLinesPModel::
CVHoughLinesPModel()
    : PBAsyncDataModel(_model_name),
      _minPixmap(":/HoughLinesPoint.png")
{
    mpIntegerData = std::make_shared<IntegerData>(int());

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdRho;
    QString propId = "rho";
    auto propRho = std::make_shared<TypedProperty<DoublePropertyType>>("Rho (Distance Resolution)", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propRho);
    mMapIdToProperty[propId] = propRho;

    doublePropertyType.mdValue = mParams.mdTheta * 180.0 / CV_PI; // Convert to degrees for UI
    propId = "theta";
    auto propTheta = std::make_shared<TypedProperty<DoublePropertyType>>("Theta (Angle Resolution °)", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propTheta);
    mMapIdToProperty[propId] = propTheta;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miThreshold;
    propId = "threshold";
    auto propThreshold = std::make_shared<TypedProperty<IntPropertyType>>("Threshold", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propThreshold);
    mMapIdToProperty[propId] = propThreshold;

    doublePropertyType.mdValue = mParams.mdMinLineLength;
    propId = "min_line_length";
    auto propMinLineLength = std::make_shared<TypedProperty<DoublePropertyType>>("Min Line Length", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propMinLineLength);
    mMapIdToProperty[propId] = propMinLineLength;

    doublePropertyType.mdValue = mParams.mdMaxLineGap;
    propId = "max_line_gap";
    auto propMaxLineGap = std::make_shared<TypedProperty<DoublePropertyType>>("Max Line Gap", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propMaxLineGap);
    mMapIdToProperty[propId] = propMaxLineGap;

    propId = "display_lines";
    auto propDisplayLines = std::make_shared<TypedProperty<bool>>("Display Lines", propId, QMetaType::Bool, mParams.mbDisplayLines, "Display");
    mvProperty.push_back(propDisplayLines);
    mMapIdToProperty[propId] = propDisplayLines;

    UcharPropertyType ucharPropertyType;
    for (int i = 0; i < 3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucLineColor[i];
        propId = QString("line_color_%1").arg(i);
        QString lineColor = QString::fromStdString("Line Color " + color[i]);
        auto propLineColor = std::make_shared<TypedProperty<UcharPropertyType>>(lineColor, propId, QMetaType::Int, ucharPropertyType, "Display");
        mvProperty.push_back(propLineColor);
        mMapIdToProperty[propId] = propLineColor;
    }

    intPropertyType.miValue = mParams.miLineThickness;
    propId = "line_thickness";
    auto propLineThickness = std::make_shared<TypedProperty<IntPropertyType>>("Line Thickness", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back(propLineThickness);
    mMapIdToProperty[propId] = propLineThickness;

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"LINE_8", "LINE_4", "LINE_AA"});
    enumPropertyType.miCurrentIndex = 2;
    propId = "line_type";
    auto propLineType = std::make_shared<TypedProperty<EnumPropertyType>>("Line Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display");
    mvProperty.push_back(propLineType);
    mMapIdToProperty[propId] = propLineType;

    qRegisterMetaType<CVHoughLinesPParameters>("CVHoughLinesPParameters");
}

QObject*
CVHoughLinesPModel::
createWorker()
{
    return new CVHoughLinesPWorker();
}

void
CVHoughLinesPModel::
connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVHoughLinesPWorker*>(worker);
    if (w) {
        connect(w, &CVHoughLinesPWorker::frameReady,
                this, [this](std::shared_ptr<CVImageData> img, std::shared_ptr<IntegerData> count) {
                    mpCVImageData = img;
                    mpIntegerData = count;
                    
                    Q_EMIT dataUpdated(0); // image
                    Q_EMIT dataUpdated(1); // count
                    mpSyncData->data() = true;
                    Q_EMIT dataUpdated(2); // sync
                    
                    setWorkerBusy(false);
                    dispatchPendingWork();
                },
                Qt::QueuedConnection);
    }
}

unsigned int
CVHoughLinesPModel::
nPorts(PortType portType) const
{
    switch (portType)
    {
    case PortType::In:
        return 2; // image + sync
    case PortType::Out:
        return 3; // image + count + sync
    default:
        return 0;
    }
}

NodeDataType
CVHoughLinesPModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out)
    {
        if (portIndex == 0)
            return CVImageData().type();
        else if (portIndex == 1)
            return IntegerData().type();
        else if (portIndex == 2)
            return SyncData().type();
    }
    else if (portType == PortType::In)
    {
        if (portIndex == 0)
            return CVImageData().type();
        else if (portIndex == 1)
            return SyncData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
CVHoughLinesPModel::
outData(PortIndex port)
{
    if (port == 0 && mpCVImageData) {
        return mpCVImageData;
    } else if (port == 1 && mpIntegerData) {
        return mpIntegerData;
    } else if (port == 2 && mpSyncData) {
        return mpSyncData;
    }
    return nullptr;
}

void
CVHoughLinesPModel::
dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;

    cv::Mat input = mPendingFrame;
    CVHoughLinesPParameters params = mPendingParams;
    setPendingWork(false);

    ensure_frame_pool(input.cols, input.rows, CV_8UC3);

    long frameId = getNextFrameId();
    QString producerId = getNodeId();

    std::shared_ptr<CVImagePool> poolCopy = getFramePool();

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(CVHoughLinesPParameters, params),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

QJsonObject
CVHoughLinesPModel::
save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["rho"] = mParams.mdRho;
    cParams["theta"] = mParams.mdTheta;
    cParams["threshold"] = mParams.miThreshold;
    cParams["minLineLength"] = mParams.mdMinLineLength;
    cParams["maxLineGap"] = mParams.mdMaxLineGap;
    cParams["displayLines"] = mParams.mbDisplayLines;
    for (int i = 0; i < 3; i++)
    {
        cParams[QString("lineColor%1").arg(i)] = mParams.mucLineColor[i];
    }
    cParams["lineThickness"] = mParams.miLineThickness;
    cParams["lineType"] = mParams.miLineType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVHoughLinesPModel::
load(QJsonObject const& p)
{
    PBAsyncDataModel::load(p);

    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v = paramsObj["rho"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["rho"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdRho = v.toDouble();
        }
        v = paramsObj["theta"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["theta"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            double degrees = v.toDouble();
            typedProp->getData().mdValue = degrees;
            mParams.mdTheta = degrees * CV_PI / 180.0;
        }
        v = paramsObj["threshold"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["threshold"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            mParams.miThreshold = v.toInt();
        }
        v = paramsObj["minLineLength"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["min_line_length"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdMinLineLength = v.toDouble();
        }
        v = paramsObj["maxLineGap"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["max_line_gap"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = v.toDouble();
            mParams.mdMaxLineGap = v.toDouble();
        }
        v = paramsObj["displayLines"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["display_lines"];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();
            mParams.mbDisplayLines = v.toBool();
        }
        for (int i = 0; i < 3; i++)
        {
            v = paramsObj[QString("lineColor%1").arg(i)];
            if (!v.isNull())
            {
                auto prop = mMapIdToProperty[QString("line_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
                typedProp->getData().mucValue = v.toInt();
                mParams.mucLineColor[i] = v.toInt();
            }
        }
        v = paramsObj["lineThickness"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["line_thickness"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            mParams.miLineThickness = v.toInt();
        }
        v = paramsObj["lineType"];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty["line_type"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miLineType = v.toInt();
        }
    }
}

void
CVHoughLinesPModel::
setModelProperty(QString& id, const QVariant& value)
{
    if (!mMapIdToProperty.contains(id))
    {
        PBAsyncDataModel::setModelProperty(id, value);
        return;
    }

    auto prop = mMapIdToProperty[id];
    
    if (id == "rho")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdRho = value.toDouble();
    }
    else if (id == "theta")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        double degrees = value.toDouble();
        typedProp->getData().mdValue = degrees;
        mParams.mdTheta = degrees * CV_PI / 180.0; // Convert to radians
    }
    else if (id == "threshold")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miThreshold = value.toInt();
    }
    else if (id == "min_line_length")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdMinLineLength = value.toDouble();
    }
    else if (id == "max_line_gap")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdMaxLineGap = value.toDouble();
    }
    else if (id == "display_lines")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typedProp->getData() = value.toBool();
        mParams.mbDisplayLines = value.toBool();
    }
    else if (id.startsWith("line_color_"))
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        typedProp->getData().mucValue = value.toInt();
        int idx = id.mid(11).toInt();
        if (idx >= 0 && idx < 3)
            mParams.mucLineColor[idx] = value.toInt();
    }
    else if (id == "line_thickness")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miLineThickness = value.toInt();
    }
    else if (id == "line_type")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        
        switch (value.toInt())
        {
        case 0:
            mParams.miLineType = cv::LINE_8;
            break;
        case 1:
            mParams.miLineType = cv::LINE_4;
            break;
        case 2:
            mParams.miLineType = cv::LINE_AA;
            break;
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

void
CVHoughLinesPModel::
process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;
    
    cv::Mat input = mpCVImageInData->data();
    
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(2);
    });
    
    if (isWorkerBusy())
    {
        mPendingFrame = input.clone();
        mPendingParams = mParams;
        setPendingWork(true);
    }
    else
    {
        setWorkerBusy(true);
        
        ensure_frame_pool(input.cols, input.rows, CV_8UC3);
        
        long frameId = getNextFrameId();
        QString producerId = getNodeId();
        
        std::shared_ptr<CVImagePool> poolCopy = getFramePool();
        
        CVHoughLinesPParameters params = mParams;
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(CVHoughLinesPParameters, params),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}
