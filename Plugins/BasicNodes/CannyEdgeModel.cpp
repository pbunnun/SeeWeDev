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

#include "CannyEdgeModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>

CannyEdgeModel::
CannyEdgeModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":CannyEdge.png" )
{
    mpCVImageOutData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >();

    IntPropertyType intPropertyType;
    QString propId = "kernel_size";
    intPropertyType.miValue = mParams.miSizeKernel;
    auto propKernelSize = std::make_shared< TypedProperty< IntPropertyType > >( "Kernel Size", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    intPropertyType.miValue = mParams.miThresholdU;
    intPropertyType.miMax = 255;
    propId = "th_u";
    auto propThresholdU = std::make_shared< TypedProperty< IntPropertyType > >( "Upper Threshold", propId, QMetaType::Int, intPropertyType, "Operation" );
    mvProperty.push_back( propThresholdU );
    mMapIdToProperty[ propId ] = propThresholdU;

    intPropertyType.miValue = mParams.miThresholdL;
    propId = "th_l";
    auto propThresholdL = std::make_shared< TypedProperty< IntPropertyType > >( "Lower Threshold", propId, QMetaType::Int, intPropertyType , "Operation");
    mvProperty.push_back( propThresholdL );
    mMapIdToProperty[ propId ] = propThresholdL;

    propId = "enable_gradient";
    auto propEnableGradient = std::make_shared< TypedProperty < bool > > ("Use Edge Gradient", propId, QMetaType::Bool, mParams.mbEnableGradient, "Operation");
    mvProperty.push_back( propEnableGradient );
    mMapIdToProperty[ propId ] = propEnableGradient;
}

unsigned int
CannyEdgeModel::
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
CannyEdgeModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
    {
        return CVImageData().type();
    }
    else if( portIndex == 1 )
    {
        return SyncData().type();
    }
    return NodeDataType();
}


std::shared_ptr<NodeData>
CannyEdgeModel::
outData(PortIndex idx)
{
    if( isEnable() )
    {
        if( idx == 0 )
            return mpCVImageOutData;
        else if( idx == 1 )
            return mpSyncData;
    }
    return nullptr;
}

void
CannyEdgeModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if( !isEnable() )
        return;

    if (nodeData)
    {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (!d->data().empty())
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mpCVImageOutData, mParams );
        }
        mpSyncData->data() = true;
        Q_EMIT dataUpdated(1);
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
CannyEdgeModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["kernelSize"] = mParams.miSizeKernel;
    cParams["thresholdU"] = mParams.miThresholdU;
    cParams["thresholdL"] = mParams.miThresholdL;
    cParams["enableGradient"] = mParams.mbEnableGradient;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CannyEdgeModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "kernelSize" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miSizeKernel = v.toInt();
        }
        v =  paramsObj[ "thresholdU" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "th_u" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miThresholdU = v.toInt();
        }
        v =  paramsObj[ "thresholdL" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "th_l" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miThresholdL = v.toInt();
        }
        v = paramsObj[ "enableGradient" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "enable_gradient" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < bool > > (prop);
            typedProp->getData() = v.toBool();

            mParams.mbEnableGradient = v.toBool();
        }
    }
}

void
CannyEdgeModel::
setModelProperty( QString & id, const QVariant & value )
{
    mpSyncData->data() = false;
    Q_EMIT dataUpdated(1);
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "kernel_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        int kSize =  value.toInt();
        bool adjValue = false;
        if( kSize > 7 )
        {
            kSize = 7;
            adjValue = true;
        }
        if( kSize < 3 )
        {
            kSize = 3;
            adjValue = true;
        }
        if( kSize >=3 && kSize <=7 && kSize%2!=1 )
        {
            kSize += 1;
            adjValue = true;
        }
        if( adjValue )
        {
            typedProp->getData().miValue = kSize;

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = kSize;

            mParams.miSizeKernel = kSize;
        }
    }
    else if( id == "th_u" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miThresholdU = value.toInt();
    }
    else if( id == "th_l" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miThresholdL = value.toInt();
    }
    else if( id == "enable_gradient" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <bool>>(prop);
        typedProp->getData() = value.toBool();

        mParams.mbEnableGradient = value.toBool();
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageOutData, mParams );

        Q_EMIT dataUpdated(0);
    }
    mpSyncData->data() = true;
    Q_EMIT dataUpdated(1);
}

void
CannyEdgeModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr<CVImageData> & out,
            const CannyEdgeParameters & params )
{
    cv::Mat& in_image = in->data();
    if(in_image.empty() || (in_image.depth()!=CV_8U && in_image.depth()!=CV_8S) )
    {
        return;
    }
    cv::Canny(in_image, out->data(), params.miThresholdL, params.miThresholdU, params.miSizeKernel, params.mbEnableGradient);
}

const QString CannyEdgeModel::_category = QString( "Image Conversion" );

const QString CannyEdgeModel::_model_name = QString( "Canny Edge" );
