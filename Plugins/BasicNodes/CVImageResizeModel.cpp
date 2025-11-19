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

#include "CVImageResizeModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>

const QString CVImageResizeModel::_category = QString( "Image Operation" );

const QString CVImageResizeModel::_model_name = QString( "CV Resize" );

CVImageResizeModel::
CVImageResizeModel()
    : PBNodeDelegateModel( _model_name )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    mpCVImageInData = std::make_shared< CVImageData >( cv::Mat() );
    mpCVImageOutData = std::make_shared< CVImageData >( cv::Mat() );

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = mSize.width;
    sizePropertyType.miHeight = mSize.height;
    QString propId = "size_id";
    auto propSize = std::make_shared< TypedProperty< SizePropertyType > >("Resize", propId, QMetaType::QSize, sizePropertyType);
    mvProperty.push_back( propSize );
    mMapIdToProperty[ propId ] = propSize;
}

unsigned int
CVImageResizeModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 1 );
    case PortType::Out:
        return( 1 );
    default:
        return( 0 );
    }
}

NodeDataType
CVImageResizeModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::Out && portIndex == 0 )
        return CVImageData().type();
    else if( portType == PortType::In && portIndex == 0 )
        return CVImageData().type();
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
CVImageResizeModel::
outData(PortIndex)
{
    if( isEnable() && mpCVImageOutData->data().data != nullptr )
        return mpCVImageOutData;
    else
        return nullptr;
}

void
CVImageResizeModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if( nodeData )
    {
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
        {
            mpCVImageInData = d;
            processData(mpCVImageInData, mpCVImageOutData);
            Q_EMIT dataUpdated( 0 );
        }
    }
}

QJsonObject
CVImageResizeModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDelegateModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "width" ] = mSize.width;
    cParams[ "height" ] = mSize.height;

    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
CVImageResizeModel::
load(const QJsonObject &p)
{
    /*
     * If load() was overridden, PBNodeDelegateModel::load() must be called explicitely.
     */
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue width = paramsObj[ "width" ];
        QJsonValue height = paramsObj[ "height" ];
        if( !width.isNull() && !height.isNull() )
        {
            auto prop = mMapIdToProperty[ "size_id" ];
            /* Restore internal property */
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = width.toInt();
            typedProp->getData().miHeight = height.toInt();
            mSize = cv::Size(width.toInt(), height.toInt());
        }
    }
}

void
CVImageResizeModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "size_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
        auto size = value.toSize();

        typedProp->getData().miWidth = size.width();
        typedProp->getData().miHeight = size.height();

        mSize = cv::Size( size.width(), size.height() );

        processData(mpCVImageInData, mpCVImageOutData);
        Q_EMIT dataUpdated(0);
    }
}

void
CVImageResizeModel::
processData(const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out )
{
    if( !in->data().empty() )
    {
        cv::Mat resizeImage;
        auto image = in->data();
        auto new_size = mSize;
        cv::resize(image, resizeImage, new_size, cv::INTER_LINEAR);
        //cv::resize(image, resizeImage, new_size, cv::INTER_CUBIC);
        out->set_image( resizeImage );
    }
}


