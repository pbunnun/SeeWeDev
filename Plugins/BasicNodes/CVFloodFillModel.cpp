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

#include "CVFloodFillModel.hpp"

#include <QDebug> //for debugging using qDebug()
#include <QtCore/QTimer>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVFloodFillModel::_category = QString( "Image Modification" );

const QString CVFloodFillModel::_model_name = QString( "CV Flood Fill" );

const std::string CVFloodFillModel::color[4] = {"B","G","R","Gray"};

void CVFloodFillWorker::
    processFrame(cv::Mat input,
                 cv::Mat maskInput,
                 const CVFloodFillParameters &params,
                 FrameSharingMode mode,
                 std::shared_ptr<CVImagePool> pool,
                 long frameId,
                 QString producerId)
{
    if(input.empty() || (input.channels()!=1 && input.channels()!=3) ||
       (input.depth()!=CV_8U && input.depth()!=CV_8S && input.depth()!=CV_16F && input.depth()!=CV_32F && input.depth()!=CV_64F))
    {
        Q_EMIT frameReady(nullptr, nullptr);
        return;
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    auto newMaskData = std::make_shared<CVImageData>(cv::Mat());
    
    // Clone input to preserve original
    cv::Mat output = input.clone();
    
    // Convert parameters to OpenCV Scalar format
    cv::Scalar fillColor = cv::Scalar(params.mucFillColor[0], params.mucFillColor[1], params.mucFillColor[2]);
    cv::Scalar lowerDiff = cv::Scalar(params.mucLowerDiff[0], params.mucLowerDiff[1], params.mucLowerDiff[2]);
    cv::Scalar upperDiff = cv::Scalar(params.mucUpperDiff[0], params.mucUpperDiff[1], params.mucUpperDiff[2]);
    
    // Check if mask is valid
    bool hasMask = (params.mbActiveMask && !maskInput.empty() && maskInput.type()==CV_8UC1 
                    && maskInput.cols==input.cols+2 && maskInput.rows==input.rows+2);
    
    cv::Mat mask;
    if (hasMask)
    {
        mask = maskInput.clone();
    }
    
    // Perform flood fill - call appropriate overload based on mask availability
    if (params.mbDefineBoundaries)
    {
        cv::Rect rect(params.mCVPointRect1, params.mCVPointRect2);
        if (hasMask)
        {
            cv::floodFill(output, mask, params.mCVPointSeed, fillColor, &rect,
                         lowerDiff, upperDiff, params.miFlags | (params.miMaskColor << 8));
        }
        else
        {
            cv::floodFill(output, params.mCVPointSeed, fillColor, &rect,
                         lowerDiff, upperDiff, params.miFlags);
        }
    }
    else
    {
        if (hasMask)
        {
            cv::floodFill(output, mask, params.mCVPointSeed, fillColor, nullptr,
                         lowerDiff, upperDiff, params.miFlags | (params.miMaskColor << 8));
        }
        else
        {
            cv::floodFill(output, params.mCVPointSeed, fillColor,
                         0, lowerDiff, upperDiff, params.miFlags);
        }
    }
    
    if(output.empty())
    {
        Q_EMIT frameReady(nullptr, nullptr);
        return;
    }
    
    newImageData->updateMove(std::move(output), metadata);
    
    if (hasMask && !mask.empty())
    {
        newMaskData->updateMove(std::move(mask), metadata);
    }
    
    Q_EMIT frameReady(newImageData, hasMask ? newMaskData : nullptr);
}

CVFloodFillModel::
CVFloodFillModel()
    : PBAsyncDataModel( _model_name ),
      mpEmbeddedWidget(new CVFloodFillEmbeddedWidget),
      _minPixmap( ":FloodFill.png" )
{
    for(std::shared_ptr<CVImageData>& mp : mapCVImageData)
        mp = std::make_shared< CVImageData >( cv::Mat() );
    for(std::shared_ptr<CVImageData>& mp : mapCVImageInData)
        mp = std::make_shared< CVImageData >( cv::Mat() );

    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &CVFloodFillEmbeddedWidget::spinbox_clicked_signal, this, &CVFloodFillModel::em_spinbox_clicked );

    PointPropertyType pointPropertyType;
    pointPropertyType.miXPosition = mParams.mCVPointSeed.x;
    pointPropertyType.miYPosition = mParams.mCVPointSeed.y;
    pointPropertyType.miXMin = 0;
    pointPropertyType.miXMax = INT_MAX;
    pointPropertyType.miYMin = 0;
    pointPropertyType.miYMax = INT_MAX;
    QString propId = "seed_point";
    auto propSeedPoint = std::make_shared< TypedProperty< PointPropertyType > >( "Seed Point", propId, QMetaType::QPoint, pointPropertyType, "Operation");
    mvProperty.push_back( propSeedPoint );
    mMapIdToProperty[ propId ] = propSeedPoint;

    UcharPropertyType ucharPropertyType;
    for(int i=0; i<4; i++)
    {
        ucharPropertyType.mucValue = mParams.mucFillColor[i];
        propId = QString("fill_color_%1").arg(i);
        auto propFillColor = std::make_shared< TypedProperty< UcharPropertyType > >( QString::fromStdString("Fill Color "+color[i] ), propId, QMetaType::Int, ucharPropertyType, "Operation");
        mvProperty.push_back( propFillColor );
        mMapIdToProperty[ propId ] = propFillColor;

        ucharPropertyType.mucValue = mParams.mucLowerDiff[i];
        propId = QString("lower_diff_%1").arg(i);
        auto propLowerDiff = std::make_shared< TypedProperty< UcharPropertyType > >( QString::fromStdString("Lower Diff "+color[i] ), propId, QMetaType::Int, ucharPropertyType, "Operation");
        mMapIdToProperty[ propId ] = propLowerDiff;

        ucharPropertyType.mucValue = mParams.mucFillColor[i];
        propId = QString("upper_diff_%1").arg(i);
        auto propUpperDiff = std::make_shared< TypedProperty< UcharPropertyType > >( QString::fromStdString("Upper Diff "+color[i] ), propId, QMetaType::Int, ucharPropertyType, "Operation");
        mMapIdToProperty[ propId ] = propUpperDiff;
    }
    mpEmbeddedWidget->set_lower_upper(mParams.mucLowerDiff,mParams.mucUpperDiff);
    mpEmbeddedWidget->toggle_widgets(3);

    propId = "define_boundaries";
    auto propDefineBoundaries = std::make_shared< TypedProperty <bool> >("Define Boundaries", propId, QMetaType::Bool, mParams.mbDefineBoundaries, "Display");
    mvProperty.push_back( propDefineBoundaries );
    mMapIdToProperty[ propId ] = propDefineBoundaries;

    pointPropertyType.miXPosition = mParams.mCVPointRect1.x;
    pointPropertyType.miYPosition = mParams.mCVPointRect1.y;
    pointPropertyType.miXMin = 0;
    pointPropertyType.miXMax = INT_MAX;
    pointPropertyType.miYMin = 0;
    pointPropertyType.miYMax = INT_MAX;
    propId = "rect_point_1";
    auto propRectPoint1 = std::make_shared< TypedProperty< PointPropertyType > >( "Boundary Point 1", propId, QMetaType::QPoint, pointPropertyType, "Display");
    mvProperty.push_back( propRectPoint1 );
    mMapIdToProperty[ propId ] = propRectPoint1;

    pointPropertyType.miXPosition = mParams.mCVPointRect2.x;
    pointPropertyType.miYPosition = mParams.mCVPointRect2.y;
    pointPropertyType.miXMin = 0;
    pointPropertyType.miXMax = INT_MAX;
    pointPropertyType.miYMin = 0;
    pointPropertyType.miYMax = INT_MAX;
    propId = "rect_point_2";
    auto propRectPoint2 = std::make_shared< TypedProperty< PointPropertyType > >( "Boundary Point 2", propId, QMetaType::QPoint, pointPropertyType, "Display");
    mvProperty.push_back( propRectPoint2 );
    mMapIdToProperty[ propId ] = propRectPoint2;

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"4 neighbor pixels","8 neighbor pixels", "FLOODFILL_MASK_ONLY", "FLOODFILL_FIXED_RANGE"});
    enumPropertyType.miCurrentIndex = 0;
    propId = "flags";
    auto propFlags = std::make_shared< TypedProperty < EnumPropertyType > > ("Flags", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propFlags );
    mMapIdToProperty[ propId ] = propFlags;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miMaskColor;
    intPropertyType.miMax = 255;
    propId = "mask_color";
    auto propMaskColor = std::make_shared<TypedProperty<IntPropertyType>>("Mask Color", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back( propMaskColor);
    mMapIdToProperty[propId] = propMaskColor;

    propId = "active_mask";
    auto propActiveMask = std::make_shared<TypedProperty<bool>>("", propId, QMetaType::Bool, mParams.mbActiveMask);
    mMapIdToProperty[ propId ] = propActiveMask;
}

