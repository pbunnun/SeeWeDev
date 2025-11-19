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

#include "MathIntegerSumModel.hpp"

#include "IntegerData.hpp"
#include "SyncData.hpp"

const QString MathIntegerSumModel::_category = QString( "Math Operation" );

const QString MathIntegerSumModel::_model_name = QString( "Sum Integer" );

MathIntegerSumModel::
MathIntegerSumModel()
    : PBNodeDelegateModel( _model_name )
{
    mpIntegerData = std::make_shared< IntegerData >( );
}

unsigned int
MathIntegerSumModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 3;
    else if( portType == PortType::Out )
        return 1;
    else
        return 0;
}

NodeDataType
MathIntegerSumModel::
dataType( PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if(portIndex == 0 || portIndex == 1 )
        {
            return IntegerData().type();
        }
        else if(portIndex == 2)
        {
            return SyncData().type();
        }
    }
    else if( portType == PortType::Out )
    {
        return IntegerData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
MathIntegerSumModel::
outData(PortIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
        result = mpIntegerData;
    return result;
}

void
MathIntegerSumModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;
    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< IntegerData >( nodeData );
        if( d )
            mpIntegerData_1 = d;
    }
    else if(portIndex == 1)
    {
        auto d = std::dynamic_pointer_cast< IntegerData >( nodeData );
        if( d )
            mpIntegerData_2 = d;
    }
    else if(portIndex == 2)
    {
        if( mpIntegerData_1 && mpIntegerData_2 )
        {
            mpIntegerData->data() = mpIntegerData_1->data() + mpIntegerData_2->data();

            updateAllOutputPorts();
        }
    }
}


