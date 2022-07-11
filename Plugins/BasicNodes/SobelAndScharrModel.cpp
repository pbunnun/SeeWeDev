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

#include "SobelAndScharrModel.hpp"

#include <QtCore/QEvent> //???
#include <QtCore/QDir> //???
#include <QDebug> //for debugging using qDebug()

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

SobelAndScharrModel::
SobelAndScharrModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget(new SobelAndScharrEmbeddedWidget()),
      _minPixmap( ":SobelAndScharr.png" )
{
    for(std::shared_ptr<CVImageData>& mp : mapCVImageData)
    {
        mp = std::make_shared<CVImageData>(cv::Mat());
    }
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &SobelAndScharrEmbeddedWidget::checkbox_checked_signal, this, &SobelAndScharrModel::em_checkbox_checked);

    IntPropertyType intPropertyType;
    QString propId = "order_x";
    intPropertyType.miValue = mParams.miOrderX;
    auto propOrderX = std::make_shared< TypedProperty< IntPropertyType > >( "X order", propId, QVariant::Int, intPropertyType ,"Operation");
    mvProperty.push_back( propOrderX );
    mMapIdToProperty[ propId ] = propOrderX;

    intPropertyType.miValue = mParams.miOrderY;
    propId = "order_y";
    auto propOrderY = std::make_shared< TypedProperty< IntPropertyType > >( "Y order", propId, QVariant::Int, intPropertyType, "Operation" );
    mvProperty.push_back( propOrderY );
    mMapIdToProperty[ propId ] = propOrderY;

    intPropertyType.miValue = mParams.miKernelSize;
    propId = "kernel_size";
    auto propKernelSize = std::make_shared< TypedProperty< IntPropertyType > >( "Kernel Size", propId, QVariant::Int, intPropertyType ,"Operation");
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdScale;
    propId = "scale";
    auto propScale = std::make_shared< TypedProperty <DoublePropertyType>>("Scale", propId,QVariant::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propScale );
    mMapIdToProperty[ propId ] = propScale;

    doublePropertyType.mdValue = mParams.mdDelta;
    propId = "delta";
    auto propDelta = std::make_shared< TypedProperty <DoublePropertyType>>("Delta", propId,QVariant::Double, doublePropertyType , "Operation");
    mvProperty.push_back(propDelta);
    mMapIdToProperty[ propId ] = propDelta;

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList( {"DEFAULT", "CONSTANT", "REPLICATE", "REFLECT", "WRAP", "TRANSPARENT", "ISOLATED"} );
    enumPropertyType.miCurrentIndex = 0;
    propId = "border_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Border Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;

    propId = "checked";
    auto propChecked = std::make_shared<TypedProperty<bool>>("", propId, QVariant::Bool, mpEmbeddedWidget->checkbox_is_checked());
    mMapIdToProperty[ propId ] = propChecked;

    propId = "enabled";
    auto propEnabled = std::make_shared<TypedProperty<bool>>("", propId, QVariant::Bool, mpEmbeddedWidget->checkbox_is_enabled());
    mMapIdToProperty[ propId ] = propEnabled;
}

unsigned int
SobelAndScharrModel::
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
SobelAndScharrModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
SobelAndScharrModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        return mapCVImageData[I];
    }
    else
        return nullptr;
}

void
SobelAndScharrModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData(mpCVImageInData,mapCVImageData,mParams);
        }
    }
    updateAllOutputPorts();
}