QObject*
CVFloodFillModel::
createWorker()
{
    return new CVFloodFillWorker();
}

void
CVFloodFillModel::
connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVFloodFillWorker*>(worker);
    if (w) {
        connect(w, &CVFloodFillWorker::frameReady,
                this, &CVFloodFillModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void
CVFloodFillModel::
dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;

    cv::Mat input = mPendingFrame;
    cv::Mat maskInput = mPendingMask;
    CVFloodFillParameters params = mPendingParams;
    setPendingWork(false);

    long frameId = getNextFrameId();
    QString producerId = getNodeId();

    std::shared_ptr<CVImagePool> poolCopy = getFramePool();

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(cv::Mat, maskInput.empty() ? cv::Mat() : maskInput.clone()),
                              Q_ARG(CVFloodFillParameters, params),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

unsigned int
CVFloodFillModel::
nPorts(PortType portType) const
{
    unsigned int result = 0;

    switch ( portType)
    {
    case PortType::In:
        result = 3;  // Image input + mask input + sync input
        break;

    case PortType::Out:
        result = 3;  // Image output + mask output + sync signal
        break;

    default:
        break;
    }

    return result;
}


NodeDataType
CVFloodFillModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out && portIndex == 2)
        return SyncData().type();
    if (portType == PortType::In && portIndex == 2)
        return SyncData().type();
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVFloodFillModel::
outData(PortIndex portIndex)
{
    if( isEnable() )
    {
        if (portIndex < 2)
            return mapCVImageData[portIndex];
        else if (portIndex == 2)
            return mpSyncData;
    }
    return nullptr;
}

void
CVFloodFillModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        if (portIndex == 0)
        {
            auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
            if (d)
            {
                mapCVImageInData[0] = d;
                if (mpCVImageInData)
                    mpCVImageInData.reset();
                mpCVImageInData = d;
                
                // Update bounds for all point properties when image dimensions change
                if (!d->data().empty())
                {
                    int maxWidth = d->data().cols - 1;
                    int maxHeight = d->data().rows - 1;
                    updatePointPropertyBounds("seed_point", maxWidth, maxHeight);
                    updatePointPropertyBounds("rect_point_1", maxWidth, maxHeight);
                    updatePointPropertyBounds("rect_point_2", maxWidth, maxHeight);
                }
                
                if( mpEmbeddedWidget && mpEmbeddedWidget->isVisible() )
                {
                    mpEmbeddedWidget->toggle_widgets(d->data().channels());
                }
                
                if (!isShuttingDown())
                    process_cached_input();
            }
        }
        else if (portIndex == 1)
        {
            auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
            if (d)
            {
                mapCVImageInData[1] = d;
                
                // Update active mask status
                if (mapCVImageInData[0] && !mapCVImageInData[0]->data().empty())
                {
                    cv::Mat& in_image = mapCVImageInData[0]->data();
                    mParams.mbActiveMask = (!d->data().empty() && d->data().type()==CV_8UC1
                                          && d->data().cols==in_image.cols+2
                                          && d->data().rows==in_image.rows+2);
                    
                    if( mpEmbeddedWidget && mpEmbeddedWidget->isVisible() )
                    {
                        mpEmbeddedWidget->set_maskStatus_label(mParams.mbActiveMask);
                    }
                }
                
                if (mapCVImageInData[0] && !isShuttingDown())
                    process_cached_input();
            }
        }
        else if (portIndex == 2)
        {
            // Sync input port - only process when sync signal is true
            auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
            if (d && d->data() == true)
            {
                if (mapCVImageInData[0] && !isShuttingDown())
                    process_cached_input();
            }
        }
    }
    else
    {
        if (portIndex == 0)
        {
            mapCVImageInData[0].reset();
            if (mpCVImageInData)
                mpCVImageInData.reset();
        }
        else if (portIndex == 1)
        {
            mapCVImageInData[1].reset();
            mParams.mbActiveMask = false;
            if( mpEmbeddedWidget && mpEmbeddedWidget->isVisible() )
            {
                mpEmbeddedWidget->set_maskStatus_label(false);
            }
        }
    }
}

