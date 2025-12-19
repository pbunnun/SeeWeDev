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

#include "CVConvertDepthModel.hpp"

#include <QDebug> //for debugging using qDebug()


#include <opencv2/highgui.hpp>
#include "qtvariantproperty_p.h"

const QString CVConvertDepthModel::_category = QString( "Image Conversion" );

const QString CVConvertDepthModel::_model_name = QString( "CV Convert Depth" );

CVConvertDepthModel::
CVConvertDepthModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":/CVConvertDepthModel.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"CV_8U", "CV_8S", "CV_16U", "CV_16S" ,"CV_32S", "CV_32F", "CV_64F", "CV_16F"});
    QString propId = "image_depth";
    auto propImageDepth = std::make_shared< TypedProperty< EnumPropertyType > >( "Image Depth", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propImageDepth );
    mMapIdToProperty[ propId ] = propImageDepth;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdAlpha;
    propId = "alpha";
    auto propAlpha = std::make_shared< TypedProperty< DoublePropertyType > >( "Alpha", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propAlpha );
    mMapIdToProperty[ propId ] = propAlpha;

    doublePropertyType.mdValue = mParams.mdBeta;
    propId = "beta";
    auto propBeta = std::make_shared< TypedProperty< DoublePropertyType > >( "Beta", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propBeta );
    mMapIdToProperty[ propId ] = propBeta;
}

unsigned int
CVConvertDepthModel::
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
CVConvertDepthModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if(portType == PortType::In && portIndex == 1)
        return IntegerData().type();
    else
        return CVImageData().type();
}


std::shared_ptr<NodeData>
CVConvertDepthModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
CVConvertDepthModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if(nodeData)
    {
        if(portIndex == 0)
        {
            auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
            if(d)
            {
                mpCVImageInData = d;
            }
        }
        else if(portIndex ==1)
        {
            auto d = std::dynamic_pointer_cast<IntegerData>(nodeData);
            if(d)
            {
                mpIntegerInData = d;
            }
        }
        if(mpCVImageData)
        {
            if(mpIntegerInData)
            {
                overwrite(mpIntegerInData,mParams);
            }
            processData(mpCVImageInData,mpCVImageData,mParams);
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
CVConvertDepthModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["imageDepth"] = mParams.miImageDepth;
    cParams["alpha"] = mParams.mdAlpha;
    cParams["beta"] = mParams.mdBeta;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVConvertDepthModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "imageDepth" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "image_depth" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miImageDepth = v.toInt();
        }
        v = paramsObj[ "alpha" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "alpha" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdAlpha = v.toDouble();
        }
        v = paramsObj[ "beta" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "beta" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdBeta = v.toDouble();
        }
    }
}

void
CVConvertDepthModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "image_depth" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        mParams.miImageDepth = value.toInt();
        // In this case, the value nicely corresponds with the index.
    }
    else if( id == "alpha" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdAlpha = value.toDouble();
    }
    else if( id == "beta" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdBeta = value.toDouble();
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }
}

void
CVConvertDepthModel::
processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr<CVImageData> & out,
             const CVConvertDepthParameters & params )
{
    if(!in->data().empty())
    {
        in->data().convertTo(out->data(),
                               params.miImageDepth,
                               params.mdAlpha,
                               params.mdBeta);
    }
}

void
CVConvertDepthModel::
overwrite(std::shared_ptr<IntegerData> &in, CVConvertDepthParameters &params)
{
    int& in_number = in->data();
    if(in_number>=0 && in_number<=7)
    {
        auto prop = mMapIdToProperty["image_depth"];
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = in_number;
        params.miImageDepth = in_number;
    }
    in.reset();
}


