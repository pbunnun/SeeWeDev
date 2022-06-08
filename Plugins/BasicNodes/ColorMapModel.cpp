#include "ColorMapModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

ColorMapModel::
ColorMapModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":ColorMap.png" )
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
    auto propColorMap = std::make_shared< TypedProperty< EnumPropertyType > >( "Color Map", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propColorMap );
    mMapIdToProperty[ propId ] = propColorMap;
}

unsigned int
ColorMapModel::
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
ColorMapModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 1)
        return SyncData().type();
    else
        return CVImageData().type();
}


std::shared_ptr<NodeData>
ColorMapModel::
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
ColorMapModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        mpSyncData->state() = false;
        Q_EMIT dataUpdated(1);
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mpCVImageData, mParams );
        }
        mpSyncData->state() = true;
        Q_EMIT dataUpdated(1);
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
ColorMapModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["colorMap"] = mParams.miColorMap;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
ColorMapModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "colorMap" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "color_map" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miColorMap = v.toInt();
        }
    }
}

void
ColorMapModel::
setModelProperty( QString & id, const QVariant & value )
{
    mpSyncData->state() = false;
    Q_EMIT dataUpdated(1);

    PBNodeDataModel::setModelProperty( id, value );

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
            mParams.miColorMap = cv::COLORMAP_HOT;
            break;

        case 1:
            mParams.miColorMap = cv::COLORMAP_HSV;
            break;

        case 2:
            mParams.miColorMap = cv::COLORMAP_JET;
            break;

        case 3:
            mParams.miColorMap = cv::COLORMAP_BONE;
            break;

        case 4:
            mParams.miColorMap = cv::COLORMAP_COOL;
            break;

        case 5:
            mParams.miColorMap = cv::COLORMAP_PINK;
            break;

        case 6:
            mParams.miColorMap = cv::COLORMAP_MAGMA;
            break;

        case 7:
            mParams.miColorMap = cv::COLORMAP_OCEAN;
            break;

        case 8:
            mParams.miColorMap = cv::COLORMAP_TURBO;
            break;

        case 9:
            mParams.miColorMap = cv::COLORMAP_AUTUMN;
            break;

        case 10:
            mParams.miColorMap = cv::COLORMAP_PARULA;
            break;

        case 11:
            mParams.miColorMap = cv::COLORMAP_PLASMA;
            break;

        case 12:
            mParams.miColorMap = cv::COLORMAP_SPRING;
            break;

        case 13:
            mParams.miColorMap = cv::COLORMAP_SUMMER;
            break;

        case 14:
            mParams.miColorMap = cv::COLORMAP_WINTER;
            break;

        case 15:
            mParams.miColorMap = cv::COLORMAP_CIVIDIS;
            break;

        case 16:
            mParams.miColorMap = cv::COLORMAP_INFERNO;
            break;

        case 17:
            mParams.miColorMap = cv::COLORMAP_RAINBOW;
            break;

        case 18:
            mParams.miColorMap = cv::COLORMAP_VIRIDIS;
            break;

        case 19:
            mParams.miColorMap = cv::COLORMAP_TWILIGHT;
            break;

        case 20:
            mParams.miColorMap = cv::COLORMAP_TWILIGHT_SHIFTED;
            break;
        }
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }

    mpSyncData->state() = true;
    Q_EMIT dataUpdated(1);
}

void
ColorMapModel::
processData(const std::shared_ptr< CVImageData > &in, std::shared_ptr<CVImageData> & out,
            const ColorMapParameters & params )
{
    cv::Mat& in_image = in->image();
    if(!in_image.empty() && (in_image.type()==CV_8UC1 || in_image.type()==CV_8UC3))
    {
        cv::applyColorMap(in_image,out->image(),params.miColorMap);
    }
}

const QString ColorMapModel::_category = QString( "Image Analysis" );

const QString ColorMapModel::_model_name = QString( "Color Map" );
