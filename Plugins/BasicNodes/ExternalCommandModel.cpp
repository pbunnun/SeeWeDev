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

#include "ExternalCommandModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"
#include <QProcess>

const QString ExternalCommandModel::_category = QString( "Utility" );

const QString ExternalCommandModel::_model_name = QString( "Call External Command" );

ExternalCommandModel::
ExternalCommandModel()
    : PBNodeDelegateModel( _model_name )
{
    QString propId = "ext_command";
    auto propCommand = std::make_shared< TypedProperty< QString > >( "External Command", propId, QMetaType::QString, msExternalCommand);
    mvProperty.push_back( propCommand );
    mMapIdToProperty[ propId ] = propCommand;

    propId = "arguments";
    auto propArguments = std::make_shared< TypedProperty< QString > >( "Arguments", propId, QMetaType::QString, msArguments);
    mvProperty.push_back( propArguments );
    mMapIdToProperty[ propId ] = propArguments;
}

unsigned int
ExternalCommandModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 1;
    else
        return 0;
}

NodeDataType
ExternalCommandModel::
dataType( PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if( portIndex == 0 )
        {
            return SyncData().type();
        }
    }
    return NodeDataType();
}

void
ExternalCommandModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() || !nodeData )
        return;

    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< SyncData > ( nodeData );
        if( d && d->data() )
        {
            QProcess cmd;
            cmd.start(msExternalCommand, QStringList() << msArguments);
            cmd.waitForFinished();
        }
    }
}

QJsonObject
ExternalCommandModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["ext_command"] = msExternalCommand;
    cParams["arguments"] = msArguments;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
ExternalCommandModel::
load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj["ext_command"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["ext_command"];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > > ( prop );
            typedProp->getData() = v.toString();

            msExternalCommand = v.toString();
        }
        v = paramsObj["arguments"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["arguments"];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > > ( prop );
            typedProp->getData() = v.toString();

            msArguments = v.toString();
        }
    }
}

void
ExternalCommandModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    if( id == "ext_command" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();

        msExternalCommand = value.toString();
    }
    else if( id == "arguments" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();

        msArguments = value.toString();
    }
}


