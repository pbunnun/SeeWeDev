#include "WatershedModel.hpp"

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

WatershedModel::
WatershedModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":Watershed.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >();
}

unsigned int
WatershedModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 2;
        break;

    case PortType::Out:
        result = 2;
        break;

    default:
        break;
    }

    return result;
}

NodeDataType
WatershedModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if(portType == PortType::Out && portIndex == 1)
        return SyncData().type();
    else
        return CVImageData().type();
}

std::shared_ptr<NodeData>
WatershedModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        if(I==0)
            return mpCVImageData;
        else if( I==1 )
            return mpSyncData;
    }
    return nullptr;
}

void
WatershedModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;

    if( nodeData )
    {
        mpSyncData->state() = false;
        Q_EMIT dataUpdated(1);
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
        {
            mapCVImageInData[portIndex] = d;
            if(mapCVImageInData[0] && mapCVImageInData[1])
            {
                processData( mapCVImageInData, mpCVImageData );
            }
        }
        mpSyncData->state() = true;
        Q_EMIT dataUpdated(1);
    }

    Q_EMIT dataUpdated( 0 );
}

void
WatershedModel::
processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr< CVImageData > & out )
{
    cv::Mat& in_image = in[0]->image();
    cv::Mat& in_marker = in[1]->image();
    if(!in_image.empty() && !in_marker.empty() && in_image.type()==CV_8UC3 && in_marker.type()==CV_32SC1)
    {
        out->set_image(in_marker);
        cv::watershed(in_image,out->image());
    }
}

const QString WatershedModel::_category = QString("Image Transformation");

const QString WatershedModel::_model_name = QString( "Watershed" );
