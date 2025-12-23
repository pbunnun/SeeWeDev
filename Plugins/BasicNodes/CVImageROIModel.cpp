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

#include "CVImageROIModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>
#include <opencv2/imgproc.hpp>

#include <QtWidgets/QFileDialog>


#include "CVImageData.hpp"

const QString CVImageROIModel::_category = QString( "Image Operation" );

const QString CVImageROIModel::_model_name = QString( "CV Image ROI" );

CVImageROIModel::

CVImageROIModel()
    : PBNodeDelegateModel( _model_name ),
      mpEmbeddedWidget( new CVImageROIEmbeddedWidget() ),
      _minPixmap(":/ROI.png")
{
    for(std::shared_ptr<CVImageData>& mp : mapCVImageData)
    {
        mp = std::make_shared<CVImageData>(cv::Mat());
    }
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &CVImageROIEmbeddedWidget::button_clicked_signal, this, &CVImageROIModel::em_button_clicked );

    PointPropertyType pointPropertyType;
    pointPropertyType.miXPosition = mParams.mCVPointRect1.x;
    pointPropertyType.miYPosition = mParams.mCVPointRect1.y;
    QString propId = "rect_point_1";
    auto propRectPoint1 = std::make_shared< TypedProperty< PointPropertyType > >("Point 1", propId, QMetaType::QPoint, pointPropertyType, "Operation");
    mvProperty.push_back( propRectPoint1 );
    mMapIdToProperty[ propId ] = propRectPoint1;

    pointPropertyType.miXPosition = mParams.mCVPointRect2.x;
    pointPropertyType.miYPosition = mParams.mCVPointRect2.y;
    propId = "rect_point_2";
    auto propRectPoint2 = std::make_shared< TypedProperty< PointPropertyType > >("Point 2", propId, QMetaType::QPoint, pointPropertyType, "Operation");
    mvProperty.push_back( propRectPoint2 );
    mMapIdToProperty[ propId ] = propRectPoint2;

    UcharPropertyType ucharPropertyType;
    for(int i=0; i<3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucLineColor[i];
        propId = QString("line_color_%1").arg(i);
        auto propLineColor = std::make_shared< TypedProperty< UcharPropertyType > >( QString::fromStdString("Line Color "+color[i]), propId, QMetaType::Int, ucharPropertyType, "Display");
        mvProperty.push_back( propLineColor );
        mMapIdToProperty[ propId ] = propLineColor;
    }

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miLineThickness;
    propId = "line_thickness";
    auto propLineThickness = std::make_shared<TypedProperty<IntPropertyType>>("Line Thickness", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back( propLineThickness );
    mMapIdToProperty[ propId ] = propLineThickness;

    propId = "display_lines";
    auto propDisplayLines = std::make_shared<TypedProperty<bool>>("Display Lines", propId, QMetaType::Bool, mParams.mbDisplayLines, "Display");
    mvProperty.push_back( propDisplayLines );
    mMapIdToProperty[ propId ] = propDisplayLines;

    propId = "lock_output_roi";
    auto propLockOutputROI = std::make_shared<TypedProperty<bool>>("Lock Output ROI", propId, QMetaType::Bool, mParams.mbLockOutputROI, "Operation");
    mvProperty.push_back( propLockOutputROI );
    mMapIdToProperty[ propId ] = propLockOutputROI;
}

unsigned int
CVImageROIModel::

nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
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
CVImageROIModel::

dataType( PortType, PortIndex ) const
{
    return CVImageData().type();
}

std::shared_ptr<NodeData>
CVImageROIModel::

outData(PortIndex I)
{
    if( isEnable() )
        return mapCVImageData[I];
    else
        return nullptr;
}

void
CVImageROIModel::

setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;

    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mapCVImageInData[portIndex] = d;
            mProps.mbNewMat = true;
            if(mapCVImageInData[0] && !mapCVImageInData[1])
            {
                overwrite( mapCVImageInData[0], mParams);
                processData( mapCVImageInData, mapCVImageData, mParams, mProps);
                mParams.mbLockOutputROI ? Q_EMIT dataUpdated( 1 ) : updateAllOutputPorts();
            }
            else if(mapCVImageInData[0] && mapCVImageInData[1])
            {
                overwrite( mapCVImageInData[0], mParams);
                processData( mapCVImageInData, mapCVImageData, mParams, mProps);
                Q_EMIT dataUpdated(1);
            }
        }
    }
}

QJsonObject
CVImageROIModel::

