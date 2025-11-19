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

#include "PBNodeDelegateModel.hpp"
#include "qtvariantproperty_p.h"
#include <QFontMetrics>
#include <QFont>
#include <algorithm>

using QtNodes::PortType;

PBNodeDelegateModel::PBNodeDelegateModel(QString modelName, bool bSource, bool bEnable)
    : QtNodes::NodeDelegateModel(),
      mMinPixmap(":NodeEditor.png"),
      msModelName(modelName),
      mOrgNodeStyle(nodeStyle()),
      mbSource(bSource),
      mbEnable(bEnable),
      mbMinimize(false),
      mbLockPosition(false),
      mbDrawEntries(true),
      mbDrawConnectionPoint(true)
{
    setCaption(msModelName);
    // Set default green boundaries for enabled nodes
    // Normal = lighter green, Selected = darker green
    mOrgNodeStyle.NormalBoundaryColor = QColor(0, 150, 0);      // Dark green for unselected
    mOrgNodeStyle.SelectedBoundaryColor = QColor(60, 200, 60);  // Light green for selected
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

    propId = "caption_visible";
    auto propCaptionVisible = std::make_shared< TypedProperty< bool > >( "Show Caption", propId, QMetaType::Bool, mbCaptionVisible, "Common" );
    mvProperty.push_back( propCaptionVisible );
    mMapIdToProperty[ propId ] = propCaptionVisible;

    // Add "Enable Zenoh" property for hybrid Qt/Zenoh mode (default: false = Qt mode)
    propId = "enable_zenoh";
    auto propEnableZenoh = std::make_shared< TypedProperty< bool > >( "Enable Zenoh", propId, QMetaType::Bool, false, "Common" );
    mvProperty.push_back( propEnableZenoh );
    mMapIdToProperty[ propId ] = propEnableZenoh;

    connect( this, SIGNAL( enable_changed_signal(bool) ), this, SLOT( enable_changed(bool) ) );
    connect( this, SIGNAL( minimize_changed_signal(bool) ), this, SLOT( minimize_changed(bool) ) );
    connect( this, SIGNAL( draw_entries_changed_signal(bool) ), this, SLOT( draw_entries_changed(bool) ) );
    connect( this, SIGNAL( lock_position_changed_signal(bool) ), this, SLOT( lock_position_changed(bool) ) );
}

QJsonObject
PBNodeDelegateModel::
save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();

    modelJson["source"] = mbSource;

    QJsonObject params;
    params["caption"] = caption();
    params["minimize"] = isMinimize();
    params["enable"] = isEnable();
    params["draw_entries"] = isDrawEntries();
    params["lock_position"] = isLockPosition();
    params["caption_visible"] = mbCaptionVisible;
    params["enable_zenoh"] = getModelPropertyValue("enable_zenoh").toBool();

    if( mbSource )
        params["enable"] = false;

    modelJson["params"] = params;

    return modelJson;
}

void
PBNodeDelegateModel::
load(QJsonObject const &p)
{
    NodeDelegateModel::load(p);

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

        v = paramsObj[ "caption_visible" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "caption_visible" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mbCaptionVisible = v.toBool();
        }

        v = paramsObj[ "enable_zenoh" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "enable_zenoh" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();
        }
    }

}

