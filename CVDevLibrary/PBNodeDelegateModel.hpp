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

#pragma once

// NodeEditor v3 Migration: Migrated to use NodeDelegateModel from v3
// Changes made:
// - Now inherits from QtNodes::NodeDelegateModel
// - Removed v2-specific methods: setEnable(), setMinimize(), setLockPosition(), setDrawEntries()
// - These features are commented out and need reimplementation in the application layer:
//   * Enable/Disable nodes - use validation state or custom tracking
//   * Minimize nodes - requires custom painter/geometry implementation  
//   * Lock position - use NodeFlags in DataFlowGraphModel
//   * Draw entries - controlled by node geometry in v3

#include "CVDevLibrary.hpp"
#include "Property.hpp"
#include "DebugLogging.hpp"
#include <QtNodes/NodeDelegateModel>

using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeStyle;

class CVDEVSHAREDLIB_EXPORT PBNodeDelegateModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT
public:
    explicit PBNodeDelegateModel(QString modelName, bool bSource=false, bool bEnable=true);

    QJsonObject
    save() const override;

    void
    load(QJsonObject const & p) override;  // v3: restore() renamed to load()

    QString
    caption() const override { return msCaptionName; };

    void
    setCaption(QString caption) { msCaptionName = caption; };

    QString
    name() const override { return msModelName; };

    QString
    modelName() const { return msModelName; };

    PropertyVector
    getProperty() { return mvProperty; };

    QVariant
    getModelPropertyValue(const QString & id) const;

    virtual
    std::shared_ptr<NodeData>
    outData(PortIndex) override { return nullptr; };

    virtual
    void
    setModelProperty(QString & , const QVariant & );

    // They now properly update the internal state and emit signals
    void setEnable( bool enable );
    void setMinimize( bool minimize ) { mbMinimize = minimize; }
    void setLockPosition( bool lock ) { mbLockPosition = lock; }
    void setDrawEntries( bool draw ) { mbDrawEntries = draw; }
    void setDrawConnectionPoints( bool draw ) { mbDrawConnectionPoint = draw; }

    bool isEnable() const { return mbEnable; }
    bool isMinimize() const { return mbMinimize; }
    bool isLockPosition() const { return mbLockPosition; }
    bool isDrawEntries() const { return mbDrawEntries; }
    bool isDrawConnectionPoints() const { return mbDrawConnectionPoint; }

    // Override from NodeDelegateModel to control caption visibility
    bool captionVisible() const override { return mbCaptionVisible; }

    // Virtual method to indicate whether the node is resizable.
    // Default is true so most nodes support dynamic resizing unless
    // a derived model explicitly returns false.
    virtual bool resizable() const override { return true; }

    // Virtual method to check if node can be minimized
    virtual bool canMinimize() const { return true; }

    /// Call this function when a node want to initialise somethings, eg. hardware interface, after it was added to the scene.
    ///
    /// The base implementation is idempotent: it records that late construction
    /// has been performed. Derived classes that perform heavy initialization
    /// should either call `PBNodeDelegateModel::late_constructor()` at the top
    /// of their override, or call the protected helper `start_late_constructor()`
    /// to check-and-mark. This prevents double initialization when both the
    /// graph model and the UI previously invoked `late_constructor()`.
    virtual void late_constructor() { }

protected:
    void
    updateAllOutputPorts();

