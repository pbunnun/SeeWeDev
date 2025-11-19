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

#include "CVColorSpaceModel.hpp"

#include <QDebug>


#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVColorSpaceModel::_category = QString( "Image Conversion" );

const QString CVColorSpaceModel::_model_name = QString( "CV Color Space" );

CVColorSpaceModel::

CVColorSpaceModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":CVColorSpace.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList( {"GRAY", "BGR", "RGB", "HSV"} );
    enumPropertyType.miCurrentIndex = 1;
    QString propId = "color_space_input";
    auto propCVColorSpaceInput = std::make_shared< TypedProperty< EnumPropertyType > >( "Input Color Space", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation" );
    mvProperty.push_back( propCVColorSpaceInput );
    mMapIdToProperty[ propId ] = propCVColorSpaceInput;

    enumPropertyType.mslEnumNames = QStringList( {"GRAY", "BGR", "RGB", "HSV"} );
    enumPropertyType.miCurrentIndex = 2;
    propId = "color_space_output";
    auto propCVColorSpaceOutput = std::make_shared< TypedProperty< EnumPropertyType > >( "Output Color Space", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation" );
    mvProperty.push_back( propCVColorSpaceOutput );
    mMapIdToProperty[ propId ] = propCVColorSpaceOutput;
}

unsigned int
CVColorSpaceModel::

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
CVColorSpaceModel::

dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVColorSpaceModel::

outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
CVColorSpaceModel::

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
CVColorSpaceModel::

save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["colorSpaceInput"] = mParams.miCVColorSpaceInput;
    cParams["colorSpaceOutput"] = mParams.miCVColorSpaceOutput;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVColorSpaceModel::

load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v =  paramsObj[ "colorSpaceInput" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "color_space_input" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miCVColorSpaceInput = v.toInt();
        }
        v = paramsObj[ "colorSpaceOutput" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "color_space_output" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miCVColorSpaceOutput = v.toInt();
        }
    }
}

void
CVColorSpaceModel::

setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "color_space_input" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        mParams.miCVColorSpaceInput = value.toInt();
    }
    else if( id == "color_space_output" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        mParams.miCVColorSpaceOutput = value.toInt();
    }
    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }
}

void
CVColorSpaceModel::

processData( const std::shared_ptr< CVImageData > & in, std::shared_ptr< CVImageData > & out,
             const CVColorSpaceParameters & params)
{
    cv::Mat& in_image = in->data();
    if(in_image.empty() || in_image.depth()!=CV_8U)
    {
        return;
    }
    else if( params.miCVColorSpaceInput == params.miCVColorSpaceOutput )
    {
        out->set_image(in_image);
    }
    else
    {
        int cvColorSpaceConvertion = -1;
        switch( params.miCVColorSpaceInput )
        {
        case 0 :
            if(in_image.channels()!=1)
            {
                return;
            }
            switch( params.miCVColorSpaceOutput )
            {
            case 1 :
                cvColorSpaceConvertion = cv::COLOR_GRAY2BGR;
                break;

            case 2 :
                cvColorSpaceConvertion = cv::COLOR_GRAY2RGB;
                break;
            }
            break;

        case 1 :
            if(in_image.channels()!=3)
            {
                return;
            }
            switch( params.miCVColorSpaceOutput )
            {
            case 0 :
                cvColorSpaceConvertion = cv::COLOR_BGR2GRAY;
                break;

            case 2 :
                cvColorSpaceConvertion = cv::COLOR_BGR2RGB;
                break;

            case 3 :
                cvColorSpaceConvertion = cv::COLOR_BGR2HSV;
                break;
            }
            break;

        case 2:
            if(in_image.channels()!=3)
            {
                return;
            }
            switch( params.miCVColorSpaceOutput )
            {
            case 0 :
                cvColorSpaceConvertion = cv::COLOR_RGB2GRAY;
                break;

            case 1 :
                cvColorSpaceConvertion = cv::COLOR_RGB2BGR;
                break;

            case 3 :
                cvColorSpaceConvertion = cv::COLOR_RGB2HSV;
                break;
            }
            break;

        case 3 :
            if(in_image.channels()!=3)
            {
                return;
            }
            switch( params.miCVColorSpaceOutput )
            {
            case 1 :
                cvColorSpaceConvertion = cv::COLOR_HSV2BGR;
                break;

            case 2 :
                cvColorSpaceConvertion = cv::COLOR_HSV2RGB;
                break;
            }
        }
        if(cvColorSpaceConvertion != -1)
        {
            cv::cvtColor( in->data(), out->data() , cvColorSpaceConvertion );
        }
        else
        {
            out->set_image(in_image);
        }
    }
}


