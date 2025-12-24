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

#include "CVMakeBorderModel.hpp"

#include <QDebug> //for debugging using qDebug()


#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVMakeBorderModel::_category = QString( "Image Transformation" );

const QString CVMakeBorderModel::_model_name = QString( "CV Make Border" );

CVMakeBorderModel::
CVMakeBorderModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":/MakeBorder.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miBorderTop;
    QString propId = "border_top";
    auto propBorderTop = std::make_shared< TypedProperty< IntPropertyType > >( "Top", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back( propBorderTop );
    mMapIdToProperty[ propId ] = propBorderTop;

    intPropertyType.miValue = mParams.miBorderBottom;
    propId = "border_bottom";
    auto propBorderBottom = std::make_shared< TypedProperty< IntPropertyType > >( "Bottom", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back( propBorderBottom );
    mMapIdToProperty[ propId ] = propBorderBottom;

    intPropertyType.miValue = mParams.miBorderBottom;
    propId = "border_left";
    auto propBorderLeft = std::make_shared< TypedProperty< IntPropertyType > >( "Left", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back( propBorderLeft );
    mMapIdToProperty[ propId ] = propBorderLeft;

    intPropertyType.miValue = mParams.miBorderRight;
    propId = "border_right";
    auto propBorderRight = std::make_shared< TypedProperty< IntPropertyType > >( "Right", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back( propBorderRight );
    mMapIdToProperty[ propId ] = propBorderRight;

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList( {"DEFAULT", "CONSTANT", "REPLICATE", "REFLECT", "WRAP"} );
    enumPropertyType.miCurrentIndex = 1;
    propId = "border_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Border Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;

    UcharPropertyType ucharPropertyType;
    ucharPropertyType.mucValue = mParams.mucBorderColor[0];
    propId = "border_color_b";
    auto propBorderColorB = std::make_shared< TypedProperty <UcharPropertyType>>("B value", propId, QMetaType::Int, ucharPropertyType, "Display");
    mvProperty.push_back( propBorderColorB );
    mMapIdToProperty[ propId ] = propBorderColorB;

    ucharPropertyType.mucValue = mParams.mucBorderColor[1];
    propId = "border_color_g";
    auto propBorderColorG = std::make_shared< TypedProperty <UcharPropertyType>>("G value", propId, QMetaType::Int, ucharPropertyType, "Display");
    mvProperty.push_back( propBorderColorG );
    mMapIdToProperty[ propId ] = propBorderColorG;

    ucharPropertyType.mucValue = mParams.mucBorderColor[2];
    propId = "border_color_r";
    auto propBorderColorR = std::make_shared< TypedProperty <UcharPropertyType>>("R value", propId, QMetaType::Int, ucharPropertyType, "Display");
    mvProperty.push_back( propBorderColorR );
    mMapIdToProperty[ propId ] = propBorderColorR;

    QString inputSize = QString("%1 px x %2 px").arg(mProps.mCVSizeInput.height).arg(mProps.mCVSizeInput.width);
    propId = "input_size";
    auto propInputSize = std::make_shared< TypedProperty <QString>>("Input Size", propId, QMetaType::QString, inputSize, "Properties");
    mvProperty.push_back( propInputSize );
    mMapIdToProperty[ propId ] = propInputSize;

    QString outputSize = QString("%1 px x %2 px").arg(mProps.mCVSizeOutput.height).arg(mProps.mCVSizeOutput.width);
    propId = "output_size";
    auto propOutputSize = std::make_shared< TypedProperty <QString>>("Output Size", propId, QMetaType::QString, outputSize, "Properties");
    mvProperty.push_back( propOutputSize );
    mMapIdToProperty[ propId ] = propOutputSize;
}

unsigned int
CVMakeBorderModel::
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
CVMakeBorderModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVMakeBorderModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
CVMakeBorderModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mpCVImageData, mParams, mProps );
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
CVMakeBorderModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["borderTop"] = mParams.miBorderTop;
    cParams["borderBottom"] = mParams.miBorderBottom;
    cParams["borderLeft"] = mParams.miBorderLeft;
    cParams["borderRight"] = mParams.miBorderRight;
    cParams["borderType"] = mParams.miBorderType;
    cParams["borderColorB"] = mParams.mucBorderColor[0];
    cParams["borderColorG"] = mParams.mucBorderColor[1];
    cParams["borderColorR"] = mParams.mucBorderColor[2];
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVMakeBorderModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "borderTop" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_top" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miBorderTop = v.toInt();
        }
        v = paramsObj[ "borderBottom" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_bottom" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miBorderBottom = v.toInt();
        }
        v = paramsObj[ "borderLeft" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_left" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miBorderLeft = v.toInt();
        }
        v = paramsObj[ "borderRight" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_right" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miBorderRight = v.toInt();
        }
        v = paramsObj[ "borderType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miBorderType = v.toInt();
        }
        v = paramsObj[ "borderColorB" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_color_b" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
            typedProp->getData().mucValue = v.toInt();

            mParams.mucBorderColor[0] = v.toInt();
        }
        v = paramsObj[ "borderColorG" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_color_g" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
            typedProp->getData().mucValue = v.toInt();

            mParams.mucBorderColor[1] = v.toInt();
        }
        v = paramsObj[ "borderColorR" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_color_r" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
            typedProp->getData().mucValue = v.toInt();

            mParams.mucBorderColor[2] = v.toInt();
        }
    }
}

