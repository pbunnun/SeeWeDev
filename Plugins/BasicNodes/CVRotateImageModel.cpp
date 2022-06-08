#include "CVRotateImageModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>
#include "qtvariantproperty.h"

CVRotateImageModel::
CVRotateImageModel()
    : PBNodeDataModel( _model_name )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    mpCVImageInData = std::make_shared< CVImageData >( cv::Mat() );
    mpCVImageOutData = std::make_shared< CVImageData >( cv::Mat() );

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mdAngle;
    doublePropertyType.mdMax = 360;
    QString propId = "angle_id";
    auto propRect = std::make_shared< TypedProperty< DoublePropertyType > >("Rotate", propId, QMetaType::Double, doublePropertyType);
    mvProperty.push_back( propRect );
    mMapIdToProperty[ propId ] = propRect;
}

unsigned int
CVRotateImageModel::
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
CVRotateImageModel::
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
CVRotateImageModel::
outData(PortIndex)
{
    if( isEnable() && mpCVImageOutData->image().data != nullptr )
        return mpCVImageOutData;
    else
        return nullptr;
}

void
CVRotateImageModel::
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
CVRotateImageModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDataModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams[ "angle" ] = mdAngle;

    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
CVRotateImageModel::
restore(const QJsonObject &p)
{
    /*
     * If restore() was overrided, PBNodeDataModel::restore() must be called explicitely.
     */
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue angle = paramsObj[ "angle" ];
        if( !angle.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "angle_id" ];
            /* Restore internal property */
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = angle.toDouble();

            mdAngle = angle.toDouble();
        }
    }
}

void
CVRotateImageModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "angle_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mdAngle = value.toDouble();

        processData(mpCVImageInData, mpCVImageOutData);
        Q_EMIT dataUpdated(0);
    }
}

void
CVRotateImageModel::
processData(const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out )
{
    if( !in->image().empty() )
    {
        auto image = in->image();
        cv::Point2f center((image.cols-1)/2.0, (image.rows-1)/2.0);
        cv::Mat rot = cv::getRotationMatrix2D(center, mdAngle, 1.0);
        cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), image.size(), mdAngle).boundingRect2f();
        rot.at<double>(0,2) += bbox.width/2.0 - image.cols/2.0;
        rot.at<double>(1,2) += bbox.height/2.0 - image.rows/2.0;
        cv::Mat dst;
        cv::warpAffine(image, dst, rot, bbox.size());
        out->set_image( dst );
    }
}

const QString CVRotateImageModel::_category = QString( "Image Operation" );

const QString CVRotateImageModel::_model_name = QString( "Rotate" );
