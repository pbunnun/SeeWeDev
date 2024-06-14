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

#include "MatrixOperationModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

MatrixOperationModel::
MatrixOperationModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":MatrixOperation.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"+","-",">",">=","<","<=","*","/","MAX","MIN"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "operator";
    auto propOperator = std::make_shared< TypedProperty< EnumPropertyType > >( "Operator", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propOperator );
    mMapIdToProperty[ propId ] = propOperator;
}

unsigned int
MatrixOperationModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 2;
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
MatrixOperationModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
MatrixOperationModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
MatrixOperationModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mapCVImageInData[portIndex] = d;
            if(mapCVImageInData[0]&&mapCVImageInData[1])
            {
                processData( mapCVImageInData, mpCVImageData, mParams );
            }
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
MatrixOperationModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["operator"] = mParams.miOperator;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
MatrixOperationModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "operator" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "operator" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miOperator = v.toInt();
        }
    }
}

void
MatrixOperationModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "operator" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        mParams.miOperator = value.toInt();
    }

    if( mapCVImageInData[0] && mapCVImageInData[1] )
    {
        processData( mapCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }
}

void
MatrixOperationModel::
processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr<CVImageData> & out,
            const MatrixOperationParameters & params )
{
    cv::Mat& in0 = in[0]->data();
    cv::Mat& in1 = in[1]->data();
    if(in0.empty() || in1.empty())
    {
        return;
    }
    if(in0.rows==in1.rows && in0.cols==in1.cols)
    {
        switch(params.miOperator)
        {
        case MatOps::PLUS :
            out->set_image(in0 + in1);
            break;

        case MatOps::MINUS :
            out->set_image(in0 - in1);
            break;

        case MatOps::GREATER_THAN :
            out->set_image(in0 > in1);
            break;

        case MatOps::GREATER_THAN_OR_EQUAL :
            out->set_image(in0 >= in1);
            break;

        case MatOps::LESSER_THAN :
            out->set_image(in0 < in1);
            break;

        case MatOps::LESSER_THAN_OR_EQUAL :
            out->set_image(in0 <= in1);
            break;

        case MatOps::MULTIPLY :
            out->set_image(in0 * in1);
            break;

        case MatOps::DIVIDE :
            out->set_image(in0 / in1);
            break;

        case MatOps::MAXIMUM :
            out->set_image( min(in0,in1) );
            break;

        case MatOps::MINIMUM :
            out->set_image( max(in0,in1) );
            break;
        }
    }
}

const QString MatrixOperationModel::_category = QString( "Image Operation" );

const QString MatrixOperationModel::_model_name = QString( "Matrix Operation" );
