#include "InvertGrayModel.hpp"

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

InvertGrayModel::
InvertGrayModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":InvertGray.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
}

unsigned int
InvertGrayModel::
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
InvertGrayModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}

std::shared_ptr<NodeData>
InvertGrayModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
InvertGrayModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if( nodeData )
    {
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
        {
            processData( d, mpCVImageData );
        }
    }

    Q_EMIT dataUpdated( 0 );
}

void
InvertGrayModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr< CVImageData > & out )
{
    if( !in->image().empty() && in->image().channels() == 1 )
    {
        cv::bitwise_not(in->image(),out->image());
    }
}

const QString InvertGrayModel::_category = QString("Image Conversion");

const QString InvertGrayModel::_model_name = QString( "Invert Grayscale" );