public:
    virtual
    void
    setSelected( bool selected ) { mbSelected = selected; }

    bool
    isSelected( ) const { return mbSelected; }

    bool
    isSource( ) const { return mbSource; }

    /**
     * @brief Checks if this node has Zenoh enabled.
     * @return bool True if "enable_zenoh" property is true, false otherwise
     */
    bool isZenohEnabled() const {
        return getModelPropertyValue("enable_zenoh").toBool();
    }

    /**
     * @brief Gets a unique identifier for this node (for Zenoh keys).
     * @return QString Node identifier combining model name and instance ID
     */
    QString getNodeId() const {
        return QString("%1_%2").arg(msModelName).arg(reinterpret_cast<quintptr>(this));
    }

    virtual QPixmap
    minPixmap() const { return mMinPixmap; }

    /**
     * @brief Request a property change through the undo/redo system.
     * 
     * This is the SINGLE POINT OF ENTRY for all property changes, regardless of source:
     * - Embedded widgets should call this instead of directly calling setters
     * - Property Browser changes route through MainWindow to here
     * - Programmatic changes should use this for undo/redo support
     * 
     * @param propertyId The ID of the property to change
     * @param newValue The new value for the property
     * @param createUndoCommand If true, creates an undo command (default). Set to false when called from undo/redo itself.
     */
    void requestPropertyChange(const QString& propertyId, const QVariant& newValue, bool createUndoCommand = true);

    // Tracking if the embedded widget is currently selected for editing
    bool isEditableEmbeddedWidgetSelected() const { return mbEditableEmbeddedWidgetSelected; }

protected:
    /**
     * @brief Check if node is selected before allowing embedded widget interactions.
     * 
     * Call this at the start of any embedded widget signal handler to enforce
     * the policy that nodes must be selected before users can interact with them.
     * This prevents accidental interactions with unselected nodes.
     * 
     * @return true if the node is selected and interaction should proceed, false otherwise
     */
    bool checkSelectionForInteraction() const
    {
        if (!isSelected())
        {
            qInfo() << "[PBNodeDelegateModel::checkSelectionForInteraction] Node not selected, interaction blocked";
            return false;
        }
        return true;
    }
    
    // Helper to calculate minimum widget size based on ports and caption
    // This ensures the embedded widget is large enough to accommodate the node layout
    QSize calculateMinimumWidgetSize(const QString& caption, int nInPorts, int nOutPorts) const;
    
    // Start the late constructor if not already started
    bool mbLateConstructed{ false };
    bool start_late_constructor()
    {
        if (!mbLateConstructed)
        {
            mbLateConstructed = true;
            return true;
        }
        return false;
    }

Q_SIGNALS:
    /**
     * Emitted when a property needs to be changed through undo/redo system.
     * MainWindow listens to this and creates PropertyChangeCommand.
     */
    void property_change_request_signal(const QString& propertyId, const QVariant& oldValue, const QVariant& newValue);

    /**
     * Emitted after a property has been changed (for UI synchronization only).
     * MainWindow listens to this to update the Property Browser.
     */
    void property_changed_signal( std::shared_ptr<Property> );

    /**
     * Emitted when an unselected node wants to be selected (e.g., user clicked embedded widget).
     * MainWindow listens to this and selects the node's graphics object.
     */
    void selection_request_signal();

    void
    enable_changed_signal( bool );

    void
    minimize_changed_signal( bool );

    void
    lock_position_changed_signal( bool );

    void
    draw_entries_changed_signal( bool );

    void
    property_structure_changed_signal( );

public Q_SLOTS:
    virtual
    void editable_embedded_widget_selected_changed( bool );


protected Q_SLOTS:
    virtual
    void enable_changed( bool );

    virtual
    void draw_entries_changed( bool ) {};

    virtual
    void minimize_changed( bool ) {};

    virtual
    void lock_position_changed( bool );

protected:
    PropertyVector mvProperty;
    QMap<QString, std::shared_ptr<Property>> mMapIdToProperty;
    bool mbSelected{false};  // Nodes are NOT selected by default
    QPixmap mMinPixmap;

private:
    QString msCaptionName;
    QString msModelName;

    NodeStyle mOrgNodeStyle;

    //Tracking states internally since NodeEditor doesn't have these built-in
    bool mbSource{false};
    bool mbEnable{true};
    bool mbMinimize{false};
    bool mbLockPosition{false};
    bool mbDrawEntries{true};
    bool mbDrawConnectionPoint{true};
    bool mbCaptionVisible{true};
    bool mbEditableEmbeddedWidgetSelected{false};

    void enabled( bool );
    virtual void minimized( bool );
    void locked_position( bool );
    void draw_entries( bool );
};
