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

#include "CVInvertGrayModel.hpp"


#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

const QString CVInvertGrayModel::_category = QString("Image Conversion");

const QString CVInvertGrayModel::_model_name = QString( "CV Invert Grayscale" );

CVInvertGrayModel::
CVInvertGrayModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":CVInvertGray.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
}

unsigned int
CVInvertGrayModel::
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
CVInvertGrayModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}

std::shared_ptr<NodeData>
CVInvertGrayModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
CVInvertGrayModel::
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
CVInvertGrayModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr< CVImageData > & out )
{
    if( !in->data().empty() && in->data().channels() == 1 )
    {
        cv::bitwise_not(in->data(),out->data());
    }
}


