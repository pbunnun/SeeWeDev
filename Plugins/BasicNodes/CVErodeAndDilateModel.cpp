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

#include "CVErodeAndDilateModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>


#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVErodeAndDilateModel::_category = QString( "Image Modification" );

const QString CVErodeAndDilateModel::_model_name = QString( "CV Erode and Dilate" );

CVErodeAndDilateModel::
CVErodeAndDilateModel()
    : PBNodeDelegateModel( _model_name ),
      mpEmbeddedWidget(new CVErodeAndDilateEmbeddedWidget),
      _minPixmap( ":CVErodeAndDilate.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());

    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &CVErodeAndDilateEmbeddedWidget::radioButton_clicked_signal, this, &CVErodeAndDilateModel::em_radioButton_clicked );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList( {"MORPH_RECT", "MORPH_CROSS", "MORTH_ELLIPSE"} );
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "kernel_shape";
    auto propKernelShape = std::make_shared< TypedProperty< EnumPropertyType > >( "Kernel Shape", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation" );
    mvProperty.push_back( propKernelShape );
    mMapIdToProperty[ propId ] = propKernelShape;

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = mParams.mCVSizeKernel.width;
    sizePropertyType.miHeight = mParams.mCVSizeKernel.height;
    propId = "kernel_size";
    auto propKernelSize = std::make_shared< TypedProperty< SizePropertyType > >( "Kernel Size", propId, QMetaType::QSize, sizePropertyType, "Operation" );
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    PointPropertyType pointPropertyType;
    pointPropertyType.miXPosition = mParams.mCVPointAnchor.x;
    pointPropertyType.miYPosition = mParams.mCVPointAnchor.y;
    propId = "anchor_point";
    auto propAnchorPoint = std::make_shared< TypedProperty< PointPropertyType > >( "Anchor Point", propId, QMetaType::QPoint, pointPropertyType ,"Operation");
    mvProperty.push_back( propAnchorPoint );
    mMapIdToProperty[ propId ] = propAnchorPoint;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miIterations;
    propId = "iterations";
    auto propIterations = std::make_shared<TypedProperty<IntPropertyType>>("Iterations", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back( propIterations );
    mMapIdToProperty[ propId ] = propIterations;

    enumPropertyType.mslEnumNames = QStringList( {"DEFAULT", "CONSTANT", "REPLICATE", "REFLECT", "WRAP", "TRANSPARENT", "ISOLATED"} );
    enumPropertyType.miCurrentIndex = 0;
    propId = "border_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Border Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;

    mpEmbeddedWidget->setCurrentState(0);
    intPropertyType.miValue = mpEmbeddedWidget->getCurrentState();
    propId = "operation";
    auto propOperation = std::make_shared< TypedProperty< IntPropertyType > >( "Operation", propId, QMetaType::Int, intPropertyType );
    mMapIdToProperty[ propId ] = propOperation;
}

unsigned int
CVErodeAndDilateModel::
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
CVErodeAndDilateModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVErodeAndDilateModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
CVErodeAndDilateModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData(mpCVImageInData,mpCVImageData,mParams);
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
CVErodeAndDilateModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["kernelShape"] = mParams.miKernelShape;
    cParams["kernelWidth"] = mParams.mCVSizeKernel.width;
    cParams["kernelHeight"] = mParams.mCVSizeKernel.height;
    cParams["anchorX"] = mParams.mCVPointAnchor.x;
    cParams["anchorY"] = mParams.mCVPointAnchor.y;
    cParams["iterations"] = mParams.miIterations;
    cParams["borderType"] = mParams.miBorderType;
    cParams["operation"] = mpEmbeddedWidget->getCurrentState();
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVErodeAndDilateModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "kernelShape" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "kernel_shape" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miKernelShape = v.toInt();
        }

        QJsonValue argX = paramsObj[ "kernelWidth" ];
        QJsonValue argY = paramsObj[ "kernelHeight" ];
        if( !argX.isNull() && !argY.isNull() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = argX.toInt();
            typedProp->getData().miHeight = argY.toInt();

            mParams.mCVSizeKernel = cv::Size( argX.toInt(), argY.toInt() );
        }
        argX = paramsObj[ "anchorX" ];
        argY = paramsObj[ "anchorY" ];
        if( !argX.isNull() && ! argY.isNull() )
        {
            auto prop = mMapIdToProperty[ "anchor_point" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = argX.toInt();
            typedProp->getData().miYPosition = argY.toInt();

            mParams.mCVPointAnchor = cv::Point(argX.toInt(),argY.toInt());
        }
        v = paramsObj[ "iterations" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "iterations" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();
            mParams.miIterations = v.toInt();
        }
        v = paramsObj[ "borderType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miBorderType = v.toInt();
        }
        v = paramsObj[ "operation" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "operation" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            mpEmbeddedWidget->setCurrentState(v.toInt());
        }
    }
}