void
CVFloodFillModel::
handleFrameReady(std::shared_ptr<CVImageData> img, std::shared_ptr<CVImageData> mask)
{
    setWorkerBusy(false);
    
    if (img)
    {
        mapCVImageData[0] = img;
        Q_EMIT dataUpdated(0);
    }
    
    if (mask)
    {
        mapCVImageData[1] = mask;
        Q_EMIT dataUpdated(1);
    }
    
    // Emit sync "true" on port 2
    if (mpSyncData)
    {
        mpSyncData->data() = true;
        Q_EMIT dataUpdated(2);
    }
    
    // Process pending work if available
    if (hasPendingWork())
    {
        dispatchPendingWork();
    }
}

QJsonObject
CVFloodFillModel::
save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["seedPointX"] = mParams.mCVPointSeed.x;
    cParams["seedPointY"] = mParams.mCVPointSeed.y;
    for(int i=0; i<4; i++)
    {
        cParams[QString("fillColor%1").arg(i)] = mParams.mucFillColor[i];
        cParams[QString("lowerDiff%1").arg(i)] = mParams.mucLowerDiff[i];
        cParams[QString("upperDiff%1").arg(i)] = mParams.mucUpperDiff[i];
    }
    cParams["defineBoundaries"] = mParams.mbDefineBoundaries;
    cParams["rectPoint1X"] = mParams.mCVPointRect1.x;
    cParams["rectPoint1Y"] = mParams.mCVPointRect1.y;
    cParams["rectPoint2X"] = mParams.mCVPointRect2.x;
    cParams["rectPoint2Y"] = mParams.mCVPointRect2.y;
    cParams["flags"] = mParams.miFlags;
    cParams["maskColor"] = mParams.miMaskColor;
    cParams["activeMask"] = mParams.mbActiveMask;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVFloodFillModel::
