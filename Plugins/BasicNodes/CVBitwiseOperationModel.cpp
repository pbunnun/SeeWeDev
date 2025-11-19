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

#include "CVBitwiseOperationModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>
#include <QtNodes/internal/ConnectionIdUtils.hpp>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "qtvariantproperty_p.h"

const QString CVBitwiseOperationModel::_category = QString( "Image Operation" );

const QString CVBitwiseOperationModel::_model_name = QString( "CV Bitwise Operation" );

CVBitwiseOperationModel::
CVBitwiseOperationModel()
    : PBNodeDelegateModel( _model_name ),
      mpEmbeddedWidget( new QLabel("AND") ),
      _minPixmap( ":BitwiseOperation.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mvCVImageInData.resize(3);

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList( {"AND", "OR", "XOR", "NOT"} );
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "bitwise_type";
    auto propBitwiseOperationType = std::make_shared< TypedProperty< EnumPropertyType > >( "Bitwise", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation" );
    mvProperty.push_back( propBitwiseOperationType );
    mMapIdToProperty[ propId ] = propBitwiseOperationType;
}

unsigned int
CVBitwiseOperationModel::
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
CVBitwiseOperationModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVBitwiseOperationModel::
outData(PortIndex)
{
    if( isEnable() && mpCVImageData->data().empty() == false )
        return mpCVImageData;
    else
        return nullptr;
}

void
CVBitwiseOperationModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            d->data().copyTo( mvCVImageInData[portIndex] );
            if( !mvCVImageInData[0].empty() && ( mBitwiseOperationType == BITWISE_NOT || !mvCVImageInData[1].empty() ) &&
                ( !mbMaskActive || !mvCVImageInData[2].empty() ) )
            {
                processData(mvCVImageInData, mpCVImageData, mBitwiseOperationType);

                Q_EMIT dataUpdated(0);
            }
        }
    }
}

QJsonObject
CVBitwiseOperationModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["bitwise_type"] = mBitwiseOperationType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVBitwiseOperationModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "bitwise_type" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["bitwise_type"];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mBitwiseOperationType = static_cast<BitwiseOperationType>(v.toInt());
        }
    }
}

void
CVBitwiseOperationModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "bitwise_type" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        mBitwiseOperationType = static_cast<BitwiseOperationType>(value.toInt());
        switch( mBitwiseOperationType )
        {
            case BITWISE_AND:
                mpEmbeddedWidget->setText("AND");
                break;
            case BITWISE_OR:
                mpEmbeddedWidget->setText("OR");
                break;
            case BITWISE_XOR:
                mpEmbeddedWidget->setText("XOR");
                break;
            case BITWISE_NOT:
                mpEmbeddedWidget->setText("NOT");
                break;
            default:
                mpEmbeddedWidget->setText("AND");
                break;
        }
    }

    if( !mvCVImageInData[0].empty() && ( mBitwiseOperationType == BITWISE_NOT || !mvCVImageInData[1].empty() ) &&
        ( !mbMaskActive || !mvCVImageInData[2].empty() ) )
    {
        processData(mvCVImageInData, mpCVImageData, mBitwiseOperationType);

        Q_EMIT dataUpdated(0);
    }
}

void
CVBitwiseOperationModel::
processData(const std::vector< cv::Mat >&in, std::shared_ptr<CVImageData> &out, const BitwiseOperationType type)
{
    if( type == BITWISE_NOT )
    {
        cv::Mat temp;
        if( mbMaskActive && in[2].type() == CV_8UC1 )
        {
            cv::bitwise_not(in[0], temp, in[2]);
        }
        else
        {
            cv::bitwise_not(in[0], temp);
        }
        temp.copyTo( out->data() );
        return;
    }
    else
    {
        if(in[0].type()!=in[1].type())
        { //Extra condition buffer added to allow the program to load properly.
            return;
        }
        cv::Mat temp;
        if(mbMaskActive && !in[2].empty() && in[2].type() == CV_8UC1)
        {
            if( type == BITWISE_AND )
            {
                cv::bitwise_and(in[0], in[1], temp, in[2]);
            }
            else if( type == BITWISE_OR )
            {
                cv::bitwise_or(in[0], in[1], temp, in[2]);
            }
            else if( type == BITWISE_XOR )
            {
                cv::bitwise_xor(in[0], in[1], temp, in[2]);
            }
        }
        else
        {
            if( type == BITWISE_AND )
            {
                cv::bitwise_and(in[0], in[1], temp);
            }
            else if( type == BITWISE_OR )
            {
                cv::bitwise_or(in[0], in[1], temp);
            }
            else if( type == BITWISE_XOR )
            {
                cv::bitwise_xor(in[0], in[1], temp);
            }
        }
        temp.copyTo( out->data() );
    }
}

void
CVBitwiseOperationModel::
inputConnectionCreated(QtNodes::ConnectionId const& conx)
{
    auto idx = QtNodes::getPortIndex(PortType::In, conx);
    if( idx == 2 )
        mbMaskActive = true;
}

void
CVBitwiseOperationModel::
inputConnectionDeleted(QtNodes::ConnectionId const& conx)
{
    auto idx = QtNodes::getPortIndex(PortType::In, conx);
    mvCVImageInData[idx].release();
    if( idx == 2 )
    {
        mbMaskActive = false;
        if( !mvCVImageInData[0].empty() && ( mBitwiseOperationType == BITWISE_NOT || !mvCVImageInData[1].empty() ) )
        {
            processData(mvCVImageInData, mpCVImageData, mBitwiseOperationType);

            Q_EMIT dataUpdated(0);
        }
    }
}
