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

#include "ThresholdingModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

ThresholdingModel::
ThresholdingModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":Thresholding.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpIntegerData = std::make_shared< IntegerData >( int() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"THRESH_MASK", "THRESH_OTSU", "THRESH_TRUNC", "THRESH_BINARY", "THRESH_TOZERO", "THRESH_TRIANGLE", "THRESH_BINARY_INV", "THRESH_TOZERO_INV", "ADAPTIVE_THRESH_MEAN_C", "ADAPTIVE_THRESH_GAUSSIAN_C"});
    enumPropertyType.miCurrentIndex = 3;
    QString propId = "threshold_type";
    auto propThresholdType = std::make_shared< TypedProperty< EnumPropertyType > >( "Threshold Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propThresholdType );
    mMapIdToProperty[ propId ] = propThresholdType;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdThresholdValue;
    doublePropertyType.mdMax = 255;
    propId = "threshold_value";
    auto propThresholdValue = std::make_shared< TypedProperty< DoublePropertyType > >( "Threshold Value", propId, QMetaType::Double, doublePropertyType, "Operation" );
    mvProperty.push_back( propThresholdValue );
    mMapIdToProperty[ propId ] = propThresholdValue;

    doublePropertyType.mdValue = mParams.mdBinaryValue;
    doublePropertyType.mdMax = 255;
    propId = "binary_value";
    auto propBinaryValue = std::make_shared< TypedProperty< DoublePropertyType > >( "Binary Value", propId, QMetaType::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propBinaryValue );
    mMapIdToProperty[ propId ] = propBinaryValue;
}

unsigned int
ThresholdingModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
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
ThresholdingModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 1)
    {
        return IntegerData().type();
    }
    return CVImageData().type();
}


std::shared_ptr<NodeData>
ThresholdingModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        if(I == 0 && mpCVImageData->data().data != nullptr )
        {
            return mpCVImageData;
        }
        else if(I == 1)
        {
            return mpIntegerData;
        }
    }
    return nullptr;
}

void
ThresholdingModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mpCVImageData, mpIntegerData, mParams);
        }
    }

    updateAllOutputPorts();
}

QJsonObject
ThresholdingModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["thresholdType"] = mParams.miThresholdType;
    cParams["thresholdValue"] = mParams.mdThresholdValue;
    cParams["binaryValue"] = mParams.mdBinaryValue;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
ThresholdingModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "thresholdType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "threshold_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miThresholdType = v.toInt();
        }
        v =  paramsObj[ "thresholdValue" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "threshold_value" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdThresholdValue = v.toDouble();
        }
        v =  paramsObj[ "binaryValue" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "binary_value" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdBinaryValue = v.toDouble();
        }
    }
}

void
ThresholdingModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "threshold_type" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miThresholdType = cv::THRESH_MASK;
            break;

        case 1:
            mParams.miThresholdType = cv::THRESH_OTSU;
            break;

        case 2:
            mParams.miThresholdType = cv::THRESH_TRUNC;
            break;

        case 3:
            mParams.miThresholdType = cv::THRESH_BINARY;
            break;

        case 4:
            mParams.miThresholdType = cv::THRESH_TOZERO;
            break;

        case 5:
            mParams.miThresholdType = cv::THRESH_TRIANGLE;
            break;

        case 6:
            mParams.miThresholdType = cv::THRESH_BINARY_INV;
            break;

        case 7:
            mParams.miThresholdType = cv::THRESH_TOZERO_INV;
            break;

        case 8:
            mParams.miThresholdType = cv::ADAPTIVE_THRESH_MEAN_C;
            break;

        case 9:
            mParams.miThresholdType = cv::ADAPTIVE_THRESH_GAUSSIAN_C;
            break;
        }
    }
    else if( id == "threshold_value" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdThresholdValue = value.toDouble();
    }
    else if( id == "binary_value" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdBinaryValue = value.toDouble();
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mpIntegerData, mParams);
        updateAllOutputPorts();
    }
}

void
ThresholdingModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr<CVImageData> & outImage,
            std::shared_ptr<IntegerData> &outInt, const ThresholdingParameters & params)
{
    cv::Mat& in_image = in->data();
    if(params.miThresholdType == cv::THRESH_OTSU || params.miThresholdType == cv::THRESH_TRIANGLE)
    {
        if(in_image.empty() || (in_image.type()!=CV_8UC1 && in_image.type()!=CV_8SC1))
        {
            return;
        }
        outInt->data() = cv::threshold(in_image,outImage->data(),params.mdThresholdValue,params.mdBinaryValue,params.miThresholdType);
    }
    else
    {
        if(in_image.empty() || (in_image.depth()!=CV_8U && in_image.depth()!=CV_8S && in_image.depth()!=CV_32F))
        {
            return;
        }
        cv::threshold(in_image,outImage->data(),params.mdThresholdValue,params.mdBinaryValue,params.miThresholdType);
        outInt->data() = 0;
    }
}

const QString ThresholdingModel::_category = QString( "Image Conversion" );

const QString ThresholdingModel::_model_name = QString( "Thresholding" );
