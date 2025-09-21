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

#include "PBNodeDataModel.hpp"

using QtNodes::PortType;

PBNodeDataModel::PBNodeDataModel(QString modelName, bool bSource, bool bEnable)
    : QtNodes::NodeDataModel(),
      msModelName(modelName),
      mOrgNodeStyle(nodeStyle()),
      mbSource(bSource)
{
    setCaption(msModelName);
    mOrgNodeStyle.NormalBoundaryColor = Qt::darkGreen;
    mOrgNodeStyle.SelectedBoundaryColor = Qt::green;
    setNodeStyle(mOrgNodeStyle);
    if(mbSource) // if this node is a source node, it must be disabled from start. Set bEnable to false internally.
        bEnable = false;
    enabled(bEnable);

    QString propId = "caption";
    auto propCaption = std::make_shared< TypedProperty< QString > >( "Caption", propId, QMetaType::QString, msCaptionName );
    mvProperty.push_back( propCaption );
    mMapIdToProperty[ propId ] = propCaption;

    propId = "lock_position";
    auto propLock_Position = std::make_shared< TypedProperty< bool > >( "Lock Position", propId, QMetaType::Bool, isLockPosition(), "Common" );
    mvProperty.push_back( propLock_Position );
    mMapIdToProperty[ propId ] = propLock_Position;

    propId = "enable";
    auto propEnable = std::make_shared< TypedProperty< bool > >( "Enable", propId, QMetaType::Bool, isEnable(), "Common" );
    mvProperty.push_back( propEnable );
    mMapIdToProperty[ propId ] = propEnable;

    propId = "minimize";
    auto propMinimize = std::make_shared< TypedProperty< bool > >( "Minimize", propId, QMetaType::Bool, isMinimize(), "Common" );
    mvProperty.push_back( propMinimize );
    mMapIdToProperty[ propId ] = propMinimize;

    propId = "draw_entries";
    auto propDrawEntries = std::make_shared< TypedProperty< bool > >( "Draw Entries", propId, QMetaType::Bool, isDrawEntries(), "Common" );
    mvProperty.push_back( propDrawEntries );
    mMapIdToProperty[ propId ] = propDrawEntries;

    connect( this, SIGNAL( enable_changed_signal(bool) ), this, SLOT( enable_changed(bool) ) );
    connect( this, SIGNAL( minimize_changed_signal(bool) ), this, SLOT( minimize_changed(bool) ) );
    connect( this, SIGNAL( draw_entries_changed_signal(bool) ), this, SLOT( draw_entries_changed(bool) ) );
    connect( this, SIGNAL( lock_position_changed_signal(bool) ), this, SLOT( lock_position_changed(bool) ) );
}

QJsonObject
PBNodeDataModel::
save() const
{
    QJsonObject modelJson = NodeDataModel::save();

    modelJson["source"] = mbSource;

    QJsonObject params = modelJson["params"].toObject();
    params["caption"] = caption();
    if( mbSource )
        params["enable"] = false;

    modelJson["params"] = params;

    return modelJson;
}

void
PBNodeDataModel::
restore(QJsonObject const &p)
{
    NodeDataModel::restore(p);

    QJsonValue v;
    v = p[ "source" ];
    if( !v.isNull() )
        mbSource = v.toBool();

    QJsonObject paramsObj = p["params"].toObject();
    if( !paramsObj.isEmpty() )
    {
        v = paramsObj["caption"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["caption"];
            auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
            typedProp->getData() = v.toString();

            setCaption( v.toString() );
        }

        v = paramsObj[ "enable" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "enable" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            if( mbSource )
            {
                typedProp->getData() = false;

                enabled( false );
            }
            else
            {
                typedProp->getData() = v.toBool();

                enabled( v.toBool() );
            }
        }

        v = paramsObj[ "minimize" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "minimize" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            minimized( v.toBool() );
        }

        v = paramsObj[ "lock_position" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "lock_position" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            locked_position( v.toBool() );
        }

        v = paramsObj[ "draw_entries" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "draw_entries" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            draw_entries( v.toBool() );
        }
    }

}

void
PBNodeDataModel::
setModelProperty(QString & id, const QVariant & value)
{
    if(!mMapIdToProperty.contains(id))
        return;

    auto prop = mMapIdToProperty[id];
    if( id == "caption" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
        typedProp->getData() = value.toString();

        setCaption( value.toString() );
    }
    else if( id == "enable" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        enabled( value.toBool() );
        Q_EMIT enable_changed_signal( value.toBool() );
    }
    else if( id == "minimize" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        minimized( value.toBool() );
        Q_EMIT minimize_changed_signal( value.toBool() );
    }
    else if( id == "lock_position" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        locked_position( value.toBool() );
        Q_EMIT lock_position_changed_signal( value.toBool() );
    }
    else if( id == "draw_entries" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        draw_entries( value.toBool() );
        Q_EMIT draw_entries_changed_signal( value.toBool() );
    }
}

void
PBNodeDataModel::
setEnable( bool enable )
{
    enabled( enable );

    auto prop = mMapIdToProperty[ "enable" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
    typedProp->getData() = enable;
    Q_EMIT property_changed_signal( prop );
    if( !mbSelected ) // if the node is selected, don't need to emit the following signal.
        Q_EMIT enable_changed_signal( enable );
}

void
PBNodeDataModel::
setMinimize( bool minimize )
{
    minimized( minimize );

    auto prop = mMapIdToProperty[ "minimize" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
    typedProp->getData() = minimize;
    Q_EMIT property_changed_signal( prop );
}

void
PBNodeDataModel::
setLockPosition( bool lock_position )
{
    locked_position( lock_position );

    auto prop = mMapIdToProperty[ "lock_position" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
    typedProp->getData() = lock_position;
    Q_EMIT property_changed_signal( prop );
}

void
PBNodeDataModel::
setDrawEntries( bool draw )
{
    draw_entries( draw );

    auto prop = mMapIdToProperty[ "draw_entries" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
    typedProp->getData() = draw;
    Q_EMIT property_changed_signal( prop );
}

void
PBNodeDataModel::
enabled( bool enable )
{
    NodeDataModel::setEnable( enable );

    if( enable )
        setNodeStyle(mOrgNodeStyle);
    else
    {
        auto style = mOrgNodeStyle;
        style.NormalBoundaryColor = Qt::darkRed;
        style.SelectedBoundaryColor = Qt::red;
        setNodeStyle(style);
    }
}

void
PBNodeDataModel::
minimized( bool minimize )
{
    NodeDataModel::setMinimize( minimize );
}

void
PBNodeDataModel::
locked_position( bool lock_position )
{
    NodeDataModel::setLockPosition( lock_position );
}

void
PBNodeDataModel::
draw_entries( bool draw )
{
    NodeDataModel::setDrawEntries( draw );
}

void
PBNodeDataModel::
updateAllOutputPorts()
{
    auto no_output_ports = nPorts( PortType::Out );
    for( unsigned int i = 0; i < no_output_ports; i++ )
        Q_EMIT dataUpdated( i );
}

void
PBNodeDataModel::
enable_changed( bool enable )
{
    enabled( enable );
    if( enable )
        updateAllOutputPorts();
}

void
PBNodeDataModel::
lock_position_changed( bool lock_position )
{
    locked_position( lock_position );
}