QJsonObject
SobelAndScharrModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["orderX"] = mParams.miOrderX;
    cParams["orderY"] = mParams.miOrderY;
    cParams["kernelSize"] = mParams.miKernelSize;
    cParams["scale"] = mParams.mdScale;
    cParams["delta"] = mParams.mdDelta;
    cParams["borderType"] = mParams.miBorderType;
    cParams["checked"] = mpEmbeddedWidget->checkbox_is_checked();
    cParams["enabled"] = mpEmbeddedWidget->checkbox_is_enabled();
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
SobelAndScharrModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "orderX" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "order_x" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miOrderX = v.toInt();
        }
        v =  paramsObj[ "orderY" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "order_y" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miOrderY = v.toInt();
        }
        v =  paramsObj[ "kernelSize" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miKernelSize = v.toInt();
        }
        v = paramsObj[ "scale" ];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty[ "scale" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType >>(prop);
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdScale = v.toDouble();
        }
        v = paramsObj[ "delta" ];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty[ "delta" ];
            auto typedProp  = std::static_pointer_cast< TypedProperty< DoublePropertyType >>(prop);
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdDelta = v.toDouble();
        }
        v = paramsObj[ "borderType" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "border_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miBorderType = v.toInt();
        }
        v = paramsObj[ "checked" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "checked" ];
            auto typedProp = std::static_pointer_cast< TypedProperty <bool>>(prop);
            typedProp->getData() = v.toBool();

            mpEmbeddedWidget->change_check_checkbox(v.toBool()? Qt::Checked : Qt::Unchecked);
        }
        v = paramsObj[ "enabled" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "enabled" ];
            auto typedProp = std::static_pointer_cast< TypedProperty <bool>>(prop);
            typedProp->getData() = v.toBool();

            mpEmbeddedWidget->change_enable_checkbox(v.toBool());
        }
    }
}

void SobelAndScharrModel::em_checkbox_checked(int)
{
    if(mpCVImageInData)
    {
        processData(mpCVImageInData,mapCVImageData,mParams);
        updateAllOutputPorts();
    }
}

void
SobelAndScharrModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if(id=="order_x")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miOrderX = value.toInt();
    }
    else if(id=="order_y")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        mParams.miOrderY = value.toInt();
    }
    else if( id == "kernel_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        int kSize =  value.toInt();
        bool adjValue = false;
        if( kSize%2==0 )
        {
            if(kSize==typedProp->getData().miMax)
            {
                kSize -= 1;
            }
            else
            {
                kSize += 1;
            }
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
            if(kSize==3)
            {
                mpEmbeddedWidget->change_enable_checkbox(true);
            }
            else
            {
                mpEmbeddedWidget->change_check_checkbox(Qt::Unchecked);
                mpEmbeddedWidget->change_enable_checkbox(false);
            }
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = kSize;
            mParams.miKernelSize = kSize;
        }
    }
    else if( id == "scale" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdScale = value.toDouble();
    }
    else if( id == "delta" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();
        mParams.mdDelta = value.toDouble();
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
        case 4: // WRAP (BUG)
            mParams.miBorderType = cv::BORDER_WRAP;
            break;
        case 5: // TRANSPARENT (BUG)
            mParams.miBorderType = cv::BORDER_TRANSPARENT;
            break;
        case 6: // ISOLATED
            mParams.miBorderType = cv::BORDER_ISOLATED;
            break;
        }
    }

    if( mpCVImageInData )
    {
        processData(mpCVImageInData,mapCVImageData,mParams);
        updateAllOutputPorts();
    }
}

void SobelAndScharrModel::processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<CVImageData> (&out)[3],
                                      const SobelAndScharrParameters &params)
{
    cv::Mat& in_image = in->image();
    if(in_image.empty())
    {
        return;
    }
    cv::Mat Temp[2];
    if(mpEmbeddedWidget->checkbox_is_checked())
    {
        cv::Scharr(in_image,Temp[0],CV_16S,params.miOrderX,0,params.mdScale,params.mdDelta,params.miBorderType);
        cv::Scharr(in_image,Temp[1],CV_16S,0,params.miOrderY,params.mdScale,params.mdDelta,params.miBorderType);
    }
    else
    {
        cv::Sobel(in_image,Temp[0],CV_16S,params.miOrderX,0,params.miKernelSize,params.mdScale,params.mdDelta,params.miBorderType);
        cv::Sobel(in_image,Temp[1],CV_16S,0,params.miOrderY,params.miKernelSize,params.mdScale,params.mdDelta,params.miBorderType);
    }
    cv::convertScaleAbs(Temp[0],out[1]->image());
    cv::convertScaleAbs(Temp[1],out[2]->image());
    cv::addWeighted(out[1]->image(),0.5,out[2]->image(),0.5,0,out[0]->image());
}

const QString SobelAndScharrModel::_category = QString( "Image Processing" );

const QString SobelAndScharrModel::_model_name = QString( "Sobel and Scharr" );