QVariant
PBNodeDelegateModel::
getModelPropertyValue(const QString & id) const
{
    if(!mMapIdToProperty.contains(id))
        return QVariant();

    auto prop = mMapIdToProperty[id];
    auto type = prop->getType();
    
    // Extract value based on the property's actual type
    if (type == QMetaType::QString)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
        return QVariant(typedProp->getData());
    }
    else if (type == QMetaType::Int)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        return QVariant(typedProp->getData().miValue);
    }
    else if (type == QMetaType::Double)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        return QVariant(typedProp->getData().mdValue);
    }
    else if (type == QtVariantPropertyManager::enumTypeId())
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        return QVariant(typedProp->getData().miCurrentIndex);
    }
    else if (type == QMetaType::Bool)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
        return QVariant(typedProp->getData());
    }
    else if (type == QtVariantPropertyManager::filePathTypeId())
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(prop);
        return QVariant(typedProp->getData().msFilename);
    }
    else if (type == QtVariantPropertyManager::pathTypeId())
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<PathPropertyType>>(prop);
        return QVariant(typedProp->getData().msPath);
    }
    else if (type == QMetaType::QSize)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<SizePropertyType>>(prop);
        return QVariant(QSize(typedProp->getData().miWidth, typedProp->getData().miHeight));
    }
    else if (type == QMetaType::QSizeF)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<SizeFPropertyType>>(prop);
        return QVariant(QSizeF(typedProp->getData().mfWidth, typedProp->getData().mfHeight));
    }
    else if (type == QMetaType::QRect)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<RectPropertyType>>(prop);
        return QVariant(QRect(typedProp->getData().miXPosition, typedProp->getData().miYPosition,
                              typedProp->getData().miWidth, typedProp->getData().miHeight));
    }
    else if (type == QMetaType::QPoint)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<PointPropertyType>>(prop);
        return QVariant(QPoint(typedProp->getData().miXPosition, typedProp->getData().miYPosition));
    }
    else if (type == QMetaType::QPointF)
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<PointFPropertyType>>(prop);
        return QVariant(QPointF(typedProp->getData().mfXPosition, typedProp->getData().mfYPosition));
    }
    
    // Unknown type - return invalid QVariant
    return QVariant();
}

void
PBNodeDelegateModel::
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

        //enabled( value.toBool() );
        //Q_EMIT property_changed_signal( prop );
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
    else if( id == "caption_visible" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mbCaptionVisible = value.toBool();
        // Trigger repaint to show/hide caption
        Q_EMIT embeddedWidgetSizeUpdated();
    }
}

