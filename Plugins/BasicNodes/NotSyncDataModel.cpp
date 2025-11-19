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

#include "NotSyncDataModel.hpp"
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>
#include "qtvariantproperty_p.h"

const QString NotSyncDataModel::_category = QString( "Utility" );

const QString NotSyncDataModel::_model_name = QString( "Not Sync" );

NotSyncDataModel::
NotSyncDataModel()
    : PBNodeDelegateModel( _model_name )
      // PBNodeDataModel( model's name, is it enable at start? )
{
    mpSyncData = std::make_shared< SyncData >();
}

unsigned int
NotSyncDataModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 1 );
    case PortType::Out:
        return( 1 );
    default:
        return( 0 );
    }
}

NodeDataType
NotSyncDataModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( (portType == PortType::Out || portType == PortType::In ) && portIndex == 0 )
        return SyncData().type();
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
NotSyncDataModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpSyncData;
    else
        return nullptr;
}

QJsonObject
NotSyncDataModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDelegateModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDelegateModel::save();

    return modelJson;
}

void
NotSyncDataModel::
load(const QJsonObject &p)
{
    /*
     * If load() was overridden, PBNodeDelegateModel::load() must be called explicitely.
     */
    PBNodeDelegateModel::load(p);

}

void
NotSyncDataModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );
}

void
NotSyncDataModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if( !isEnable() )
        return;
    
    if( nodeData )
    {
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
        if( d )
        {
            mpSyncData->data() = !d->data();
            Q_EMIT dataUpdated( 0 );
        }
    }
}


