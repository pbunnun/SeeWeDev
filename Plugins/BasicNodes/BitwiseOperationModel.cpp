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

#include "BitwiseOperationModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

BitwiseOperationModel::
BitwiseOperationModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget(new BitwiseOperationEmbeddedWidget),
      _minPixmap( ":BitwiseOperation.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList( {"AND", "OR", "XOR"} );
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "bitwise_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Bitwise", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;

    propId = "active_mask";
    auto propActiveMask = std::make_shared<TypedProperty<bool>>("", propId, QMetaType::Bool, mProps.mbActiveMask);
    mMapIdToProperty[ propId ] = propActiveMask;
}

unsigned int
BitwiseOperationModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 3;
        break;

    case PortType::Out:
        result = 1;
        break;

    default:
        break;
    }

    return result;
}


NodeDataType
BitwiseOperationModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
BitwiseOperationModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
BitwiseOperationModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mapCVImageInData[portIndex] = d;
            if(mapCVImageInData[0] && mapCVImageInData[1])
            {
                processData(mapCVImageInData,mpCVImageData,mParams,mProps);
            }
        }
    }
    Q_EMIT dataUpdated(0);
}

QJsonObject
BitwiseOperationModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["bitwiseType"] = mParams.miBitwiseType;
    cParams["activeMask"] = mProps.mbActiveMask;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
BitwiseOperationModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "bitwiseType" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty["bitwise_type"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miBitwiseType = v.toInt();
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
BitwiseOperationModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "bitwise_type" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        mParams.miBitwiseType = value.toInt();
    }

    if( mapCVImageInData[0] && mapCVImageInData[1] )
    {
        processData(mapCVImageInData,mpCVImageData,mParams,mProps);

        Q_EMIT dataUpdated(0);
    }
}

void BitwiseOperationModel::processData(const std::shared_ptr<CVImageData> (&in)[3], std::shared_ptr<CVImageData> &out, const BitwiseOperationParameters &params, BitwiseOperationProperties &props)
{
    cv::Mat& in_image0 = in[0]->data();
    cv::Mat& in_image1 = in[1]->data();
    if(in_image0.empty() || in_image1.empty() || in_image0.type()!=in_image1.type())
    { //Extra condition buffer added to allow the program to load properly.
        return;
    }
    cv::Mat& out_image = out->data();
    props.mbActiveMask = (in[2]!=nullptr && !in[2]->data().empty() &&
                          in[2]->data().type()==CV_8UC1)? true : false ;
    mpEmbeddedWidget->set_maskStatus_label(props.mbActiveMask);
    if(props.mbActiveMask)
    {
        cv::Mat& mask = in[2]->data();
        switch(params.miBitwiseType)
        {
        case 0:
            cv::bitwise_and(in_image0,in_image1,out_image,mask);
            break;

        case 1:
            cv::bitwise_or(in_image0,in_image1,out_image,mask);
            break;

        case 2:
            cv::bitwise_xor(in_image0,in_image1,out_image,mask);
            break;
        }
    }
    else
    {
        switch(params.miBitwiseType)
        {
        case 0:
            cv::bitwise_and(in_image0,in_image1,out_image);
            break;

        case 1:
            cv::bitwise_or(in_image0,in_image1,out_image);
            break;

        case 2:
            cv::bitwise_xor(in_image0,in_image1,out_image);
            break;
        }
    }
}

const QString BitwiseOperationModel::_category = QString( "Image Operation" );

const QString BitwiseOperationModel::_model_name = QString( "Bitwise Operation" );
