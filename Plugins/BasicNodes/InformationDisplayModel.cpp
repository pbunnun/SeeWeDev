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

#include "InformationDisplayModel.hpp"

#include "nodes/DataModelRegistry"
#include "InformationData.hpp"
#include "SyncData.hpp"

InformationDisplayModel::
InformationDisplayModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget( new InformationDisplayEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{

}

unsigned int
InformationDisplayModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 2;
    else
        return 0;
}

NodeDataType
InformationDisplayModel::
dataType( PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
    {
        return InformationData().type();
    }
    else if(portIndex == 1)
    {
        return SyncData().type();
    }
    return NodeDataType();
}

void
InformationDisplayModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;
    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< InformationData >( nodeData );
        if( d )
        {
            if( !d->info().isEmpty() )
            {
                mpInformationData = d;
                d->set_information();
                mpEmbeddedWidget->appendPlainText("............................................\n");
                mpEmbeddedWidget->appendPlainText( d->info() );
            }
        }
    }
    if(portIndex == 1)
    {
        auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( d )
        {
            mpEmbeddedWidget->appendPlainText(d->state_str() + "\n");
        }
    }
}

const QString InformationDisplayModel::_category = QString( "Output" );

const QString InformationDisplayModel::_model_name = QString( "Info Display" );