save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["rectPoint1X"] = mParams.mCVPointRect1.x;
    cParams["rectPoint1Y"] = mParams.mCVPointRect1.y;
    cParams["rectPoint2X"] = mParams.mCVPointRect2.x;
    cParams["rectPoint2Y"] = mParams.mCVPointRect2.y;
    for(int i=0; i<3; i++)
    {
        cParams[QString("lineColor%1").arg(i)] = mParams.mucLineColor[i];
    }
    cParams["lineThickness"] = mParams.miLineThickness;
    cParams["displayLines"] = mParams.mbDisplayLines;
    cParams["lockOutputROI"] =mParams.mbLockOutputROI;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVImageROIModel::

load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue argX = paramsObj[ "rectPoint1X" ];
        QJsonValue argY = paramsObj[ "rectPoint1Y" ];
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
        QJsonValue v;
        for(int i=0; i<3; i++)
        {
            v = paramsObj[QString("lineColor%1").arg(i)];
            if( !v.isNull() )
            {
                auto prop = mMapIdToProperty[QString("line_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucLineColor[i] = v.toInt();
            }
        }
        v = paramsObj[ "lineThickness" ];
        if(! v.isNull() )
        {
            auto prop = mMapIdToProperty["line_thickness"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miLineThickness = v.toInt();
        }
        v = paramsObj[ "displayLines" ];
        if(! v.isNull() )
        {
            auto prop = mMapIdToProperty["display_lines"];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mParams.mbDisplayLines = v.toBool();
        }
        v = paramsObj[ "lockOutputROIisplayLines" ];
        if(! v.isNull() )
        {
            auto prop = mMapIdToProperty["lock_output_roi"];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mParams.mbLockOutputROI = v.toBool();
        }
    }
}

void
CVImageROIModel::

setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    int minX = 0;
    int minY = 0;
    int maxX = mParams.mCVPointRect2.x;
    int maxY = mParams.mCVPointRect2.y;

    auto prop = mMapIdToProperty[ id ];
    if( id == "rect_point_1" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint rPoint1 =  value.toPoint();
        bool adjValue = false;
        if( rPoint1.x() > maxX )
        {
            rPoint1.setX(maxX);
            adjValue = true;
        }
        else if( rPoint1.x() < minX)
        {
            rPoint1.setX(minX);
            adjValue = true;
        }
        if( rPoint1.y() > maxY)
        {
            rPoint1.setY(maxY);
            adjValue = true;
        }
        else if( rPoint1.y() < minY)
        {
            rPoint1.setY(minY);
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

    minX = mParams.mCVPointRect1.x;
    minY = mParams.mCVPointRect1.y;
    if( mapCVImageInData[0] )
    {
        maxX = mapCVImageInData[0]->data().cols;
        maxY = mapCVImageInData[0]->data().rows;
    }

    if( id == "rect_point_2" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint rPoint2 =  value.toPoint();
        bool adjValue = false;
        if( rPoint2.x() > maxX )
        {
            rPoint2.setX(maxX);
            adjValue = true;
        }
        else if( rPoint2.x() < minX)
        {
            rPoint2.setX(minX);
            adjValue = true;
        }
        if( rPoint2.y() > maxY)
        {
            rPoint2.setY(maxY);
            adjValue = true;
        }
        else if( rPoint2.y() < minY)
        {
            rPoint2.setY(minY);
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
    for(int i=0; i<3; i++)
    {
        if( id == QString("line_color_%1").arg(i) )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
            typedProp->getData().mucValue = value.toInt();

            mParams.mucLineColor[i] = value.toInt();
        }
    }
    if( id == "line_thickness" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miLineThickness = value.toInt();
    }
    else if( id == "display_lines" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbDisplayLines = value.toBool();
    }
    else if( id == "lock_output_roi" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbLockOutputROI = value.toBool();
    }

    if(mapCVImageInData[0] && !mapCVImageInData[1])
    {
        processData( mapCVImageInData, mapCVImageData, mParams, mProps);
        mParams.mbLockOutputROI?
        Q_EMIT dataUpdated( 1 ) : updateAllOutputPorts();
    }
    else if(mapCVImageInData[0] && mapCVImageInData[1])
    {
        processData( mapCVImageInData, mapCVImageData, mParams, mProps);
        mParams.mbLockOutputROI?
        Q_EMIT dataUpdated( 1 ) : updateAllOutputPorts();
    }
}

void CVImageROIModel::em_button_clicked( int button )
{
    DEBUG_LOG_INFO() << "[em_button_clicked] button:" << button << "isSelected:" << isSelected();
    
    // If node is not selected, select it first and block the interaction
    // User needs to click again when node is selected to perform the action
    if (!isSelected())
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Node not selected, requesting selection";
        Q_EMIT selection_request_signal();
        return;
    }
    
    if(button == 0) //RESET
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] RESET button";
        mProps.mbReset = true;
        processData( mapCVImageInData, mapCVImageData, mParams, mProps);
        mParams.mbLockOutputROI?
        Q_EMIT dataUpdated( 1 ) : updateAllOutputPorts();
    }
    else if(button ==1) //APPLY
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] APPLY button";
        mProps.mbApply = true;
        processData( mapCVImageInData, mapCVImageData, mParams, mProps);
        Q_EMIT dataUpdated( 1 );
    }
}

