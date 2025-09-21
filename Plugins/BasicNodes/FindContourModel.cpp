//Copyright © 2024, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "FindContourModel.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QEvent>
#include <QDir>
#include "qtvariantproperty.h"

#include <QMessageBox>

#include <nodes/DataModelRegistry>

FindContourModel::
FindContourModel()
    : PBNodeDataModel( _model_name )
{
    mpContourPointsData = std::make_shared< ContourPointsData >( );
    mpSyncData = std::make_shared< SyncData >( );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"RETR_LIST","RETR_TREE","RETR_CCOMP","RETR_EXTERNAL","RETR_FLOODFILL"});
    enumPropertyType.miCurrentIndex = 1; //initializing this to 0 somehow produces an unexpected output
    QString propId = "contour_mode";
    auto propContourMode = std::make_shared< TypedProperty< EnumPropertyType > >("Contour Mode", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propContourMode );
    mMapIdToProperty[ propId ] = propContourMode;

    enumPropertyType.mslEnumNames = QStringList( {"CHAIN_APPROX_NONE", "CHAIN_APPROX_SIMPLE", "CHAIN_APPROX_TC89_L1","CHAIN_APPROX_TC89_KCOS"} );
    enumPropertyType.miCurrentIndex = 1; //initializing this to 0 somehow produces an unexpected output
    propId = "contour_method";
    auto propContourMethod = std::make_shared< TypedProperty< EnumPropertyType > >( "Contour Method", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propContourMethod );
    mMapIdToProperty[ propId ] = propContourMethod;
}

unsigned int
FindContourModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 1 );
    case PortType::Out:
        return( 2 );
    default:
        return( -1 );
    }
}

NodeDataType
FindContourModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if(portIndex == 0)
            return CVImageData().type();
    }
    else if( portType == PortType::Out )
    {
        if(portIndex == 0)
            return ContourPointsData().type();
        else if(portIndex == 1)
            return SyncData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
FindContourModel::
outData(PortIndex idx)
{
    if( isEnable() )
    {
        if(idx == 0 && mpContourPointsData->data().size() != 0)
            return mpContourPointsData;
        else if(idx == 1)
            return mpSyncData;
    }
    return nullptr;
}

void
FindContourModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if( nodeData )
    {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( !d->data().empty() )
        {
            mpCVImageInData = d;
            processData(mpCVImageInData, mpContourPointsData, mParams);
            Q_EMIT dataUpdated(0);
        }
        mpSyncData->data() = true;
        Q_EMIT dataUpdated(1);
    }
}

QJsonObject
FindContourModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDataModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams[ "contourMode" ] = mParams.miContourMode;
    cParams[ "contourMethod" ] = mParams.miContourMethod;
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
FindContourModel::
restore(const QJsonObject &p)
{
    /*
     * If restore() was overrided, PBNodeDataModel::restore() must be called explicitely.
     */
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "contourMode" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "contour_mode" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miContourMode = v.toInt();

        }
        v = paramsObj[ "contourMethod" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "contour_method" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miContourMethod = v.toInt();
        }
    }
}

void
FindContourModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "contour_mode" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt())
        { //{"RETR_LIST","RETR_TREE","RETR_CCOMP","RETR_EXTERNAL","RETR_FLOODFILL"}
        case 0:
            mParams.miContourMode = cv::RETR_LIST;
            break;

        case 1:
            mParams.miContourMode = cv::RETR_TREE;
            break;

        case 2:
            mParams.miContourMode = cv::RETR_CCOMP;
            break;

        case 3:
            mParams.miContourMode = cv::RETR_EXTERNAL;
            break;

        case 4: //produces a bug when this case is executed
            mParams.miContourMode = cv::RETR_FLOODFILL;
            break;
        }
    }
    else if( id == "contour_method" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt())
        {
        case 0:
            mParams.miContourMethod = cv::CHAIN_APPROX_NONE;
            break;

        case 1:
            mParams.miContourMethod = cv::CHAIN_APPROX_SIMPLE;
            break;

        case 2:
            mParams.miContourMethod = cv::CHAIN_APPROX_TC89_L1;
            break;

        case 3:
            mParams.miContourMethod = cv::CHAIN_APPROX_TC89_KCOS;
            break;
        }
    }

    if(mpCVImageInData)
    {
        processData(mpCVImageInData, mpContourPointsData, mParams);
        updateAllOutputPorts();
    }
}

void FindContourModel::processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<ContourPointsData> &outContour, const ContourParameters &params)
{
    cv::Mat& in_image = in->data();
    if(in_image.empty() || (in_image.type()!=CV_8UC1 && in_image.type()!=CV_8SC1))
    {
        QMessageBox msg;
        QString msgText = "Find an image contour node accepts only 1 chanel image!";
        msg.setIcon( QMessageBox::Critical );
        msg.setText( msgText );
        msg.exec();
        return;
    }
    cv::Mat cvTemp = in_image.clone();
    std::vector<std::vector<cv::Point>> vvPtContours;
    std::vector<cv::Vec4i> vV4iHierarchy;
    cv::findContours(cvTemp, vvPtContours, vV4iHierarchy, params.miContourMode, params.miContourMethod);
    outContour->data() = vvPtContours;
}

const QString FindContourModel::_category = QString( "Image Processing" );

const QString FindContourModel::_model_name = QString( "Find Contour" );