void
CVErodeAndDilateModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "kernel_shape" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <EnumPropertyType > >(prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt())
        {
        case 0:
            mParams.miKernelShape = cv::MORPH_RECT;
            break;

        case 1:
            mParams.miKernelShape = cv::MORPH_CROSS;
            break;

        case 2:
            mParams.miKernelShape = cv::MORPH_ELLIPSE;
            break;
        }
    }
    else if( id == "kernel_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
        QSize kSize =  value.toSize();
        bool adjValue = false;
        if( kSize.width()%2 != 1 )
        {
            kSize.setWidth( kSize.width() + 1 );
            adjValue = true;
        }
        if( kSize.height()%2 != 1 )
        {
            kSize.setHeight( kSize.height() + 1 );
            adjValue = true;
        }
        if( adjValue )
        {
            typedProp->getData().miWidth = kSize.width();
            typedProp->getData().miHeight = kSize.height();

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = kSize.width();
            typedProp->getData().miHeight = kSize.height();

            mParams.mCVSizeKernel = cv::Size( kSize.width(), kSize.height() );
        }
    }
    else if( id == "anchor_point" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint aPoint =  value.toPoint();
        bool adjValue = false;
        if( aPoint.x() > (mParams.mCVSizeKernel.width+1)/2 ) //Size members are gauranteed to be odd numbers.
        {
            aPoint.setX((mParams.mCVSizeKernel.width+1)/2);
            adjValue = true;
        }
        else if( aPoint.x() < -1)
        {
            aPoint.setX(-1);
            adjValue = true;
        }
        if( aPoint.y() > (mParams.mCVSizeKernel.height+1)/2 )
        {
            aPoint.setY((mParams.mCVSizeKernel.height-1)/2);
            adjValue = true;
        }
        else if( aPoint.y() < -1)
        {
            aPoint.setY(-1);
            adjValue = true;
        }
        if( adjValue )
        {
            typedProp->getData().miXPosition = aPoint.x();
            typedProp->getData().miYPosition = aPoint.y();

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = aPoint.x();
            typedProp->getData().miYPosition = aPoint.y();

            mParams.mCVPointAnchor = cv::Point( aPoint.x(), aPoint.y() );
        }
    }
    else if( id == "iterations" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miIterations = value.toInt();
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
        case 5: // TRANSPARENT
            mParams.miBorderType = cv::BORDER_TRANSPARENT;
            break;
        case 6: // ISOLATED
            mParams.miBorderType = cv::BORDER_ISOLATED;
            break;
        }
    }
    if( mpCVImageInData )
    {
        processData(mpCVImageInData,mpCVImageData,mParams);

        Q_EMIT dataUpdated(0);
    }
}

void CVErodeAndDilateModel::em_radioButton_clicked()
{
    if( mpCVImageInData )
    {
        processData(mpCVImageInData,mpCVImageData,mParams);

        Q_EMIT dataUpdated(0);
    }
}

void CVErodeAndDilateModel::processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<CVImageData> &out, const CVErodeAndDilateParameters &params)
{
    cv::Mat& in_image = in->data();
    if(in_image.empty() || (in_image.depth()!=CV_8U && in_image.depth()!=CV_16U && in_image.depth()!=CV_16S && in_image.depth()!=CV_32F && in_image.depth()!=CV_64F))
    {
        return;
    }
    cv::Mat Kernel = cv::getStructuringElement(params.miKernelShape,params.mCVSizeKernel,params.mCVPointAnchor);
    switch(mpEmbeddedWidget->getCurrentState())
    {
    case 0:
        cv::erode(in_image,
                  out->data(),
                  Kernel,
                  params.mCVPointAnchor,
                  params.miIterations,
                  params.miBorderType);
        break;

    case 1:
        cv::dilate(in_image,
                   out->data(),
                   Kernel,
                   params.mCVPointAnchor,
                   params.miIterations,
                   params.miBorderType);
        break;
    }
}


