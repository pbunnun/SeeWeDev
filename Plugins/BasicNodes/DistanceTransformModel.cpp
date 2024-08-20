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

#include "DistanceTransformModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

DistanceTransformModel::
DistanceTransformModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":DistanceTransform.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"DIST_C", "DIST_L1", "DIST_L2", "DIST_L12"});
    enumPropertyType.miCurrentIndex = 2;
    QString propId = "operation_type";
    auto propOperationType = std::make_shared<TypedProperty<EnumPropertyType>>("Operation Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back(propOperationType);
    mMapIdToProperty[ propId ] = propOperationType;

    enumPropertyType.mslEnumNames = QStringList({"0", "3", "5"});
    propId = "mask_size";
    enumPropertyType.miCurrentIndex = 1;
    auto propMaskSize = std::make_shared< TypedProperty< EnumPropertyType > >( "Mask Size", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propMaskSize );
    mMapIdToProperty[ propId ] = propMaskSize;
}

unsigned int
DistanceTransformModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
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
DistanceTransformModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
DistanceTransformModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
DistanceTransformModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mpCVImageData, mParams );
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
DistanceTransformModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["operationType"] = mParams.miOperationType;
    cParams["maskSize"] = mParams.miMaskSize;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
DistanceTransformModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "operationType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "operation_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miOperationType = v.toInt();
        }
        v =  paramsObj[ "maskSize" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "mask_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miMaskSize = v.toInt();
        }
    }
}

void
DistanceTransformModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "operation_type" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miOperationType = cv::DIST_C;
            break;

        case 1:
            mParams.miOperationType = cv::DIST_L1;
            break;

        case 2:
            mParams.miOperationType = cv::DIST_L2;
            break;

        case 3:
            mParams.miOperationType = cv::DIST_L12;
            break;
        }
    }
    else if( id == "mask_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        mParams.miMaskSize = typedProp->getData().mslEnumNames[value.toInt()].toInt();
        qDebug()<<mParams.miMaskSize;
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }
}

void
DistanceTransformModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr<CVImageData> & out,
            const DistanceTransformParameters & params )
{
    cv::Mat& in_image= in->data();
    if(in_image.empty() || (in_image.type()!=CV_8UC1 && in_image.type()!=CV_8SC1))
    {
        return;
    }
    bool isBinary = true;
    double arr[2];
    cv::minMaxLoc(in_image,&arr[0],&arr[1]);
    if(in_image.depth()==CV_8U)
    {
        for(int i=0; i<in_image.rows; i++)
        {
            if(!isBinary)
            {
                break;
            }
            for(int j=0; j<in_image.cols; j++)
            {
                double val = static_cast<double>(in_image.at<uchar>(i,j));
                if(val!=arr[0] && val!=arr[1])
                {
                    isBinary = false;
                    break;
                }
            }
        }
    }
    else if(in_image.depth()==CV_8S)
    {
        for(int i=0; i<in_image.rows; i++)
        {
            if(!isBinary)
            {
                break;
            }
            for(int j=0; j<in_image.cols; j++)
            {
                double val = static_cast<double>(in_image.at<char>(i,j));
                if(val!=arr[0] && val!=arr[1])
                {
                    isBinary = false;
                    break;
                }
            }
        }
    }
    if(!isBinary)
    {
        return;
    }
    cv::Mat Temp;
    cv::distanceTransform(in->data(),Temp,params.miOperationType,params.miMaskSize,CV_32F);
    cv::convertScaleAbs(Temp,out->data());
}

const QString DistanceTransformModel::_category = QString( "Image Processing" );

const QString DistanceTransformModel::_model_name = QString( "Distance Transform" );
