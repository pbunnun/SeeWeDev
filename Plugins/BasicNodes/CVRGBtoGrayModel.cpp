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

#include "CVRGBtoGrayModel.hpp"


#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

const QString CVRGBtoGrayModel::_category = QString("Image Conversion");

const QString CVRGBtoGrayModel::_model_name = QString( "CV RGB to Gray" );

CVRGBtoGrayModel::
CVRGBtoGrayModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":RGBtoGray.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >();
}

unsigned int
CVRGBtoGrayModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
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
CVRGBtoGrayModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
        return CVImageData().type();
    else
        return SyncData().type();
}

std::shared_ptr<NodeData>
CVRGBtoGrayModel::
outData(PortIndex port)
{
    if( isEnable() )
    {
        if( port == 0 && mpCVImageData->data().data != nullptr )
            return mpCVImageData;
        else if( port == 1 )
            return mpSyncData;
    }
    return nullptr;
}

void
CVRGBtoGrayModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if( nodeData )
    {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
        {
            processData( d, mpCVImageData );
        }
        mpSyncData->data() = true;
        Q_EMIT dataUpdated(1);
    }

    Q_EMIT dataUpdated( 0 );
}

void
CVRGBtoGrayModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr< CVImageData > & out )
{
    cv::Mat& in_image = in->data();
    if(!in_image.empty() && in_image.type()==CV_8UC3)
    {
        cv::cvtColor( in_image, out->data(), cv::COLOR_BGR2GRAY );
    }
}


