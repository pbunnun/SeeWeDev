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

#include "InfoConcatenateModel.hpp"

#include "nodes/DataModelRegistry"
#include "InformationData.hpp"
#include "SyncData.hpp"

InfoConcatenateModel::
InfoConcatenateModel()
    : PBNodeDataModel( _model_name )
{
    mpInformationData = std::make_shared< InformationData >( );
}

unsigned int
InfoConcatenateModel::
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
InfoConcatenateModel::
dataType( PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if(portIndex == 0 || portIndex == 1 )
        {
            return InformationData().type();
        }
        else if(portIndex == 2)
        {
            return SyncData().type();
        }
    }
    else if( portType == PortType::Out )
    {
        return InformationData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
InfoConcatenateModel::
outData(PortIndex portIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
        result = mpInformationData;
    return result;
}

void
InfoConcatenateModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;
    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< InformationData >( nodeData );
        if( d )
            mpInformationData_1 = d;
    }
    else if(portIndex == 1)
    {
        auto d = std::dynamic_pointer_cast< InformationData >( nodeData );
        if( d )
            mpInformationData_2 = d;
    }
    else if(portIndex == 2)
    {
        if( mpInformationData_1 && mpInformationData_2 )
        {
            QString result = mpInformationData_1->info() + ", " + mpInformationData_2->info();
            mpInformationData->set_information( result );
            //qDebug() << result;
            updateAllOutputPorts();
        }
    }
}

const QString InfoConcatenateModel::_category = QString( "Utility" );

const QString InfoConcatenateModel::_model_name = QString( "Info Concatenate" );
