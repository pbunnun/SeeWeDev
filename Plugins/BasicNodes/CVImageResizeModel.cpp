#include "CVImageResizeModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>
#include "qtvariantproperty.h"

CVImageResizeModel::
CVImageResizeModel()
    : PBNodeDataModel( _model_name )
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
    if( isEnable() && mpCVImageOutData->image().data != nullptr )
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
     * If save() was overrided, PBNodeDataModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams[ "width" ] = mSize.width;
    cParams[ "height" ] = mSize.height;

    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
CVImageResizeModel::
restore(const QJsonObject &p)
{
    /*
     * If restore() was overrided, PBNodeDataModel::restore() must be called explicitely.
     */
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue width = paramsObj[ "width" ];
        QJsonValue height = paramsObj[ "height" ];
        if( !width.isUndefined() && !height.isUndefined() )
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
    PBNodeDataModel::setModelProperty( id, value );

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
    if( !in->image().empty() )
    {
        cv::Mat resizeImage;
        auto image = in->image();
        auto new_size = mSize;
        cv::resize(image, resizeImage, new_size, cv::INTER_LINEAR);
        out->set_image( resizeImage );
    }
}

const QString CVImageResizeModel::_category = QString( "Image Operation" );

const QString CVImageResizeModel::_model_name = QString( "Resize" );
