//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "FloodFillModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

FloodFillModel::
FloodFillModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget(new FloodFillEmbeddedWidget),
      _minPixmap( ":FloodFill.png" )
{
    for(std::shared_ptr<CVImageData>& mp : mapCVImageData)
    {
        mp = std::make_shared< CVImageData >( cv::Mat() );
    }

    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &FloodFillEmbeddedWidget::spinbox_clicked_signal, this, &FloodFillModel::em_spinbox_clicked );

    PointPropertyType pointPropertyType;
    pointPropertyType.miXPosition = mParams.mCVPointSeed.x;
    pointPropertyType.miYPosition = mParams.mCVPointSeed.y;
    QString propId = "seed_point";
    auto propSeedPoint = std::make_shared< TypedProperty< PointPropertyType > >( "Seed Point", propId, QVariant::Point, pointPropertyType, "Operation");
    mvProperty.push_back( propSeedPoint );
    mMapIdToProperty[ propId ] = propSeedPoint;

    UcharPropertyType ucharPropertyType;
    for(int i=0; i<4; i++)
    {
        ucharPropertyType.mucValue = mParams.mucFillColor[i];
        propId = QString("fill_color_%1").arg(i);
        auto propFillColor = std::make_shared< TypedProperty< UcharPropertyType > >( QString::fromStdString("Fill Color "+color[i] ), propId, QVariant::Int, ucharPropertyType, "Operation");
        mvProperty.push_back( propFillColor );
        mMapIdToProperty[ propId ] = propFillColor;

        ucharPropertyType.mucValue = mParams.mucLowerDiff[i];
        propId = QString("lower_diff_%1").arg(i);
        auto propLowerDiff = std::make_shared< TypedProperty< UcharPropertyType > >( QString::fromStdString("Lower Diff "+color[i] ), propId, QVariant::Int, ucharPropertyType, "Operation");
        mMapIdToProperty[ propId ] = propLowerDiff;

        ucharPropertyType.mucValue = mParams.mucFillColor[i];
        propId = QString("upper_diff_%1").arg(i);
        auto propUpperDiff = std::make_shared< TypedProperty< UcharPropertyType > >( QString::fromStdString("Upper Diff "+color[i] ), propId, QVariant::Int, ucharPropertyType, "Operation");
        mMapIdToProperty[ propId ] = propUpperDiff;
    }
    mpEmbeddedWidget->set_lower_upper(mParams.mucLowerDiff,mParams.mucUpperDiff);
    mpEmbeddedWidget->toggle_widgets(3);

    propId = "define_boundaries";
    auto propDefineBoundaries = std::make_shared< TypedProperty <bool> >("Define Boundaries", propId, QVariant::Bool, mParams.mbDefineBoundaries, "Display");
    mvProperty.push_back( propDefineBoundaries );
    mMapIdToProperty[ propId ] = propDefineBoundaries;

    pointPropertyType.miXPosition = mParams.mCVPointRect1.x;
    pointPropertyType.miYPosition = mParams.mCVPointRect1.y;
    propId = "rect_point_1";
    auto propRectPoint1 = std::make_shared< TypedProperty< PointPropertyType > >( "Boundary Point 1", propId, QVariant::Point, pointPropertyType, "Display");
    mvProperty.push_back( propRectPoint1 );
    mMapIdToProperty[ propId ] = propRectPoint1;

    pointPropertyType.miXPosition = mParams.mCVPointRect2.x;
    pointPropertyType.miYPosition = mParams.mCVPointRect2.y;
    propId = "rect_point_2";
    auto propRectPoint2 = std::make_shared< TypedProperty< PointPropertyType > >( "Boundary Point 2", propId, QVariant::Point, pointPropertyType, "Display");
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
    auto propMaskColor = std::make_shared<TypedProperty<IntPropertyType>>("Mask Color", propId, QVariant::Int, intPropertyType, "Display");
    mvProperty.push_back( propMaskColor);
    mMapIdToProperty[propId] = propMaskColor;

    propId = "active_mask";
    auto propActiveMask = std::make_shared<TypedProperty<bool>>("", propId, QVariant::Bool, mProps.mbActiveMask);
    mMapIdToProperty[ propId ] = propActiveMask;
}

unsigned int
FloodFillModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch ( portType)
    {
    case PortType::In:
        result = 2;
        break;

    case PortType::Out:
        result = 2;
        break;

    default:
        break;
    }

    return result;
}