void
CVImageROIModel::

processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr<CVImageData> (&out)[2],
            const CVImageROIParameters &params, CVImageROIProperties &props )
{
    const cv::Mat& in_image = in[0]->data();
    const bool invalid = in_image.empty() || in_image.depth()!=CV_8U;
    mpEmbeddedWidget->enable_reset_button(!invalid);
    if(invalid)
    {
        return;
    }
    const cv::Rect rect(params.mCVPointRect1,params.mCVPointRect2);
    cv::Mat& out_image = out[1]->data();
    out[1]->set_image(in_image);
    static cv::Mat save;
    if(props.mbReset || save.empty() ||props.mbNewMat)
    {
        save = in_image.clone();
    }
    out[1]->set_image(save);
    const bool validROI = in[1]!=nullptr &&
                          !in[1]->data().empty() &&
                          in[1]->data().cols==rect.width &&
                          in[1]->data().rows==rect.height &&
                          in[1]->data().channels() == out_image.channels();

    mpEmbeddedWidget->enable_apply_button(validROI);
    if(props.mbApply)
    {
        cv::Mat roi(out_image,rect);
        cv::addWeighted(in[1]->data(),1,roi,0,0,roi);
    }
    save = out_image.clone();
    out[0]->set_image(cv::Mat(in_image,rect));

    const int& d_rows = out_image.rows;
    const int& d_cols = out_image.cols;
    const cv::Scalar color(params.mucLineColor[0],params.mucLineColor[1],params.mucLineColor[2]);
    if(params.mbDisplayLines)
    {
        if(out_image.channels()==1)
        {
            cv::cvtColor(out_image,out_image,cv::COLOR_GRAY2BGR);
        }
        cv::line(out_image,
                 cv::Point(params.mCVPointRect1.x,0),
                 cv::Point(params.mCVPointRect1.x,d_rows),
                 color,
                 params.miLineThickness,
                 cv::LINE_8);
        cv::line(out_image,
                 cv::Point(0,params.mCVPointRect1.y),
                 cv::Point(d_cols,params.mCVPointRect1.y),
                 color,
                 params.miLineThickness,
                 cv::LINE_8);
        cv::line(out_image,
                 cv::Point(params.mCVPointRect2.x,0),
                 cv::Point(params.mCVPointRect2.x,d_rows),
                 color,
                 params.miLineThickness,
                 cv::LINE_8);
        cv::line(out_image,
                 cv::Point(0,params.mCVPointRect2.y),
                 cv::Point(d_cols,params.mCVPointRect2.y),
                 color,
                 params.miLineThickness,
                 cv::LINE_8);
    }

    props.mbReset = false;
    props.mbApply = false;
    props.mbNewMat = false;
}

void CVImageROIModel::overwrite(const std::shared_ptr<CVImageData> &in, CVImageROIParameters &params)
{
    int& row = in->data().rows;
    int& col = in->data().cols;
    if(params.mCVPointRect2.x > col)
    {
        auto prop = mMapIdToProperty["rect_point_2"];
        auto typedProp = std::static_pointer_cast<TypedProperty<PointPropertyType>>(prop);
        typedProp->getData().miXPosition = col;
        params.mCVPointRect2.x = col;
    }
    if(params.mCVPointRect2.y > row)
    {
        auto prop = mMapIdToProperty["rect_point_2"];
        auto typedProp = std::static_pointer_cast<TypedProperty<PointPropertyType>>(prop);
        typedProp->getData().miYPosition = row;
        params.mCVPointRect2.y = row;
    }
}

const std::string CVImageROIModel::color[3] = {"B", "G", "R"};


