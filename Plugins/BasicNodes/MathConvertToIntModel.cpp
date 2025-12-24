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

#include "MathConvertToIntModel.hpp"

#include "IntegerData.hpp"
#include "SyncData.hpp"

const QString MathConvertToIntModel::_category = QString( "Math Operation" );

const QString MathConvertToIntModel::_model_name = QString( "Convert to Integer" );

MathConvertToIntModel::
MathConvertToIntModel()
    : PBNodeDelegateModel( _model_name ),
    _minPixmap(":/ConvertToInteger.png")
{
    mpIntegerData = std::make_shared< IntegerData >( );
}

unsigned int
MathConvertToIntModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 1;
    else if( portType == PortType::Out )
        return 1;
    else
        return 0;
}

NodeDataType
MathConvertToIntModel::
dataType( PortType portType, PortIndex) const
{
    if( portType == PortType::In )
    {
        return InformationData().type();
    }
    else if( portType == PortType::Out )
    {
        return IntegerData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
MathConvertToIntModel::
outData(PortIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
        result = mpIntegerData;
    return result;
}

void
MathConvertToIntModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;
    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< InformationData >( nodeData );
        if( d )
        {
            bool can_convert = false;
            int data = d->info().toInt(&can_convert);
            if( can_convert )
            {
                mpIntegerData->data() = data;

                updateAllOutputPorts();
            }
        }
    }
}