NodeDataType
FloodFillModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
FloodFillModel::
outData(PortIndex I)
{
    if( isEnable() )
        return mapCVImageData[I];
    else
        return nullptr;
}

void
FloodFillModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mapCVImageInData[portIndex] = d;
            if( mapCVImageInData[0] )
            {
                mpEmbeddedWidget->toggle_widgets(mapCVImageInData[0]->image().channels());
                processData( mapCVImageInData, mapCVImageData, mParams, mProps);
            }
        }
    }

    updateAllOutputPorts();
}

QJsonObject
FloodFillModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

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
    cParams["activeMask"] = mProps.mbActiveMask;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
FloodFillModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore( p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue argX = paramsObj[ "seedPointX" ];
        QJsonValue argY = paramsObj[ "seedPointY" ];
        if( !argX.isUndefined() && !argY.isUndefined() )
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
            if( !v.isUndefined() )
            {
                auto prop = mMapIdToProperty[QString("fill_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucFillColor[i] = v.toInt();
            }
            v = paramsObj[QString("lowerDiff%1").arg(i)];
            if( !v.isUndefined() )
            {
                auto prop = mMapIdToProperty[QString("lower_diff_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucLowerDiff[i] = v.toInt();
            }
            v = paramsObj[QString("upperDiff%1").arg(i)];
            if( !v.isUndefined() )
            {
                auto prop = mMapIdToProperty[QString("upper_diff_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucUpperDiff[i] = v.toInt();
            }
        }
        mpEmbeddedWidget->set_lower_upper(mParams.mucLowerDiff,mParams.mucUpperDiff);
        v = paramsObj["defineBoundarie"];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "define_boundaries" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop);
            typedProp->getData() = v.toBool();

            mParams.mbDefineBoundaries = v.toBool();
        }
        argX = paramsObj[ "rectPoint1X" ];
        argY = paramsObj[ "rectPoint1Y" ];
        if( !argX.isUndefined() && !argY.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "rect_point_1" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = argX.toInt();
            typedProp->getData().miYPosition = argY.toInt();

            mParams.mCVPointRect1 = cv::Point(argX.toInt(),argY.toInt());
        }
        argX = paramsObj[ "rectPoint2X" ];
        argY = paramsObj[ "rectPoint2Y" ];
        if( !argX.isUndefined() && !argY.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "rect_point_2" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = argX.toInt();
            typedProp->getData().miYPosition = argY.toInt();

            mParams.mCVPointRect2 = cv::Point(argX.toInt(),argY.toInt());
        }
        v = paramsObj[ "flags" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "flags" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < EnumPropertyType > >( prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miFlags = v.toInt();
        }
        v = paramsObj[ "maskColor" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "mask_color" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < IntPropertyType >>( prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miMaskColor = v.toInt();
        }
        v = paramsObj["activeMask"];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "active_mask" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mpEmbeddedWidget->set_maskStatus_label(v.toBool());
        }
    }
}

void
FloodFillModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    const int& maxX = mapCVImageInData[0]->image().cols;
    const int& maxY = mapCVImageInData[0]->image().rows;

    auto prop = mMapIdToProperty[ id ];
    if( id == "seed_point" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint sPoint =  value.toPoint();
        bool adjValue = false;
        if( sPoint.x() > maxX )
        {
            sPoint.setX( maxX);
            adjValue = true;
        }
        else if( sPoint.x() < 0)
        {
            sPoint.setX(0);
            adjValue = true;
        }
        if( sPoint.y() > maxY)
        {
            sPoint.setY( maxY);
            adjValue = true;
        }
        else if( sPoint.y() < 0)
        {
            sPoint.setY(0);
            adjValue = true;
        }
        if( adjValue )
        {
            typedProp->getData().miXPosition = sPoint.x();
            typedProp->getData().miYPosition = sPoint.y();

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = sPoint.x();
            typedProp->getData().miYPosition = sPoint.y();

            mParams.mCVPointSeed = cv::Point( sPoint.x(), sPoint.y() );
        }
    }
    for(int i=0; i<4; i++)
    {
        if( id == QString("fill_color_%1").arg(i) )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
            typedProp->getData().mucValue = value.toInt();

            mParams.mucFillColor[i] = value.toInt();
        }
    }
    if( id == "define_boundaries")
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <bool>>( prop);
        typedProp->getData() = value.toBool();

        mParams.mbDefineBoundaries = value.toBool();
    }
    else if( id == "rect_point_1" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint rPoint1 =  value.toPoint();
        bool adjValue = false;
        if( rPoint1.x() > maxX )
        {
            rPoint1.setX( maxX);
            adjValue = true;
        }
        else if( rPoint1.x() < 0)
        {
            rPoint1.setX(0);
            adjValue = true;
        }
        if( rPoint1.y() > maxY)
        {
            rPoint1.setY( maxY);
            adjValue = true;
        }
        else if( rPoint1.y() < 0)
        {
            rPoint1.setY(0);
            adjValue = true;
        }
        if( adjValue )
        {
            typedProp->getData().miXPosition = rPoint1.x();
            typedProp->getData().miYPosition = rPoint1.y();

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = rPoint1.x();
            typedProp->getData().miYPosition = rPoint1.y();

            mParams.mCVPointRect1 = cv::Point( rPoint1.x(), rPoint1.y() );
        }
    }
    else if( id == "rect_point_2" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint rPoint2 =  value.toPoint();
        bool adjValue = false;
        if( rPoint2.x() > maxX )
        {
            rPoint2.setX( maxX);
            adjValue = true;
        }
        else if( rPoint2.x() < 0)
        {
            rPoint2.setX(0);
            adjValue = true;
        }
        if( rPoint2.y() > maxY)
        {
            rPoint2.setY( maxY);
            adjValue = true;
        }
        else if( rPoint2.y() < 0)
        {
            rPoint2.setY(0);
            adjValue = true;
        }
        if( adjValue )
        {
            typedProp->getData().miXPosition = rPoint2.x();
            typedProp->getData().miYPosition = rPoint2.y();

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = rPoint2.x();
            typedProp->getData().miYPosition = rPoint2.y();

            mParams.mCVPointRect2 = cv::Point( rPoint2.x(), rPoint2.y() );
        }
    }
    else if( id == "flags" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < EnumPropertyType > >( prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miFlags = 4;
            break;

        case 1:
            mParams.miFlags = 8;
            break;

        case 2:
            mParams.miFlags = cv::FLOODFILL_MASK_ONLY;
            break;

        case 3:
            mParams.miFlags = cv::FLOODFILL_FIXED_RANGE;
            break;
        }
    }
    else if( id == "mask_color" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <IntPropertyType>>( prop);
        typedProp->getData().miValue = value.toInt();

        mParams.miMaskColor = value.toInt();
    }

    if( mapCVImageInData[0] )
    {
        mpEmbeddedWidget->toggle_widgets(mapCVImageInData[0]->image().channels());
        processData( mapCVImageInData, mapCVImageData, mParams, mProps );
        updateAllOutputPorts();
    }
}

