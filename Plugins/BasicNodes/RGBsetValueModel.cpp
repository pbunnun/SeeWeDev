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

#include "RGBsetValueModel.hpp" //INCOMPLETE

#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>

RGBsetValueModel::RGBsetValueModel()
    :PBNodeDataModel( _model_name ),
     mpEmbeddedWidget(new RGBsetValueEmbeddedWidget()),
     _minPixmap(":RGBsetValue.png")
{
    mpCVImageData = std::make_shared<CVImageData>( cv::Mat() );

    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &RGBsetValueEmbeddedWidget::button_clicked_signal, this, &RGBsetValueModel::em_button_clicked );

    UcharPropertyType ucharPropertyType;
    QString propId = "r_value";
    ucharPropertyType.mucValue = mParams.mucRvalue;
    auto propRvalue = std::make_shared<TypedProperty<UcharPropertyType>>("R Value",propId,QMetaType::Int, ucharPropertyType, "Operation");
    mvProperty.push_back(propRvalue);
    mMapIdToProperty[propId] = propRvalue;

    propId = "g_value";
    ucharPropertyType.mucValue = mParams.mucGvalue;
    auto propGvalue = std::make_shared<TypedProperty<UcharPropertyType>>("G Value",propId,QMetaType::Int, ucharPropertyType, "Operation");
    mvProperty.push_back(propGvalue);
    mMapIdToProperty[propId] = propGvalue;

    propId = "b_value";
    ucharPropertyType.mucValue = mParams.mucBvalue;
    auto propBvalue = std::make_shared<TypedProperty<UcharPropertyType>>("B Value",propId,QMetaType::Int,ucharPropertyType, "Operation");
    mvProperty.push_back(propBvalue);
    mMapIdToProperty[propId] = propBvalue;
}

unsigned int RGBsetValueModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch(portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result =1;
        break;
    default:
        break;
    }
    return result;
}

NodeDataType RGBsetValueModel::dataType(PortType,PortIndex) const
{
    return CVImageData().type();
}

std::shared_ptr<NodeData> RGBsetValueModel::outData(PortIndex)
{
    if( isEnable() )
    {
        return mpCVImageData;
    }
    else
    {
        return nullptr;
    }
}

void RGBsetValueModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if(nodeData)
    {
        auto d= std::dynamic_pointer_cast<CVImageData>(nodeData);
        if(d) //no image modification when initially setting in the data
        {
            mpCVImageInData = d;
            mpCVImageData->set_image(mpCVImageInData->data());
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject RGBsetValueModel::save() const
{
    QJsonObject modelJson =PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["R value"] = mParams.mucRvalue;
    cParams["G value"] = mParams.mucGvalue;
    cParams["B value"] = mParams.mucBvalue;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void RGBsetValueModel::restore(QJsonObject const &p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p["cParams"].toObject();
    if(!paramsObj.isEmpty())
    {
        QJsonValue v =paramsObj["R value"];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty["r_value"];
            auto typedProp =std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typedProp->getData().mucValue = v.toInt();
            mParams.mucRvalue = v.toInt();
        }
        v =paramsObj["G value"];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty["g_value"];
            auto typedProp =std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typedProp->getData().mucValue = v.toInt();
            mParams.mucGvalue = v.toInt();
        }
        v =paramsObj["B value"];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty["b_value"];
            auto typedProp =std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typedProp->getData().mucValue = v.toInt();
            mParams.mucBvalue = v.toInt();
        }
    }
}

void
RGBsetValueModel::
em_button_clicked( int )
{
   if(mpCVImageInData)
   {
       mpCVImageData->set_image(mpCVImageInData->data());
       Q_EMIT dataUpdated( 0 );
   }
}

void RGBsetValueModel::setModelProperty(QString &id, const QVariant & value)
{
    PBNodeDataModel::setModelProperty(id, value);
    if(!mMapIdToProperty.contains(id))
    {
        return;
    }
    auto prop = mMapIdToProperty[id];
    if(id=="r_value")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        typedProp->getData().mucValue = value.toInt();
        mParams.mucRvalue = value.toInt();
    }
    else if(id=="g_value")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        typedProp->getData().mucValue = value.toInt();
        mParams.mucGvalue = value.toInt();
    }
    else if(id=="b_value")
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        typedProp->getData().mucValue = value.toInt();
        mParams.mucBvalue = value.toInt();
    }

    if(mpCVImageData)
    {
        if(id=="r_value")
        {
            mProps.miChannel = 2;
            mProps.mucValue = mParams.mucRvalue;
        }
        else if(id=="g_value")
        {
            mProps.miChannel = 1;
            mProps.mucValue = mParams.mucGvalue;
        }
        else if(id=="b_value")
        {
            mProps.miChannel = 0;
            mProps.mucValue = mParams.mucBvalue;
        }
        processData(mpCVImageData,mProps);

        Q_EMIT dataUpdated(0);
    }
}

void RGBsetValueModel::processData(std::shared_ptr<CVImageData> &out, const RGBsetValueProperties &props)
{
    cv::Mat& out_image = out->data();
    if(out_image.empty() || out_image.type()!=CV_8UC3)
    {
        return;
    }
    for(int i=0; i<out_image.rows; i++)
    {
        for(int j=0; j<out_image.cols; j++)
        {
            out_image.at<cv::Vec3b>(i,j)[props.miChannel] = cv::saturate_cast<uchar>(props.mucValue);
        }
    }
}

const QString RGBsetValueModel::_category = QString("Image Conversion");

const QString RGBsetValueModel::_model_name = QString("RGB Values");
