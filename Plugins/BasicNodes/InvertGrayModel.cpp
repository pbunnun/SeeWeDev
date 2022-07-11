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