void FloodFillModel::em_spinbox_clicked(int spinbox, int value)
{
    if( spinbox < 4 )
        mParams.mucLowerDiff[ spinbox ] = value;
    else if( spinbox < 8 )
        mParams.mucUpperDiff[ spinbox - 4 ] = value;

    if( mapCVImageInData[0] )
    {
        processData( mapCVImageInData,mapCVImageData,mParams,mProps);
        updateAllOutputPorts();
    }
}

void
FloodFillModel::
processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr<CVImageData> (&out)[2],
            const FloodFillParameters & params, FloodFillProperties &props)
{
    cv::Mat& in_image = in[0]->image();
    if(in_image.empty() || (in_image.channels()!=1 && in_image.channels()!=3) ||
    (in_image.depth()!=CV_8U && in_image.depth()!=CV_8S && in_image.depth()!=CV_16F && in_image.depth()!=CV_32F && in_image.depth()!=CV_64F))
    {
        return;
    }
    cv::Mat& out_image = out[0]->image();
    out[0]->set_image(in_image);
    props.mbActiveMask = (in[1]!=nullptr && !in[1]->image().empty() && in[1]->image().type()==CV_8UC1
    &&in[1]->image().cols==in_image.cols+2
    && in[1]->image().rows==in_image.rows+2)? true : false ;
    mpEmbeddedWidget->set_maskStatus_label( props.mbActiveMask);
    if( params.mbDefineBoundaries)
    {
        cv::Rect rect( params.mCVPointRect1, params.mCVPointRect2);
        switch(in_image.channels())
        {
        case 1:
            if( props.mbActiveMask)
            {
            out[1]->set_image(in[1]->image());
            cv::floodFill(out_image,
                          out[1]->image(),
                          params.mCVPointSeed,
                          cv::Scalar( params.mucFillColor[3] ),
                          &rect,
                          cv::Scalar( params.mucLowerDiff[3] ),
                          cv::Scalar( params.mucUpperDiff[3] ),
                          params.miFlags | ( params.miMaskColor<<8));
            }
            else
            {
                cv::floodFill(out_image,
                              cv::Mat(),
                              params.mCVPointSeed,
                              cv::Scalar( params.mucFillColor[3] ),
                              &rect,
                              cv::Scalar( params.mucLowerDiff[3] ),
                              cv::Scalar( params.mucUpperDiff[3] ),
                              params.miFlags | ( params.miMaskColor<<8));
            }
            break;

        case 3:
            if( props.mbActiveMask)
            {
                out[1]->set_image(in[1]->image());
                cv::floodFill(out_image,
                              out[1]->image(),
                              params.mCVPointSeed,
                              cv::Scalar( params.mucFillColor[0], params.mucFillColor[1], params.mucFillColor[2] ),
                              &rect,
                              cv::Scalar( params.mucLowerDiff[0], params.mucLowerDiff[1], params.mucLowerDiff[2] ),
                              cv::Scalar( params.mucUpperDiff[0], params.mucUpperDiff[1], params.mucUpperDiff[2] ),
                              params.miFlags | ( params.miMaskColor<<8));
            }
            else
            {
                cv::floodFill(out_image,
                              cv::Mat(),
                              params.mCVPointSeed,
                              cv::Scalar( params.mucFillColor[0], params.mucFillColor[1], params.mucFillColor[2] ),
                              &rect,
                              cv::Scalar( params.mucLowerDiff[0], params.mucLowerDiff[1], params.mucLowerDiff[2] ),
                              cv::Scalar( params.mucUpperDiff[0], params.mucUpperDiff[1], params.mucUpperDiff[2] ),
                              params.miFlags | ( params.miMaskColor<<8));
            }
            break;
        }
    }
    else
    {
        switch(in_image.channels())
        {
        case 1:
            if( props.mbActiveMask)
            {
                out[1]->set_image(in[1]->image());
                cv::floodFill(out_image,
                              out[1]->image(),
                              params.mCVPointSeed,
                              cv::Scalar( params.mucFillColor[3] ),
                              0,
                              cv::Scalar( params.mucLowerDiff[3] ),
                              cv::Scalar( params.mucUpperDiff[3] ),
                              params.miFlags | ( params.miMaskColor<<8));
            }
            else
            {
                cv::floodFill(out_image,
                              cv::Mat(),
                              params.mCVPointSeed,
                              cv::Scalar( params.mucFillColor[3] ),
                              0,
                              cv::Scalar( params.mucLowerDiff[3] ),
                              cv::Scalar( params.mucUpperDiff[3] ),
                              params.miFlags | ( params.miMaskColor<<8));
            }
            break;

        case 3:
            if( props.mbActiveMask)
            {
                out[1]->set_image(in[1]->image());
                cv::floodFill(out_image,
                              out[1]->image(),
                              params.mCVPointSeed,
                              cv::Scalar( params.mucFillColor[0], params.mucFillColor[1], params.mucFillColor[2] ),
                              0,
                              cv::Scalar( params.mucLowerDiff[0], params.mucLowerDiff[1], params.mucLowerDiff[2] ),
                              cv::Scalar( params.mucUpperDiff[0], params.mucUpperDiff[1], params.mucUpperDiff[2] ),
                              params.miFlags | ( params.miMaskColor<<8));
            }
            else
            {
                cv::floodFill(out_image,
                              cv::Mat(),
                              params.mCVPointSeed,
                              cv::Scalar( params.mucFillColor[0], params.mucFillColor[1], params.mucFillColor[2] ),
                              0,
                              cv::Scalar( params.mucLowerDiff[0], params.mucLowerDiff[1], params.mucLowerDiff[2] ),
                              cv::Scalar( params.mucUpperDiff[0], params.mucUpperDiff[1], params.mucUpperDiff[2] ),
                              params.miFlags | ( params.miMaskColor<<8));
            }
            break;
        }
    }
}

const QString FloodFillModel::_category = QString( "Image Modification" );

const QString FloodFillModel::_model_name = QString( "Flood Fill" );

const std::string FloodFillModel::color[4] = {"B","G","R","Gray"};
