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

#include "CVColorMapModel.hpp"

#include <QDebug> //for debugging using qDebug()


#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVColorMapModel::_category = QString( "Image Analysis" );

const QString CVColorMapModel::_model_name = QString( "CV Color Map" );

CVColorMapModel::
CVColorMapModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":CVColorMap.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared<SyncData>();

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"COLORMAP_HOT",
    "COLORMAP_HSV","COLORMAP_JET","COLORMAP_BONE","COLORMAP_COOL",
    "COLORMAP_PINK","COLORMAP_MAGMA","COLORMAP_OCEAN","COLORMAP_TURBO",
    "COLORMAP_AUTUMN","COLORMAP_PARULA","COLORMAP_PLASMA","COLORMAP_SPRING",
    "COLORMAP_SUMMER","COLORMAP_WINTER","COLORMAP_CIVIDIS","COLORMAP_INFERNO",
    "COLORMAP_RAINBOW","COLORMAP_VIRIDIS","COLORMAP_TWILIGHT","COLORMAP_TWILIGHT_SHIFTED"});
    enumPropertyType.miCurrentIndex = 2;
    QString propId = "color_map";
    auto propCVColorMap = std::make_shared< TypedProperty< EnumPropertyType > >( "Color Map", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propCVColorMap );
    mMapIdToProperty[ propId ] = propCVColorMap;
}

unsigned int
CVColorMapModel::
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
CVColorMapModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 1)
        return SyncData().type();
    else
        return CVImageData().type();
}


std::shared_ptr<NodeData>
CVColorMapModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        if(I == 0)
            return mpCVImageData;
        else if(I == 1)
            return mpSyncData;
    }
    return nullptr;
}

void
CVColorMapModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mpCVImageData, mParams );
        }
        mpSyncData->data() = true;
        Q_EMIT dataUpdated(1);
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
CVColorMapModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["colorMap"] = mParams.miCVColorMap;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVColorMapModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "colorMap" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "color_map" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miCVColorMap = v.toInt();
        }
    }
}

void
CVColorMapModel::
setModelProperty( QString & id, const QVariant & value )
{
    mpSyncData->data() = false;
    Q_EMIT dataUpdated(1);

    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "color_map" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miCVColorMap = cv::COLORMAP_HOT;
            break;

        case 1:
            mParams.miCVColorMap = cv::COLORMAP_HSV;
            break;

        case 2:
            mParams.miCVColorMap = cv::COLORMAP_JET;
            break;

        case 3:
            mParams.miCVColorMap = cv::COLORMAP_BONE;
            break;

        case 4:
            mParams.miCVColorMap = cv::COLORMAP_COOL;
            break;

        case 5:
            mParams.miCVColorMap = cv::COLORMAP_PINK;
            break;

        case 6:
            mParams.miCVColorMap = cv::COLORMAP_MAGMA;
            break;

        case 7:
            mParams.miCVColorMap = cv::COLORMAP_OCEAN;
            break;

        case 8:
            mParams.miCVColorMap = cv::COLORMAP_TURBO;
            break;

        case 9:
            mParams.miCVColorMap = cv::COLORMAP_AUTUMN;
            break;

        case 10:
            mParams.miCVColorMap = cv::COLORMAP_PARULA;
            break;

        case 11:
            mParams.miCVColorMap = cv::COLORMAP_PLASMA;
            break;

        case 12:
            mParams.miCVColorMap = cv::COLORMAP_SPRING;
            break;

        case 13:
            mParams.miCVColorMap = cv::COLORMAP_SUMMER;
            break;

        case 14:
            mParams.miCVColorMap = cv::COLORMAP_WINTER;
            break;

        case 15:
            mParams.miCVColorMap = cv::COLORMAP_CIVIDIS;
            break;

        case 16:
            mParams.miCVColorMap = cv::COLORMAP_INFERNO;
            break;

        case 17:
            mParams.miCVColorMap = cv::COLORMAP_RAINBOW;
            break;

        case 18:
            mParams.miCVColorMap = cv::COLORMAP_VIRIDIS;
            break;

        case 19:
            mParams.miCVColorMap = cv::COLORMAP_TWILIGHT;
            break;

        case 20:
            mParams.miCVColorMap = cv::COLORMAP_TWILIGHT_SHIFTED;
            break;
        }
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }

    mpSyncData->data() = true;
    Q_EMIT dataUpdated(1);
}

void
CVColorMapModel::
processData(const std::shared_ptr< CVImageData > &in, std::shared_ptr<CVImageData> & out,
            const CVColorMapParameters & params )
{
    cv::Mat& in_image = in->data();
    if(!in_image.empty() && (in_image.type()==CV_8UC1 || in_image.type()==CV_8UC3))
    {
        cv::applyColorMap(in_image,out->data(),params.miCVColorMap);
    }
}

