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

#include "DrawContourModel.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QEvent>
#include <QDir>
#include "internal/Connection.hpp"
#include "qtvariantproperty.h"

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

DrawContourModel::
DrawContourModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap(":DrawContour.png")
{
    mpCVImageOutData = std::make_shared< CVImageData >( cv::Mat() );

    EnumPropertyType enumPropertyType;
    UcharPropertyType ucharPropertyType;
    ucharPropertyType.mucValue = mParams.mucBValue;
    QString propId = "b_value";
    auto propBValue = std::make_shared<TypedProperty<UcharPropertyType>>("B value",propId,QMetaType::Int,ucharPropertyType, "Operation");
    mvProperty.push_back(propBValue);
    mMapIdToProperty[propId] = propBValue;

    ucharPropertyType.mucValue = mParams.mucGValue;
    propId = "g_value";
    auto propGValue = std::make_shared<TypedProperty<UcharPropertyType>>("G value",propId,QMetaType::Int,ucharPropertyType, "Operation");
    mvProperty.push_back(propGValue);
    mMapIdToProperty[propId] = propGValue;

    ucharPropertyType.mucValue = mParams.mucRValue;
    propId = "r_value";
    auto propRValue = std::make_shared<TypedProperty<UcharPropertyType>>("R value",propId,QMetaType::Int,ucharPropertyType, "Operation");
    mvProperty.push_back(propRValue);
    mMapIdToProperty[propId] = propRValue;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miLineThickness;
    propId = "line_thickness";
    auto propLineThickness = std::make_shared<TypedProperty<IntPropertyType>>("Line Thickness",propId,QMetaType::Int,intPropertyType, "Display");
    mvProperty.push_back(propLineThickness);
    mMapIdToProperty[propId] = propLineThickness;

    enumPropertyType.mslEnumNames = QStringList({"LINE_8", "LINE_4", "LINE_AA"});
    enumPropertyType.miCurrentIndex = 0;
    propId = "line_type";
    auto propLineType = std::make_shared<TypedProperty<EnumPropertyType>>("Line Type",propId,QtVariantPropertyManager::enumTypeId(),enumPropertyType, "Display");
    mvProperty.push_back(propLineType);
    mMapIdToProperty[propId] = propLineType;
}

unsigned int
DrawContourModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 2 );
    case PortType::Out:
        return( 1 );
    default:
        return( -1 );
    }
}

NodeDataType
DrawContourModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if( portIndex == 0 )
        {
            return CVImageData().type();
        }
        else if( portIndex == 1 )
        {
            return ContourPointsData().type();
        }
    }
    else
    {
        if( portIndex == 0 )
            return CVImageData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
DrawContourModel::
outData( PortIndex portIndex )
{
    if( isEnable() )
    {
        if( portIndex == 0 && mpCVImageOutData->data().data != nullptr )
        {
            return mpCVImageOutData;
        }
    }
    return nullptr;
}

void
DrawContourModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex )
{
    if( !isEnable() || !nodeData )
        return;
    if( portIndex == 0 )
    {
        if( nodeData )
        {
            if( mpCVImageInData )
                mpContourPointsData = nullptr;
            auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
            if( d )
            {
                mpCVImageInData = d;
                if( mpContourPointsData )
                {
                    processData(mpCVImageInData, mpCVImageOutData, mpContourPointsData, mParams);
                    Q_EMIT dataUpdated(0);
                }
            }
        }
    }
    else if( portIndex == 1 )
    {
        if( nodeData )
        {
            if( mpContourPointsData )
                mpCVImageInData = nullptr;
            auto d = std::dynamic_pointer_cast< ContourPointsData >( nodeData );
            if( d )
            {
                mpContourPointsData = d;
                if( mpCVImageInData )
                {
                    processData(mpCVImageInData, mpCVImageOutData, mpContourPointsData, mParams);
                    Q_EMIT dataUpdated(0);
                }
            }
        }
    }
}

QJsonObject
DrawContourModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDataModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams[ "bValue" ] = mParams.mucBValue;
    cParams[ "gValue" ] = mParams.mucGValue;
    cParams[ "rValue" ] = mParams.mucRValue;
    cParams[ "lineThickness" ] = mParams.miLineThickness;
    cParams[ "lineType" ] = mParams.miLineType;
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
DrawContourModel::
restore(const QJsonObject &p)
{
    /*
     * If restore() was overrided, PBNodeDataModel::restore() must be called explicitely.
     */
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "bValue" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "b_value" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >(prop);
            typedProp->getData().mucValue = v.toInt();

            mParams.mucBValue = v.toInt();
        }
        v = paramsObj[ "gValue" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "g_value" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >(prop);
            typedProp->getData().mucValue = v.toInt();

            mParams.mucGValue = v.toInt();
        }
        v = paramsObj[ "rValue" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "r_value" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < UcharPropertyType > >(prop);
            typedProp->getData().mucValue = v.toInt();

            mParams.mucRValue = v.toInt();
        }
        v = paramsObj[ "lineThickness" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "line_thickness" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < IntPropertyType > >(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miLineThickness = v.toInt();
        }
        v = paramsObj[ "lineType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "line_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < EnumPropertyType > >(prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miLineType = v.toInt();
        }
    }


}

void
DrawContourModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if(id=="b_value")
    {
        auto TypedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        TypedProp->getData().mucValue = value.toInt();
        mParams.mucBValue = value.toInt();
    }
    else if(id=="g_value")
    {
        auto TypedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        TypedProp->getData().mucValue = value.toInt();
        mParams.mucGValue = value.toInt();
    }
    else if(id=="r_value")
    {
        auto TypedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        TypedProp->getData().mucValue = value.toInt();
        mParams.mucRValue = value.toInt();
    }
    else if(id=="line_thickness")
    {
        auto TypedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        TypedProp->getData().miValue = value.toInt();
        mParams.miLineThickness = value.toInt();
    }
    else if(id=="line_type")
    {
        auto TypedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        TypedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt())
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

    if(mpCVImageInData && mpContourPointsData)
    {
        processData(mpCVImageInData, mpCVImageOutData, mpContourPointsData, mParams);
        Q_EMIT dataUpdated(0);
    }
}

void DrawContourModel::processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<CVImageData> &outImage,
                                   std::shared_ptr<ContourPointsData> &ctrPnts, const DrawContourParameters &params)
{
    cv::Mat& in_image = in->data();
    if(in_image.empty())
        return;
    cv::Mat& out_image = outImage->data();
    in_image.copyTo( out_image );
    std::vector<std::vector<cv::Point>> vvPtContours = ctrPnts->data();

    cv::drawContours(out_image, vvPtContours, -1,
                     cv::Vec3b(static_cast<uchar>(params.mucBValue),static_cast<uchar>(params.mucGValue),static_cast<uchar>(params.mucRValue)),
                     params.miLineThickness,params.miLineType);
}

const QString DrawContourModel::_category = QString( "Image Processing" );

const QString DrawContourModel::_model_name = QString( "Draw Contour" );
