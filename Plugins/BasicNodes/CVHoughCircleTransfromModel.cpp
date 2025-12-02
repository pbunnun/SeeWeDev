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

#include "CVHoughCircleTransfromModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QDebug>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVHoughCircleTransformModel::_category = QString( "Image Processing" );

const QString CVHoughCircleTransformModel::_model_name = QString( "CV Hough Circle" );

const std::string CVHoughCircleTransformModel::color[3] = {"B", "G", "R"};

void CVHoughCircleTransformWorker::
    processFrame(cv::Mat input,
                 int houghMethod,
                 double inverseRatio,
                 double centerDistance,
                 double thresholdU,
                 double thresholdL,
                 int radiusMin,
                 int radiusMax,
                 bool displayPoint,
                 unsigned char pointColorB,
                 unsigned char pointColorG,
                 unsigned char pointColorR,
                 int pointSize,
                 bool displayCircle,
                 unsigned char circleColorB,
                 unsigned char circleColorG,
                 unsigned char circleColorR,
                 int circleThickness,
                 int circleType,
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

    // Detect circles
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(input,
                     circles,
                     houghMethod,
                     inverseRatio,
                     centerDistance,
                     thresholdU,
                     thresholdL,
                     radiusMin,
                     radiusMax);

    // Create output image data
    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    
    if (mode == FrameSharingMode::PoolMode && pool)
    {
        auto handle = pool->acquire(3, metadata); // 3 channels for BGR
        if (handle)
        {
            cv::cvtColor(input, handle.matrix(), cv::COLOR_GRAY2BGR);
            
            // Draw circles
            for (const cv::Vec3f& circle : circles)
            {
                if (displayPoint)
                {
                    cv::circle(handle.matrix(),
                              cv::Point(static_cast<int>(circle[0]), static_cast<int>(circle[1])),
                              1,
                              cv::Scalar(pointColorB, pointColorG, pointColorR),
                              pointSize,
                              cv::LINE_8);
                }
                if (displayCircle)
                {
                    cv::circle(handle.matrix(),
                              cv::Point(static_cast<int>(circle[0]), static_cast<int>(circle[1])),
                              static_cast<int>(circle[2]),
                              cv::Scalar(circleColorB, circleColorG, circleColorR),
                              circleThickness,
                              circleType);
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
        
        // Draw circles
        for (const cv::Vec3f& circle : circles)
        {
            if (displayPoint)
            {
                cv::circle(result,
                          cv::Point(static_cast<int>(circle[0]), static_cast<int>(circle[1])),
                          1,
                          cv::Scalar(pointColorB, pointColorG, pointColorR),
                          pointSize,
                          cv::LINE_8);
            }
            if (displayCircle)
            {
                cv::circle(result,
                          cv::Point(static_cast<int>(circle[0]), static_cast<int>(circle[1])),
                          static_cast<int>(circle[2]),
                          cv::Scalar(circleColorB, circleColorG, circleColorR),
                          circleThickness,
                          circleType);
            }
        }
        
        if (result.empty())
        {
            Q_EMIT frameReady(nullptr, nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }

    // Create output count data
    auto countData = std::make_shared<IntegerData>(static_cast<int>(circles.size()));
    
    Q_EMIT frameReady(newImageData, countData);
}

CVHoughCircleTransformModel::
CVHoughCircleTransformModel()
    : PBAsyncDataModel( _model_name ),
      _minPixmap( ":CVHoughCircleTransform.png" )
{
    mpIntegerData = std::make_shared< IntegerData >( int() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"HOUGH_GRADIENT", "HOUGH_STANDARD", "HOUGH_MULTI_SCALE", "HOUGH_GRADIENT_ALT", "HOUGH_PROBABILISTIC"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "hough_method";
    auto propHoughMethod = std::make_shared< TypedProperty< EnumPropertyType > >( "Method", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propHoughMethod );
    mMapIdToProperty[ propId ] = propHoughMethod;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdInverseRatio;
    propId = "inverse_ratio";
    auto propInverseRatio = std::make_shared< TypedProperty< DoublePropertyType > >( "Resolution Inverse Ratio", propId, QMetaType::Double, doublePropertyType, "Operation" );
    mvProperty.push_back( propInverseRatio );
    mMapIdToProperty[ propId ] = propInverseRatio;

    doublePropertyType.mdValue = mParams.mdCenterDistance;
    propId = "center_distance";
    auto propCenterDistance = std::make_shared< TypedProperty< DoublePropertyType > >( "Minimum Center Distance", propId, QMetaType::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propCenterDistance );
    mMapIdToProperty[ propId ] = propCenterDistance;

    doublePropertyType.mdValue = mParams.mdThresholdU;
    propId = "th_u";
    auto propThresholdU = std::make_shared< TypedProperty< DoublePropertyType > >( "Upper Threshold", propId, QMetaType::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propThresholdU );
    mMapIdToProperty[ propId ] = propThresholdU;

    doublePropertyType.mdValue = mParams.mdThresholdL;
    propId = "th_l";
    auto propThresholdL = std::make_shared< TypedProperty< DoublePropertyType > >( "Lower Threshold", propId, QMetaType::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propThresholdL );
    mMapIdToProperty[ propId ] = propThresholdL;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miRadiusMin;
    propId = "radius_min";
    auto propRadiusMin = std::make_shared<TypedProperty<IntPropertyType>>("Minimum Radius", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propRadiusMin);
    mMapIdToProperty[ propId ] = propRadiusMin;

    intPropertyType.miValue = mParams.miRadiusMax;
    propId = "radius_max";
    auto propRadiusMax = std::make_shared<TypedProperty<IntPropertyType>>("Maximum Radius", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propRadiusMax);
    mMapIdToProperty[ propId ] = propRadiusMax;

    propId = "display_point";
    auto propDisplayPoint = std::make_shared< TypedProperty < bool > > ("Display Points", propId, QMetaType::Bool, mParams.mbDisplayPoint, "Display");
    mvProperty.push_back( propDisplayPoint );
    mMapIdToProperty[ propId ] = propDisplayPoint;

    UcharPropertyType ucharPropertyType;
    for(int i=0; i<3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucPointColor[i];
        propId = QString("point_color_%1").arg(i);
        QString pointColor = QString::fromStdString("Point Color "+color[i]);
        auto propPointColor = std::make_shared<TypedProperty<UcharPropertyType>>(pointColor, propId, QMetaType::Int, ucharPropertyType, "Display");
        mvProperty.push_back(propPointColor);
        mMapIdToProperty[ propId ] = propPointColor;
    }

    intPropertyType.miValue = mParams.miPointSize;
    propId = "point_size";
    auto propPointSize = std::make_shared<TypedProperty<IntPropertyType>>("Point Size", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back( propPointSize );
    mMapIdToProperty[ propId ] = propPointSize;

    propId = "display_circle";
    auto propDisplayCircle = std::make_shared<TypedProperty<bool>>("Display Circle", propId, QMetaType::Bool, mParams.mbDisplayCircle, "Display");
    mvProperty.push_back(propDisplayCircle);
    mMapIdToProperty[ propId ] = propDisplayCircle;

    for(int i=0; i<3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucCircleColor[i];
        propId = QString("circle_color_%1").arg(i);
        QString circleColor = QString::fromStdString("Circle Color "+color[i]);
        auto propCircleColor = std::make_shared<TypedProperty<UcharPropertyType>>(circleColor, propId, QMetaType::Int, ucharPropertyType, "Display");
        mvProperty.push_back(propCircleColor);
        mMapIdToProperty[ propId ] = propCircleColor;
    }

    intPropertyType.miValue = mParams.miCircleThickness;
    propId = "circle_thickness";
    auto propCircleThickness = std::make_shared<TypedProperty<IntPropertyType>>("Circle Thickness", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back( propCircleThickness );
    mMapIdToProperty[ propId ] = propCircleThickness;

    enumPropertyType.mslEnumNames = QStringList({"LINE_8", "LINE_4", "LINE_AA"});
    enumPropertyType.miCurrentIndex = 2;
    propId = "circle_type";
    auto propCircleType = std::make_shared<TypedProperty<EnumPropertyType>>("Circle Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display");
    mvProperty.push_back( propCircleType );
    mMapIdToProperty[ propId ] = propCircleType;
}

QObject*
CVHoughCircleTransformModel::
createWorker()
{
    return new CVHoughCircleTransformWorker();
}

void
CVHoughCircleTransformModel::
connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVHoughCircleTransformWorker*>(worker);
    if (w) {
        connect(w, &CVHoughCircleTransformWorker::frameReady,
                this, [this](std::shared_ptr<CVImageData> img, std::shared_ptr<IntegerData> count) {
                    // Handle both outputs
                    mpCVImageData = img;
                    mpIntegerData = count;
                    
                    // Update port 0 (image)
                    Q_EMIT dataUpdated(0);
                    // Update port 1 (count)
                    Q_EMIT dataUpdated(1);
                    // Update sync port
                    mpSyncData->data() = true;
                    Q_EMIT dataUpdated(2);
                    
                    setWorkerBusy(false);
                    dispatchPendingWork();
                },
                Qt::QueuedConnection);
    }
}

unsigned int
CVHoughCircleTransformModel::
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
CVHoughCircleTransformModel::
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
CVHoughCircleTransformModel::
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
CVHoughCircleTransformModel::
dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;

    cv::Mat input = mPendingFrame;
    CVHoughCircleTransformParameters params = mPendingParams;
    setPendingWork(false);

    ensure_frame_pool(input.cols, input.rows, CV_8UC3); // 3 channels for output BGR

    long frameId = getNextFrameId();
    QString producerId = getNodeId();

    std::shared_ptr<CVImagePool> poolCopy = getFramePool();

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(int, params.miHoughMethod),
                              Q_ARG(double, params.mdInverseRatio),
                              Q_ARG(double, params.mdCenterDistance),
                              Q_ARG(double, params.mdThresholdU),
                              Q_ARG(double, params.mdThresholdL),
                              Q_ARG(int, params.miRadiusMin),
                              Q_ARG(int, params.miRadiusMax),
                              Q_ARG(bool, params.mbDisplayPoint),
                              Q_ARG(unsigned char, params.mucPointColor[0]),
                              Q_ARG(unsigned char, params.mucPointColor[1]),
                              Q_ARG(unsigned char, params.mucPointColor[2]),
                              Q_ARG(int, params.miPointSize),
                              Q_ARG(bool, params.mbDisplayCircle),
                              Q_ARG(unsigned char, params.mucCircleColor[0]),
                              Q_ARG(unsigned char, params.mucCircleColor[1]),
                              Q_ARG(unsigned char, params.mucCircleColor[2]),
                              Q_ARG(int, params.miCircleThickness),
                              Q_ARG(int, params.miCircleType),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

QJsonObject
CVHoughCircleTransformModel::
save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["houghMethod"] = mParams.miHoughMethod;
    cParams["inverseRatio"] = mParams.mdInverseRatio;
    cParams["centerDistance"] = mParams.mdCenterDistance;
    cParams["thresholdU"] = mParams.mdThresholdU;
    cParams["thresholdL"] = mParams.mdThresholdL;
    cParams["radiusMin"] = mParams.miRadiusMin;
    cParams["radiusMax"] = mParams.miRadiusMax;
    cParams["displayPoint"] = mParams.mbDisplayPoint;
    for(int i=0; i<3; i++)
    {
        cParams[QString("pointColor%1").arg(i)] = mParams.mucPointColor[i];
    }
    cParams["pointSize"] = mParams.miPointSize;
    cParams["displayCircle"] = mParams.mbDisplayCircle;
    for(int i=0; i<3; i++)
    {
        cParams[QString("circleColor%1").arg(i)] = mParams.mucCircleColor[i];
    }
    cParams["circleThickness"] = mParams.miCircleThickness;
    cParams["circleType"] = mParams.miCircleType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVHoughCircleTransformModel::
load(QJsonObject const& p)
{
    PBAsyncDataModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "houghMethod" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "hough_method" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miHoughMethod = v.toInt();
        }
        v = paramsObj[ "inverseRatio" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "inverse_ratio" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdInverseRatio = v.toDouble();
        }
        v = paramsObj[ "centerDistance" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "center_distance" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdCenterDistance = v.toDouble();
        }
        v =  paramsObj[ "thresholdU" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "th_u" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdThresholdU = v.toDouble();
        }
        v =  paramsObj[ "thresholdL" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "th_l" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdThresholdL = v.toDouble();
        }
        v =  paramsObj[ "radiusMin" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "radius_min" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miRadiusMin = v.toInt();
        }
        v =  paramsObj[ "radiusMax" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "radius_max" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miRadiusMax = v.toInt();
        }
        v = paramsObj[ "displayPoint" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "display_point" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < bool > > (prop);
            typedProp->getData() = v.toBool();

            mParams.mbDisplayPoint = v.toBool();
        }
        for(int i=0; i<3; i++)
        {
            v = paramsObj[QString("pointColor%1").arg(i)];
            if( !v.isNull() )
            {
                auto prop = mMapIdToProperty[QString("point_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
                typedProp->getData().mucValue = v.toInt();

                mParams.mucPointColor[i] = v.toInt();
            }
        }
        v = paramsObj[ "pointSize" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "point_size" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miPointSize = v.toInt();
        }
        v = paramsObj[ "displayCircle" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "display_circle" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < bool > > (prop);
            typedProp->getData() = v.toBool();

            mParams.mbDisplayCircle = v.toBool();
        }
        for(int i=0; i<3; i++)
        {
            v = paramsObj[QString("circleColor%1").arg(i)];
            if( !v.isNull() )
            {
                auto prop = mMapIdToProperty[QString("circle_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
                typedProp->getData().mucValue = v.toInt();

                mParams.mucCircleColor[i] = v.toInt();
            }
        }
        v = paramsObj[ "circleThickness" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "circle_thickness" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miCircleThickness = v.toInt();
        }
        v = paramsObj[ "circleType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "circle_type" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miCircleType = v.toInt();
        }
    }
}

void
CVHoughCircleTransformModel::
setModelProperty( QString & id, const QVariant & value )
{
    if( !mMapIdToProperty.contains( id ) )
    {
        // Base class handles pool_size and sharing_mode
        PBAsyncDataModel::setModelProperty(id, value);
        return;
    }

    auto prop = mMapIdToProperty[ id ];
    if( id == "hough_method" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miHoughMethod = cv::HOUGH_GRADIENT;
            break;

        case 1:
            mParams.miHoughMethod = cv::HOUGH_STANDARD;
            break;

        case 2:
            mParams.miHoughMethod = cv::HOUGH_MULTI_SCALE;
            break;

        case 3:
#if CV_MINOR_VERSION > 2
            mParams.miHoughMethod = cv::HOUGH_GRADIENT_ALT;
            break;
#endif
        case 4:
            mParams.miHoughMethod = cv::HOUGH_PROBABILISTIC;
            break;
        }
    }
    else if( id == "inverse_ratio" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdInverseRatio = value.toDouble();
    }
    else if( id == "center_distance" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdCenterDistance = value.toDouble();
    }
    else if( id == "th_u" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdThresholdU = value.toDouble();
    }
    else if( id == "th_l" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdThresholdL = value.toDouble();
    }
    else if( id == "radius_min" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miRadiusMin = value.toInt();
    }
    else if( id == "radius_max" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miRadiusMax = value.toInt();
    }
    else if( id == "display_point" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbDisplayPoint = value.toBool();
    }
    for(int i=0; i<3; i++)
    {
        if(id==QString("point_color_%1").arg(i))
        {
            auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typedProp->getData().mucValue = value.toInt();

            mParams.mucPointColor[i] = value.toInt();
        }
    }
    if( id == "point_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();

        mParams.miPointSize = value.toInt();
    }
    else if( id == "display_circle" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbDisplayCircle = value.toBool();
    }
    for(int i=0; i<3; i++)
    {
        if(id==QString("circle_color_%1").arg(i))
        {
            auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typedProp->getData().mucValue = value.toInt();

            mParams.mucCircleColor[i] = value.toInt();
        }
    }
    if( id == "circle_thickness" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();

        mParams.miCircleThickness = value.toInt();
    }
    else if( id == "circle_type" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miCircleType = cv::LINE_8;
            break;

        case 1:
            mParams.miCircleType = cv::LINE_4;
            break;

        case 2:
            mParams.miCircleType = cv::LINE_AA;
            break;
        }
    }

    // Process cached input if available
    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}

void CVHoughCircleTransformModel::
    process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;
    
    cv::Mat input = mpCVImageInData->data();
    
    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(2);
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
        
        ensure_frame_pool(input.cols, input.rows, CV_8UC3); // 3 channels for output BGR
        
        long frameId = getNextFrameId();
        QString producerId = getNodeId();
        
        std::shared_ptr<CVImagePool> poolCopy = getFramePool();
        
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(int, mParams.miHoughMethod),
                                  Q_ARG(double, mParams.mdInverseRatio),
                                  Q_ARG(double, mParams.mdCenterDistance),
                                  Q_ARG(double, mParams.mdThresholdU),
                                  Q_ARG(double, mParams.mdThresholdL),
                                  Q_ARG(int, mParams.miRadiusMin),
                                  Q_ARG(int, mParams.miRadiusMax),
                                  Q_ARG(bool, mParams.mbDisplayPoint),
                                  Q_ARG(unsigned char, mParams.mucPointColor[0]),
                                  Q_ARG(unsigned char, mParams.mucPointColor[1]),
                                  Q_ARG(unsigned char, mParams.mucPointColor[2]),
                                  Q_ARG(int, mParams.miPointSize),
                                  Q_ARG(bool, mParams.mbDisplayCircle),
                                  Q_ARG(unsigned char, mParams.mucCircleColor[0]),
                                  Q_ARG(unsigned char, mParams.mucCircleColor[1]),
                                  Q_ARG(unsigned char, mParams.mucCircleColor[2]),
                                  Q_ARG(int, mParams.miCircleThickness),
                                  Q_ARG(int, mParams.miCircleType),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}