void
PBNodeDelegateModel::
setEnable( bool enable )
{
    enabled( enable );

    auto prop = mMapIdToProperty[ "enable" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
    typedProp->getData() = enable;
    Q_EMIT property_changed_signal( prop );
    Q_EMIT enable_changed_signal( enable );
}

void
PBNodeDelegateModel::
enabled( bool enable )
{
    // Track the state internally and update the node style
    mbEnable = enable;

    if( enable )
    {
        // Enabled: Green boundaries (light when not selected, dark when selected)
        setNodeStyle(mOrgNodeStyle);
    }
    else
    {
        // Disabled: Red boundaries (light when not selected, dark when selected)
        auto style = mOrgNodeStyle;
        style.NormalBoundaryColor = QColor(180, 0, 0);     // Dark red for unselected
        style.SelectedBoundaryColor = QColor(220, 80, 80); // Light red for selected
        setNodeStyle(style);
    }
}

void
PBNodeDelegateModel::
minimized( bool minimize )
{
    // NodeEditor v3 Migration: In v2, this called NodeDataModel::setMinimize()
    // In v3, we just track the state internally
    mbMinimize = minimize;
    
    // Trigger node geometry recalculation and repaint
    Q_EMIT embeddedWidgetSizeUpdated();
}

void
PBNodeDelegateModel::
locked_position( bool lock_position )
{
    // position locking should be handled by NodeFlags in the graph model
    mbLockPosition = lock_position;
}

void
PBNodeDelegateModel::
draw_entries( bool draw )
{
    // we track the state internally and request node size update
    mbDrawEntries = draw;
    
    // Trigger node geometry recalculation and repaint
    Q_EMIT embeddedWidgetSizeUpdated();
}

void
PBNodeDelegateModel::
updateAllOutputPorts()
{
    auto no_output_ports = nPorts( PortType::Out );
    
    // Qt mode (default): Use Qt signals only
    for( unsigned int i = 0; i < no_output_ports; i++ )
        Q_EMIT dataUpdated( i );
}

void
PBNodeDelegateModel::
enable_changed( bool enable )
{
    enabled( enable );
    if( enable )
        updateAllOutputPorts();
}

void
PBNodeDelegateModel::
lock_position_changed( bool lock_position )
{
    locked_position( lock_position );
}

QSize
PBNodeDelegateModel::
calculateMinimumWidgetSize(const QString& caption, int nInPorts, int nOutPorts) const
{
    // Constants from DefaultVerticalNodeGeometry
    const int PORT_SPACING = 10;
    const int PORT_SIZE = 8;
    
    // Estimate caption width (rough approximation - Qt will calculate exact size)
    QFont font;
    QFontMetrics fm(font);
    int captionWidth = fm.horizontalAdvance(caption);
    
    // Calculate minimum width based on ports
    // Each port needs space, with spacing between them
    int inPortsWidth = nInPorts > 0 ? (PORT_SIZE * nInPorts + PORT_SPACING * (nInPorts - 1)) : 0;
    int outPortsWidth = nOutPorts > 0 ? (PORT_SIZE * nOutPorts + PORT_SPACING * (nOutPorts - 1)) : 0;
    int portsWidth = std::max(inPortsWidth, outPortsWidth);
    
    // Minimum width = max(caption, ports) + left/right spacing
    int minWidth = std::max(captionWidth, portsWidth) + 2 * PORT_SPACING;
    
    // Minimum height should accommodate some content below caption
    // Caption height ~20-30 pixels + some space for widget content
    int captionHeight = fm.height();
    int minHeight = captionHeight + PORT_SPACING * 2; // Space above and below caption
    
    return QSize(minWidth, minHeight);
}

void
PBNodeDelegateModel::
requestPropertyChange(const QString& propertyId, const QVariant& newValue, bool createUndoCommand)
{
    DEBUG_LOG_INFO() << "[requestPropertyChange] propertyId:" << propertyId 
            << "newValue:" << newValue 
            << "createUndoCommand:" << createUndoCommand
            << "isSelected:" << isSelected();
    
    // Get the old value
    QVariant oldValue = getModelPropertyValue(propertyId);
    
    // If values are the same, no need to do anything
    if (oldValue == newValue)
    {
        DEBUG_LOG_INFO() << "[requestPropertyChange] Values are the same, skipping";
        return;
    }
    
    if (createUndoCommand)
    {
        DEBUG_LOG_INFO() << "[requestPropertyChange] Emitting property_change_request_signal";
        // Always go through undo/redo system for trackable changes
        // Emit signal for MainWindow to create PropertyChangeCommand
        // This works regardless of selection state
        Q_EMIT property_change_request_signal(propertyId, oldValue, newValue);
    }
    else
    {
        DEBUG_LOG_INFO() << "[requestPropertyChange] Applying directly (undo/redo calling us)";
        // Only when undo/redo is calling us (createUndoCommand=false), apply directly
        // This prevents infinite loops when PropertyChangeCommand::redo() calls setModelProperty
        QString propId = propertyId;  // Create non-const copy
        setModelProperty(propId, newValue);
        
        // Emit property_changed_signal to sync UI (Property Browser, etc.)
        // Only emit if node is selected (otherwise Property Browser doesn't show this node)
        if (isSelected() && mMapIdToProperty.contains(propertyId))
        {
            DEBUG_LOG_INFO() << "[requestPropertyChange] Emitting property_changed_signal for UI sync";
            Q_EMIT property_changed_signal(mMapIdToProperty[propertyId]);
        }
    }
}

void
PBNodeDelegateModel::
editable_embedded_widget_selected_changed( bool isSelected )
{
    DEBUG_LOG_INFO() << "[editable_embedded_widget_selected_changed] isSelected:" << isSelected;
    
    mbEditableEmbeddedWidgetSelected = isSelected;
    Q_EMIT selection_request_signal();
}
