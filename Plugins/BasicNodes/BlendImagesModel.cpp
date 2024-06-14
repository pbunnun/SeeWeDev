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

#include "BlendImagesModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

BlendImagesModel::
BlendImagesModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget(new BlendImagesEmbeddedWidget),
      _minPixmap( ":BlendImages.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &BlendImagesEmbeddedWidget::radioButton_clicked_signal, this, &BlendImagesModel::em_radioButton_clicked );

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdAlpha;
    doublePropertyType.mdMax = 1.0;
    QString propId = "alpha";
    auto propAlpha = std::make_shared< TypedProperty< DoublePropertyType > >( "Alpha", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propAlpha );
    mMapIdToProperty[ propId ] = propAlpha;

    doublePropertyType.mdValue = mParams.mdBeta;
    doublePropertyType.mdMax = 1.0;
    propId = "beta";
    auto propBeta = std::make_shared< TypedProperty< DoublePropertyType > >( "Beta", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propBeta );
    mMapIdToProperty[ propId ] = propBeta;

    doublePropertyType.mdValue = mParams.mdGamma;
    doublePropertyType.mdMax = 100;
    propId = "gamma";
    auto propGamma = std::make_shared< TypedProperty< DoublePropertyType > >( "Gamma", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propGamma );
    mMapIdToProperty[ propId ] = propGamma;

    propId = "size_from_port0";
    auto propSizeFromPort0 = std::make_shared< TypedProperty < bool > >("Size From Port 0", propId, QMetaType::Bool, mParams.mbSizeFromPort0, "Display");
    mvProperty.push_back( propSizeFromPort0 );
    mMapIdToProperty[ propId ] = propSizeFromPort0;

    IntPropertyType intPropertyType;
    mpEmbeddedWidget->setCurrentState(1);
    intPropertyType.miValue = mpEmbeddedWidget->getCurrentState();
    propId = "operation";
    auto propOperation = std::make_shared<TypedProperty<IntPropertyType>>("", propId, QMetaType::Int, intPropertyType);
    mMapIdToProperty[ propId ] = propOperation;
}

unsigned int
BlendImagesModel::
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
BlendImagesModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
BlendImagesModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
BlendImagesModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mapCVImageInData[portIndex] = d;
            if(allports_are_active(mapCVImageInData))
            {
                processData(mapCVImageInData, mpCVImageData, mParams);
            }
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
BlendImagesModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["alpha"] = mParams.mdAlpha;
    cParams["beta"] = mParams.mdBeta;
    cParams["gamma"] = mParams.mdGamma;
    cParams["sizeFromPort0"] = mParams.mbSizeFromPort0;
    cParams["operation"] = mpEmbeddedWidget->getCurrentState();
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
BlendImagesModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "alpha" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "alpha" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdAlpha = v.toDouble();
        }
        v = paramsObj[ "beta" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "beta" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdBeta = v.toDouble();
        }
        v = paramsObj[ "gamma" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "gamma" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdGamma = v.toDouble();
        }
        v = paramsObj[ "sizeFromPort0" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "size_from_port0" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < bool > >(prop);
            typedProp->getData() = v.toBool();

            mParams.mbSizeFromPort0 = v.toBool();
        }
        v = paramsObj["operation"];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "operation" ];
            auto typedProp = std::static_pointer_cast<TypedProperty <IntPropertyType> >(prop);
            typedProp->getData().miValue = v.toInt();
            mpEmbeddedWidget->setCurrentState(v.toInt());
        }
    }
}

void
BlendImagesModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "alpha" )
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
    else if( id == "gamma" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdGamma = value.toDouble();
    }
    else if( id == "size_from_port0" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >(prop);
        typedProp->getData() = value.toBool();

        mParams.mbSizeFromPort0 = value.toBool();
    }

    if(allports_are_active(mapCVImageInData))
    {
        processData( mapCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }
}

void BlendImagesModel::em_radioButton_clicked()
{
    if(allports_are_active(mapCVImageInData))
    {
        processData(mapCVImageInData,mpCVImageData,mParams);
        Q_EMIT dataUpdated(0);
    }
}

void
BlendImagesModel::
processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr<CVImageData> & out,
            const BlendImagesParameters & params)
{
    cv::Mat& i0 = in[0]->data();
    cv::Mat& i1 = in[1]->data();
    if(i0.empty() || i1.empty())
    {
        return;
    }
    if(i0.type()==i1.type())
    {
        cv::Mat Temp;
        if(params.mbSizeFromPort0)
        {
            if(i0.size!=i1.size)
            {
                cv::resize(i1,Temp,cv::Size(i0.cols,i0.rows));
            }
            switch(mpEmbeddedWidget->getCurrentState())
            {
            case 0:
                cv::add(i0,Temp,out->data());
                break;

            case 1:
                cv::addWeighted(i0,params.mdAlpha,Temp,params.mdBeta,params.mdGamma,out->data(),-1);
                break;
            }
        }
        else
        {
            if(i0.size!=i1.size)
            {
                cv::resize(i0,Temp,cv::Size(i1.cols,i1.rows));
            }
            switch(mpEmbeddedWidget->getCurrentState())
            {
            case 0:
                cv::add(Temp,i1,out->data());
                break;

            case 1:
                cv::addWeighted(Temp,params.mdAlpha,i1,params.mdBeta,params.mdGamma,out->data(),-1);
                break;
            }
        }
    }
}

bool BlendImagesModel::allports_are_active(const std::shared_ptr<CVImageData> (&ap)[2]) const
{
    for(const std::shared_ptr<CVImageData> &p : ap)
    {
        if(p==nullptr)
        {
            return false;
        }
    }
    return true;
}

const QString BlendImagesModel::_category = QString( "Image Operation" );

const QString BlendImagesModel::_model_name = QString( "Blend Images" );
