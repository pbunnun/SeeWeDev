#include "CVImageROIModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>
#include "qtvariantproperty.h"

CVImageROIModel::
CVImageROIModel()
    : PBNodeDataModel( _model_name )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    mpCVImageInData = std::make_shared< CVImageData >( cv::Mat() );
    mpCVImageOutData = std::make_shared< CVImageData >( cv::Mat() );

    RectPropertyType rectPropertyType;
    rectPropertyType.miXPosition = mRect.x;
    rectPropertyType.miYPosition = mRect.y;
    rectPropertyType.miWidth = mRect.width;
    rectPropertyType.miHeight = mRect.height;
    QString propId = "rect_id";
    auto propRect = std::make_shared< TypedProperty< RectPropertyType > >("ROI", propId, QMetaType::QRect, rectPropertyType);
    mvProperty.push_back( propRect );
    mMapIdToProperty[ propId ] = propRect;
}

unsigned int
CVImageROIModel::
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
CVImageROIModel::
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
CVImageROIModel::
outData(PortIndex)
{
    if( isEnable() && mpCVImageOutData->image().data != nullptr )
        return mpCVImageOutData;
    else
        return nullptr;
}

void
CVImageROIModel::
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
CVImageROIModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDataModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams[ "x" ] = mRect.x;
    cParams[ "y" ] = mRect.y;
    cParams[ "width" ] = mRect.width;
    cParams[ "height" ] = mRect.height;

    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
CVImageROIModel::
restore(const QJsonObject &p)
{
    /*
     * If restore() was overrided, PBNodeDataModel::restore() must be called explicitely.
     */
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue x = paramsObj[ "x" ];
        QJsonValue y = paramsObj[ "y" ];
        QJsonValue width = paramsObj[ "width" ];
        QJsonValue height = paramsObj[ "height" ];
        if( !x.isUndefined() && !y.isUndefined() && !width.isUndefined() && !height.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "rect_id" ];
            /* Restore internal property */
            auto typedProp = std::static_pointer_cast< TypedProperty< RectPropertyType > >( prop );
            typedProp->getData().miXPosition = x.toInt();
            typedProp->getData().miYPosition = y.toInt();
            typedProp->getData().miWidth = width.toInt();
            typedProp->getData().miHeight = height.toInt();
            mRect = cv::Rect(x.toInt(), y.toInt(), width.toInt(), height.toInt());
        }
    }
}

void
CVImageROIModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "rect_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< RectPropertyType > >( prop );
        auto rect = value.toRect();

        typedProp->getData().miXPosition = rect.x();
        typedProp->getData().miYPosition = rect.y();
        typedProp->getData().miWidth = rect.width();
        typedProp->getData().miHeight = rect.height();

        mRect = cv::Rect( rect.x(), rect.y(), rect.width(), rect.height() );

        processData(mpCVImageInData, mpCVImageOutData);
        Q_EMIT dataUpdated(0);
    }
}

void
CVImageROIModel::
processData(const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out )
{
    if( !in->image().empty() )
    {
        auto image = in->image();
        auto rect = mRect;
        if( rect.x < image.cols && rect.y < image.rows )
        {
            if( rect.x + rect.width >= image.cols )
                rect.width = image.cols - rect.x;
            if( rect.y + rect.height >= image.rows )
                rect.height = image.rows - rect.y;
            auto roi = image(rect);
            out->set_image( roi );
        }
    }
}

const QString CVImageROIModel::_category = QString( "Image Operation" );

const QString CVImageROIModel::_model_name = QString( "ROI" );
