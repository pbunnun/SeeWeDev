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

#include "CVSplitImageModel.hpp"

#include <QDebug>

#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

const QString CVSplitImageModel::_category = QString("Image Conversion");

const QString CVSplitImageModel::_model_name = QString( "CV Split Image" );

CVSplitImageModel::

CVSplitImageModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":SplitImage.png" )
{
    for(std::shared_ptr<CVImageData>& mp : mapCVImageData)
    {
        mp = std::make_shared< CVImageData >( cv::Mat() );
    }
    QString propId = "maintain_channels";
    auto propMaintainChannels = std::make_shared< TypedProperty < bool > >("Maintain Channels", propId, QMetaType::Bool, mParams.mbMaintainChannels, "Display");
    mvProperty.push_back( propMaintainChannels);
    mMapIdToProperty[ propId ] = propMaintainChannels;
}

unsigned int
CVSplitImageModel::

nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 3;
        break;

    default:
        break;
    }

    return result;
}

NodeDataType
CVSplitImageModel::

dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}

std::shared_ptr<NodeData>
CVSplitImageModel::

outData(PortIndex I)
{
    if( isEnable() )
        return mapCVImageData[I];
    else
        return nullptr;
}

void
CVSplitImageModel::

setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if( nodeData )
    {
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mapCVImageData, mParams);
        }
    }

    updateAllOutputPorts();
}

QJsonObject
CVSplitImageModel::

save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["maintainChannels"] = mParams.mbMaintainChannels;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVSplitImageModel::

load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "maintainChannels" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "maintain_channels" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mParams.mbMaintainChannels = v.toBool();
        }
    }
}

void
CVSplitImageModel::

setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "maintain_channels" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <bool>>(prop);
        typedProp->getData() = value.toBool();

        mParams.mbMaintainChannels = value.toBool();
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mapCVImageData, mParams );
        updateAllOutputPorts();
    }
}

void
CVSplitImageModel::

processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr< CVImageData > (&out)[3], const SplitImageParameters &params)
{
    cv::Mat& in_image = in->data();
    if(in_image.empty() || in_image.channels()!=3)
    {
        return;
    }
    std::vector<cv::Mat> vImage;
    cv::split(in_image,vImage);
    if(params.mbMaintainChannels)
    {
        for(int i=0; i<3; i++)
        {
            cv::Mat arr[3];
            for(int j=0; j<3; j++)
            {
                arr[j] = (j==i)? vImage[i] : cv::Mat::zeros(vImage[i].size(), vImage[i].type()) ;
            }
            cv::merge(arr,3,out[i]->data());
        }
    }
    else
    {
        for(int i=0; i<3; i++)
        {
            out[i]->set_image(vImage[i]);
        }
    }
}


