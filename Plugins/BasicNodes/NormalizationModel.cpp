#include "NormalizationModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include "qtvariantproperty.h"

NormalizationModel::
NormalizationModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":Normalization.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdRangeMax;
    doublePropertyType.mdMax = 255;
    QString propId = "range_max";
    auto propRangeMax = std::make_shared< TypedProperty< DoublePropertyType > >( "Maximum", propId, QVariant::Double, doublePropertyType, "Operation" );
    mvProperty.push_back( propRangeMax );
    mMapIdToProperty[ propId ] = propRangeMax;

    doublePropertyType.mdValue = mParams.mdRangeMin;
    doublePropertyType.mdMax = 255;
    propId = "range_min";
    auto propRangeMin = std::make_shared< TypedProperty< DoublePropertyType > >( "Minimum", propId, QVariant::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propRangeMin );
    mMapIdToProperty[ propId ] = propRangeMin;

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"NORM_L1", "NORM_L2", "NORM_INF", "NORM_L2SQR", "NORM_MINMAX", "NORM_HAMMING", "NORM_HAMMING2", "NORM_RELATIVE", "NORM_TYPE_MASK"});
    enumPropertyType.miCurrentIndex = 4;
    propId = "norm_type";
    auto propThresholdType = std::make_shared< TypedProperty< EnumPropertyType > >( "Norm Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propThresholdType );
    mMapIdToProperty[ propId ] = propThresholdType;
}

unsigned int
NormalizationModel::
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
NormalizationModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
    {
        return CVImageData().type();
    }
    return DoubleData().type();
}


std::shared_ptr<NodeData>
NormalizationModel::
outData(PortIndex)
{
    if( isEnable() )
    {
        return mpCVImageData;
    }
    return nullptr;
}

void
NormalizationModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        if(portIndex == 0)
        {
            auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
            if (d)
            {
                mpCVImageInData = d;
                processData(mpCVImageInData,mpCVImageData,mParams);
            }
        }
        else
        {
            auto d= std::dynamic_pointer_cast<DoubleData>(nodeData);
            if (d)
            {
                mapDoubleInData[portIndex-1] = d;
                if(mapDoubleInData[0] || mapDoubleInData[1])
                {
                    overwrite(mapDoubleInData,mParams);
                }
                processData(mpCVImageInData,mpCVImageData,mParams);
            }
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
NormalizationModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["rangeMax"] = mParams.mdRangeMax;
    cParams["rangeMin"] = mParams.mdRangeMin;
    cParams["normType"] = mParams.miNormType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
NormalizationModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "rangeMax" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "range_max" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdRangeMax= v.toDouble();
        }
        v =  paramsObj[ "rangeMin" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "range_min" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdRangeMin = v.toDouble();
        }
        v =  paramsObj[ "normType" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "norm_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miNormType = v.toInt();
        }
    }
}

void
NormalizationModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "range_max" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdRangeMax = value.toDouble();
    }
    else if( id == "range_min" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdRangeMin = value.toDouble();
    }
    else if( id == "norm_type" ) //{"NORM_L1", "NORM_L2", "NORM_INF", "NORM_L2SQR", "NORM_MINMAX", "NORM_HAMMING", "NORM_HAMMING2", "NORM_RELATIVE", "NORM_TYPE_MASK"}
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt()) //Only NORM_MINMAX is currently functional
        {
        case 0:
            mParams.miNormType = cv::NORM_L1;
            break;

        case 1:
            mParams.miNormType = cv::NORM_L2;
            break;

        case 2:
            mParams.miNormType = cv::NORM_INF;
            break;

        case 3:
            mParams.miNormType = cv::NORM_L2SQR;
            break;

        case 4:
            mParams.miNormType = cv::NORM_MINMAX;
            break;

        case 5:
            mParams.miNormType = cv::NORM_HAMMING;
            break;

        case 6:
            mParams.miNormType = cv::NORM_HAMMING2;
            break;

        case 7:
            mParams.miNormType = cv::NORM_RELATIVE;
            break;

        case 8:
            mParams.miNormType = cv::NORM_TYPE_MASK;
            break;
        }
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams);
        updateAllOutputPorts();
    }
}

void
NormalizationModel::
processData(const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out,
            const NormalizationParameters & params)
{
    if(!in->image().empty())
    {
        cv::normalize(in->image(),
                      out->image(),
                      params.mdRangeMin,
                      params.mdRangeMax,
                      params.miNormType);
    }
}

void
NormalizationModel::
overwrite(std::shared_ptr<DoubleData> (&in)[2], NormalizationParameters &params)
{
    if(in[0])
    {
        const double& in_number = in[0]->number();
        if(in_number>=0 && in_number<=255)
        {
            auto prop = mMapIdToProperty["range_max"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = in_number;
            params.mdRangeMax = in_number;
            in[0].reset();
        }
    }
    if(in[1])
    {
        const double& in_number = in[1]->number();
        if(in_number>=0 && in_number<=255)
        {
            auto prop = mMapIdToProperty["range_min"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = in_number;
            params.mdRangeMin = in_number;
            in[1].reset();
        }
    }
}

const QString NormalizationModel::_category = QString( "Image Conversion" );

const QString NormalizationModel::_model_name = QString( "Normalization" );