load(QJsonObject const& p)
{
    PBAsyncDataModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue argX = paramsObj[ "seedPointX" ];
        QJsonValue argY = paramsObj[ "seedPointY" ];
        if( !argX.isNull() && !argY.isNull() )
        {
            auto prop = mMapIdToProperty[ "seed_point" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = argX.toInt();
            typedProp->getData().miYPosition = argY.toInt();

            mParams.mCVPointSeed = cv::Point(argX.toInt(),argY.toInt());
        }
        QJsonValue v;
        for(int i=0; i<4; i++)
        {
            v = paramsObj[QString("fillColor%1").arg(i)];
            if( !v.isNull() )
            {
                auto prop = mMapIdToProperty[QString("fill_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucFillColor[i] = v.toInt();
            }
            v = paramsObj[QString("lowerDiff%1").arg(i)];
            if( !v.isNull() )
            {
                auto prop = mMapIdToProperty[QString("lower_diff_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucLowerDiff[i] = v.toInt();
            }
            v = paramsObj[QString("upperDiff%1").arg(i)];
            if( !v.isNull() )
            {
                auto prop = mMapIdToProperty[QString("upper_diff_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucUpperDiff[i] = v.toInt();
            }
        }
        mpEmbeddedWidget->set_lower_upper(mParams.mucLowerDiff,mParams.mucUpperDiff);
        v = paramsObj["defineBoundarie"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "define_boundaries" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop);
            typedProp->getData() = v.toBool();

            mParams.mbDefineBoundaries = v.toBool();
        }
        argX = paramsObj[ "rectPoint1X" ];
        argY = paramsObj[ "rectPoint1Y" ];
        if( !argX.isNull() && !argY.isNull() )
        {
            auto prop = mMapIdToProperty[ "rect_point_1" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = argX.toInt();
            typedProp->getData().miYPosition = argY.toInt();

            mParams.mCVPointRect1 = cv::Point(argX.toInt(),argY.toInt());
        }
        argX = paramsObj[ "rectPoint2X" ];
        argY = paramsObj[ "rectPoint2Y" ];
        if( !argX.isNull() && !argY.isNull() )
        {
            auto prop = mMapIdToProperty[ "rect_point_2" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = argX.toInt();
            typedProp->getData().miYPosition = argY.toInt();

            mParams.mCVPointRect2 = cv::Point(argX.toInt(),argY.toInt());
        }
        v = paramsObj[ "flags" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "flags" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < EnumPropertyType > >( prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miFlags = v.toInt();
        }
        v = paramsObj[ "maskColor" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "mask_color" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < IntPropertyType >>( prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miMaskColor = v.toInt();
        }
        v = paramsObj["activeMask"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "active_mask" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mpEmbeddedWidget->set_maskStatus_label(v.toBool());
        }
    }
}

void
CVFloodFillModel::
setModelProperty( QString & id, const QVariant & value )
{
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "seed_point" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint sPoint =  value.toPoint();
        
        // Clamp to property bounds
        int x = std::max(typedProp->getData().miXMin, std::min(sPoint.x(), typedProp->getData().miXMax));
        int y = std::max(typedProp->getData().miYMin, std::min(sPoint.y(), typedProp->getData().miYMax));
        
        typedProp->getData().miXPosition = x;
        typedProp->getData().miYPosition = y;
        mParams.mCVPointSeed = cv::Point( x, y );
    }
    else if ( id.startsWith("fill_color_"))
    {
        for(int i=0; i<4; i++)
        {
            if( id == QString("fill_color_%1").arg(i) )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = value.toInt();
                mParams.mucFillColor[i] = value.toInt();
            }
        }
    }
    else if( id == "define_boundaries")
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <bool>>( prop);
        typedProp->getData() = value.toBool();
        mParams.mbDefineBoundaries = value.toBool();
    }
    else if( id == "rect_point_1" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint rPoint1 =  value.toPoint();
        
        // Clamp to property bounds
        int x = std::max(typedProp->getData().miXMin, std::min(rPoint1.x(), typedProp->getData().miXMax));
        int y = std::max(typedProp->getData().miYMin, std::min(rPoint1.y(), typedProp->getData().miYMax));
        
        typedProp->getData().miXPosition = x;
        typedProp->getData().miYPosition = y;
        mParams.mCVPointRect1 = cv::Point( x, y );
    }
    else if( id == "rect_point_2" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint rPoint2 =  value.toPoint();
        
        // Clamp to property bounds
        int x = std::max(typedProp->getData().miXMin, std::min(rPoint2.x(), typedProp->getData().miXMax));
        int y = std::max(typedProp->getData().miYMin, std::min(rPoint2.y(), typedProp->getData().miYMax));
        
        typedProp->getData().miXPosition = x;
        typedProp->getData().miYPosition = y;
        mParams.mCVPointRect2 = cv::Point( x, y );
    }
    else if( id == "flags" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < EnumPropertyType > >( prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt())
        {
        case 0: mParams.miFlags = 4; break;
        case 1: mParams.miFlags = 8; break;
        case 2: mParams.miFlags = cv::FLOODFILL_MASK_ONLY; break;
        case 3: mParams.miFlags = cv::FLOODFILL_FIXED_RANGE; break;
        }
    }
    else if( id == "mask_color" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <IntPropertyType>>( prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miMaskColor = value.toInt();
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
    if (mapCVImageData[0] && !isShuttingDown())
        process_cached_input();
}

void CVFloodFillModel::em_spinbox_clicked(int spinbox, int value)
{
    if( spinbox < 4 )
        mParams.mucLowerDiff[ spinbox ] = value;
    else if( spinbox < 8 )
        mParams.mucUpperDiff[ spinbox - 4 ] = value;

    if( mapCVImageInData[0] && !isShuttingDown() )
    {
        process_cached_input();
    }
}

void
CVFloodFillModel::
process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;
    
    cv::Mat input = mpCVImageInData->data();
    cv::Mat maskInput;
    if (mapCVImageInData[1] && !mapCVImageInData[1]->data().empty())
    {
        maskInput = mapCVImageInData[1]->data();
    }
    
    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(2);
    });
    
    if (isWorkerBusy())
    {
        // Store as pending - will be processed when worker finishes
        mPendingFrame = input.clone();
        if (!maskInput.empty())
            mPendingMask = maskInput.clone();
        else
            mPendingMask = cv::Mat();
        mPendingParams = mParams;
        setPendingWork(true);
    }
    else
    {
        setWorkerBusy(true);
        
        long frameId = getNextFrameId();
        QString producerId = getNodeId();
        
        std::shared_ptr<CVImagePool> poolCopy = getFramePool();
        
        // Prepare scalar values based on channel count
        cv::Scalar fillColor, lowerDiff, upperDiff;
        if (input.channels() == 1)
        {
            fillColor = cv::Scalar(mParams.mucFillColor[3]);
            lowerDiff = cv::Scalar(mParams.mucLowerDiff[3]);
            upperDiff = cv::Scalar(mParams.mucUpperDiff[3]);
        }
        else
        {
            fillColor = cv::Scalar(mParams.mucFillColor[0], mParams.mucFillColor[1], mParams.mucFillColor[2]);
            lowerDiff = cv::Scalar(mParams.mucLowerDiff[0], mParams.mucLowerDiff[1], mParams.mucLowerDiff[2]);
            upperDiff = cv::Scalar(mParams.mucUpperDiff[0], mParams.mucUpperDiff[1], mParams.mucUpperDiff[2]);
        }
        
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(cv::Mat, maskInput.empty() ? cv::Mat() : maskInput.clone()),
                                  Q_ARG(CVFloodFillParameters, mParams),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}

void
CVFloodFillModel::
updatePointPropertyBounds(const QString& propId, int maxWidth, int maxHeight)
{
    if (!mMapIdToProperty.contains(propId))
        return;
    
    auto prop = mMapIdToProperty[propId];
    auto typedProp = std::static_pointer_cast<TypedProperty<PointPropertyType>>(prop);
    
    // Update the bounds in the property data
    typedProp->getData().miXMin = 0;
    typedProp->getData().miXMax = maxWidth;
    typedProp->getData().miYMin = 0;
    typedProp->getData().miYMax = maxHeight;
    
    // Clamp current values to new bounds
    int clampedX = std::max(0, std::min(typedProp->getData().miXPosition, maxWidth));
    int clampedY = std::max(0, std::min(typedProp->getData().miYPosition, maxHeight));
    
    typedProp->getData().miXPosition = clampedX;
    typedProp->getData().miYPosition = clampedY;
    
    // Update mParams to match
    if (propId == "seed_point")
    {
        mParams.mCVPointSeed = cv::Point(clampedX, clampedY);
    }
    else if (propId == "rect_point_1")
    {
        mParams.mCVPointRect1 = cv::Point(clampedX, clampedY);
    }
    else if (propId == "rect_point_2")
    {
        mParams.mCVPointRect2 = cv::Point(clampedX, clampedY);
    }
}