void
CVMakeBorderModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "border_top" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miBorderTop = value.toInt();
    }
    else if( id == "border_bottom" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miBorderBottom = value.toInt();
    }
    else if( id == "border_left" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miBorderLeft = value.toInt();
    }
    else if( id == "border_right" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miBorderRight = value.toInt();
    }
    else if( id == "border_type" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        switch( value.toInt() )
        {
        case 0: // DEFAULT
            mParams.miBorderType = cv::BORDER_DEFAULT;
            break;
        case 1: // CONSTANT
            mParams.miBorderType = cv::BORDER_CONSTANT;
            break;
        case 2: // REPLICATE
            mParams.miBorderType = cv::BORDER_REPLICATE;
            break;
        case 3: // REFLECT
            mParams.miBorderType = cv::BORDER_REFLECT;
            break;
        case 4: // WRAP
            mParams.miBorderType = cv::BORDER_WRAP;
            break;
        }
    }
    else if( id == "border_color_b" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
        typedProp->getData().mucValue = value.toInt();

        mParams.mucBorderColor[0] = value.toInt();
    }
    else if( id == "border_color_g" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
        typedProp->getData().mucValue = value.toInt();

        mParams.mucBorderColor[1] = value.toInt();
    }
    else if( id == "border_color_r" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
        typedProp->getData().mucValue = value.toInt();

        mParams.mucBorderColor[2] = value.toInt();
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams, mProps );

        Q_EMIT dataUpdated(0);
    }
}

void
CVMakeBorderModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr<CVImageData> & out,
            const CVMakeBorderParameters & params, CVMakeBorderProperties &props )
{
    cv::Mat& in_image = in->data();
    if(in_image.empty())
    {
        return;
    }
    cv::Mat& out_image = out->data();

    cv::copyMakeBorder(in_image,
                       out_image,
                       params.miBorderTop,
                       params.miBorderBottom,
                       params.miBorderLeft,
                       params.miBorderRight,
                       params.miBorderType,
                       cv::Scalar(params.mucBorderColor[0],
                                  params.mucBorderColor[1],
                                  params.mucBorderColor[2]));

    props.mCVSizeInput.height = in_image.rows;
    props.mCVSizeInput.width = in_image.cols;
    props.mCVSizeOutput.height = out_image.rows;
    props.mCVSizeOutput.width = out_image.cols;

    auto prop = mMapIdToProperty["input_size"];
    auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedProp->getData() = QString("%1 px x %2 px").arg(props.mCVSizeInput.height).arg(props.mCVSizeInput.width);

    prop = mMapIdToProperty["output_size"];
    typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedProp->getData() = QString("%1 px x %2 px").arg(props.mCVSizeOutput.height).arg(props.mCVSizeOutput.width);
}


