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

#include "CVMinMaxLocationModel.hpp"

#include <QDebug> //for debugging using qDebug()


#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVMinMaxLocationModel::_category = QString( "Image Analysis" );

const QString CVMinMaxLocationModel::_model_name = QString( "CV MinMax Location" );

CVMinMaxLocationModel::
CVMinMaxLocationModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":MinMaxLocation.png" )
{
    for(std::shared_ptr<CVPointData>& mp : mapCVPointData)
    {
        mp = std::make_shared<CVPointData>(cv::Point());
    }
    for(std::shared_ptr<DoubleData>& mp : mapDoubleData)
    {
        mp = std::make_shared<DoubleData>(double());
    }
}

unsigned int
CVMinMaxLocationModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 4;
        break;

    default:
        break;
    }

    return result;
}


NodeDataType
CVMinMaxLocationModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if(portType == PortType::In)
    {
        return CVImageData().type();
    }
    else if(portType == PortType::Out)
    {
        if(portIndex == 0 || portIndex == 1)
        {
            return CVPointData().type();
        }
        else if(portIndex == 2 || portIndex == 3)
        {
            return DoubleData().type();
        }
    }
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVMinMaxLocationModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        if(I == 0 || I == 1)
        {
            return mapCVPointData[I];
        }
        else if(I == 2 || I == 3)
        {
            return mapDoubleData[I-2];
        }
    }
    return nullptr;
}

void
CVMinMaxLocationModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mapCVPointData, mapDoubleData );
        }
    }

    updateAllOutputPorts();
}

void
CVMinMaxLocationModel::
processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVPointData> (&outPoint)[2], std::shared_ptr<DoubleData>(&outDouble)[2])
{
    cv::Mat& in_image = in->data();
    if(!in_image.empty() && in_image.channels()==1)
    {
        cv::minMaxLoc(in->data(),
                      &outDouble[0]->data(),
                      &outDouble[1]->data(),
                      &outPoint[0]->data(),
                      &outPoint[1]->data());
    }
}


