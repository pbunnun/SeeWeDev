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

/**
 * @file MainWindow.hpp
 * @brief Main application window for CVDev visual node-based programming environment
 * 
 * This file defines the MainWindow class which serves as the primary interface for the
 * CVDev application. It manages multiple tabbed flow graph scenes, provides a property
 * browser for node configuration, and integrates with the Qt NodeEditor framework.
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeDelegateModelRegistry>

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QPluginLoader>
#include "PBNodeDelegateModel.hpp"
#include "PBDataFlowGraphModel.hpp"
#include "PBFlowGraphicsView.hpp"
#include "PBDataFlowGraphicsScene.hpp"
#include "qtpropertymanager_p.h"
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QtVariantProperty;
class QtProperty;

using QtNodes::NodeId;
using QtNodes::NodeGraphicsObject;
using QtNodes::NodeDelegateModelRegistry;
using QtNodes::DataFlowGraphicsScene;

/**
 * @struct SceneProperty
 * @brief Container for managing a single flow graph scene's components
 * 
 * Each tab in the MainWindow contains a complete flow graph scene with its own
 * model, graphics scene, and view. This structure keeps these related objects
 * together for proper lifecycle management.
 * 
 * Design rationale: Grouping related components together ensures they are:
 * - Created together in the correct order (model -> scene -> view)
 * - Deleted together in the correct order (view -> scene -> model)
 * - Easily associated with their tab widget
 */
struct SceneProperty
{
    QString sFilename;  ///< Path to the .flow file (empty for unsaved scenes)
    PBDataFlowGraphModel * pDataFlowGraphModel{nullptr};  ///< Data model containing nodes and connections
    PBDataFlowGraphicsScene * pDataFlowGraphicsScene{nullptr};  ///< Graphics scene for visualization
    PBFlowGraphicsView  * pFlowGraphicsView{nullptr}; ///< View widget (added to tab widget)
};

/**
 * @struct SelectedNodeResult
 * @brief Result type for querying selected node with unambiguous validity check
 * 
 * This struct solves a critical ambiguity: NodeId is defined as unsigned int, so NodeId(0)
 * is a valid node identifier. Previously, getSelectedNodeId() returned NodeId(0) to indicate
 * both "no selection" and "node 0 is selected", making it impossible to distinguish between
 * these two cases.
 * 
 * Design rationale:
 * - hasSelection = true, nodeId = 0  -> Node with ID 0 is selected (valid)
 * - hasSelection = false, nodeId = X -> No node selected (invalid, X is undefined)
 * 
 * Usage pattern:
 * @code
 * auto result = getSelectedNodeId();
 * if (result.hasSelection) {
 *     // Use result.nodeId safely
 * }
 * @endcode
 */
struct SelectedNodeResult
{
    bool hasSelection{false};  ///< True if exactly one node is selected
    NodeId nodeId{0};          ///< The selected node's ID (only valid when hasSelection is true)
};

/**
 * @class MainWindow
 * @brief Main application window providing the visual node programming interface
 * 
 * MainWindow is the central hub of the CVDev application. It provides:
 * - Multi-tabbed interface for working with multiple flow graphs simultaneously
 * - Property browser for configuring selected nodes
 * - Node category browser for discovering available node types
 * - Node list showing all nodes in the current scene
 * - Full undo/redo support via Qt's QUndoStack
 * - File operations (new, open, save, save as)
 * - Plugin loading for extensible node types
 * 
 * Architecture:
 * The class uses a dynamic query pattern instead of caching pointers to the current
 * scene's components. Helper methods (getCurrentView(), getCurrentScene(), getCurrentModel())
 * query the active tab on-demand, preventing stale pointer issues when tabs are switched
 * or closed. This follows the single source of truth principle - the tab widget owns
 * the scene hierarchy.
 */
class CVDEVSHAREDLIB_EXPORT MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the main application window
     * @param parent Parent widget (typically nullptr for top-level window)
     * 
     * Initializes the UI, loads plugins, creates the first empty scene,
     * and sets up all signal-slot connections.
     */
    MainWindow( QWidget *parent = nullptr );
    
    /**
     * @brief Destructor - ensures proper cleanup of all scenes and resources
     * 
     * Deletes all scenes in the proper order (view -> scene -> model) to avoid
     * dangling pointers and memory leaks.
     */
    ~MainWindow();
    
    /**
     * @brief Enables or disables all nodes in the current scene
     * @param enable true to enable all nodes, false to disable them
     * 
     * This is useful for quickly activating/deactivating an entire flow graph
     * without having to manually toggle each node.
     */
    void enable_all_nodes(bool);

private Q_SLOTS:
    // Property editing slots
    
    /**
     * @brief Handles property change requests from nodes (e.g., embedded widgets).
     * Creates a PropertyChangeCommand and pushes it to the undo stack.
     * This is the central point for all property changes that should be undoable.
     */
    void handlePropertyChangeRequest(const QString& propertyId, const QVariant& oldValue, const QVariant& newValue);
    
    /**
     * @brief Handles selection requests from unselected nodes.
     * When a user clicks an embedded widget on an unselected node, the node
     * requests to be selected first before the interaction proceeds.
     */
    void handleSelectionRequest();
    
    /**
     * @brief Handles property value changes from the property browser
     * @param property The Qt property that changed
     * @param value The new value for the property
     * 
     * Creates a PropertyChangeCommand and pushes it to the undo stack,
     * enabling undo/redo for property modifications.
     */
    void editorPropertyChanged( QtProperty *property, const QVariant &value );
    
    /**
     * @brief Receives property change notifications from node delegate models
     * @param prop Shared pointer to the property that changed
     * 
     * Updates the property browser to reflect changes made programmatically
     * by the node itself (not through the UI).
     */
    void nodePropertyChanged( std::shared_ptr<Property> );
    
    // Node lifecycle slots
    
    /**
     * @brief Called when a new node is created in the scene
     * @param nodeId Unique identifier for the newly created node
     * 
     * Performs post-creation initialization:
     * - Calls late_constructor() on the node's delegate model
     * - Adds the node to the tree view
     * - Marks the scene as modified
     */
    void nodeCreated( NodeId nodeId );
    
    /**
     * @brief Called when a node is deleted from the scene
     * @param nodeId Unique identifier for the deleted node
     * 
     * Performs cleanup:
     * - Removes the node from the tree view
     * - Disconnects signal connections if node was selected
     * - Clears property browser if needed
     */
    void nodeDeleted( NodeId nodeId );
    
    /**
     * @brief Handles selection changes in the scene
     * 
     * Updates the property browser to show properties of the selected node.
     * Only handles single selection - clears browser for multiple/no selection.
     */
    void nodeInSceneSelectionChanged();
    
    // Node tree interaction slots
    
    /**
     * @brief Handles single click on node tree item
     * @param item The tree widget item that was clicked
     * @param column The column index (unused)
     * 
     * Selects the corresponding node in the graphics scene.
     */
    void nodeListClicked( QTreeWidgetItem *, int );
    
    /**
     * @brief Handles double click on node tree item
     * @param item The tree widget item that was double-clicked
     * @param column The column index (unused)
     * 
     * Centers the view on the corresponding node.
     */
    void nodeListDoubleClicked( QTreeWidgetItem *, int );
    /**
     * @brief Handles custom context menu requests for the node list tree view
     * @param pos Position within the tree view where the context menu was requested
     *
     * Shows Copy/Cut/Delete actions for selected node or group and delegates
     * the operations to the active view's handlers (so clipboard + undo work).
     */
    void nodeListContextMenuRequested(const QPoint &pos);
    
    /**
     * @brief Handles tab changes in the tab widget
     * @param index The index of the newly selected tab
     * 
     * Updates the node tree to show nodes from the newly active scene
     * and refreshes the property browser.
     */
    void tabPageChanged( int );
    
    /**
     * @brief Called when the scene is modified
     * 
     * Updates the tab title to show unsaved changes (adds * to title).
     */
    void nodeChanged();

    // File menu action slots
    
    void actionNew_slot();      ///< Creates a new empty flow scene in a new tab
    void actionSave_slot();     ///< Saves the current scene to its file
    void actionLoad_slot();     ///< Opens a file dialog to load a .flow file
    void actionQuit_slot();     ///< Quits the application
    void actionSaveAs_slot();   ///< Opens a save dialog to save with a new filename

    // View menu action slots
    
    void actionSceneOnly_slot();    ///< Hides all dock widgets, showing only the scene
    void actionAllPanels_slot();    ///< Shows all dock widgets

    // Grouping action slots
    
    /**
     * @brief Creates a group from currently selected nodes
     * 
     * Prompts for a group name and creates a new visual group containing
     * all currently selected nodes in the active scene.
     */
    void actionGroupSelectedNodes_slot();
    
    /**
     * @brief Dissolves the group containing the selected node(s)
     * 
     * Removes the grouping but keeps all nodes in the scene.
     */
    void actionUngroupSelectedNodes_slot();
    
    /**
     * @brief Renames the group containing the selected node
     * 
     * Prompts for a new name and updates the group label.
     */
    void actionRenameGroup_slot();
    
    /**
     * @brief Changes the color of the group containing the selected node
     * 
     * Opens a color picker dialog and updates the group background color.
     */
    void actionChangeGroupColor_slot();
    void actionZoomReset_slot();    ///< Resets the view transformation to default

    // Edit menu action slots
    
    void actionUndo_slot();     ///< Undoes the last operation
    void actionRedo_slot();     ///< Redoes the last undone operation

    // Node menu action slots
    
    void actionDisableAll_slot();   ///< Disables all nodes in current scene
    void actionEnableAll_slot();    ///< Enables all nodes in current scene

    // Settings menu action slots
    
    /**
     * @brief Toggles snap-to-grid functionality
     * @param checked true to enable snap-to-grid, false to disable
     * 
     * Applies to all scenes, not just the current one.
     */
    void actionSnapToGrid_slot(bool);
    
    void actionLoadPlugin_slot();   ///< Opens dialog to load additional plugin libraries
    
    /**
     * @brief Toggles focus view mode
     * @param checked true to enter focus mode, false to exit
     * 
     * Focus mode hides non-embedded nodes and connections, showing only
     * widgets/displays for presentation purposes.
     */
    void actionFocusView_slot(bool);
    
    void actionFullScreen_slot(bool);   ///< Toggles fullscreen mode
    void actionAbout_slot();            ///< Shows about dialog

    // Group model notifications
    void groupCreated(GroupId groupId);
    void groupDissolved(GroupId groupId);

protected:
    /**
     * @brief Handles application close event
     * @param event The close event
     * 
     * Prompts to save unsaved changes before closing.
     */
    void closeEvent(QCloseEvent *);

private:
    // UI setup methods
    
    /**
     * @brief Initializes the property browser dock widget
     * 
     * Creates the QtPropertyBrowser widget and configures it for displaying
     * and editing node properties.
     */
    void setupPropertyBrowserDockingWidget();

    /**
     * @brief Initializes the node categories dock widget
     * 
     * Sets up the tree view showing available node types grouped by category.
     */
    void setupNodeCategoriesDockingWidget();
    
    /**
     * @brief Refreshes the node categories tree view
     * 
     * Populates the tree with all registered node types from the plugin registry.
     */
    void updateNodeCategoriesDockingWidget();

    /**
     * @brief Initializes the node list dock widget
     * 
     * Sets up the tree view showing all nodes in the current scene.
     */
    void setupNodeListDockingWidget();
    
    // Scene management methods
    
    /**
     * @brief Loads a flow scene from a file
     * @param filename Path to the .flow file to load
     * 
     * If the current scene is empty, loads into it. Otherwise creates a new tab.
     */
    void loadScene(QString &);
    
    /**
     * @brief Creates a new flow scene
     * @param _filename Filename for the scene (empty for "Untitle.flow")
     * @param pDataModelRegistry Shared registry of available node types
     * 
     * Creates the complete scene hierarchy:
     * 1. PBDataFlowGraphModel - data model
     * 2. PBDataFlowGraphicsScene - graphics scene with undo stack
     * 3. PBFlowGraphicsView - view widget
     * 
     * The view is added to the tab widget and signal connections are established.
     */
    void createScene(QString const & _filename, std::shared_ptr<NodeDelegateModelRegistry> & pDataModelRegistry);
    
    /**
     * @brief Closes a scene tab
     * @param index Index of the tab to close
     * @return true if successfully closed, false if cancelled
     * 
     * Prompts to save unsaved changes. If this is the last tab, creates a new
     * empty scene (unless the application is shutting down).
     */
    bool closeScene( int );
    
    // Node tree management methods
    
    /**
     * @brief Adds a node to the node list tree view
     * @param nodeId Unique identifier of the node to add
     * 
     * Creates tree items for the node's model category (if not exists) and
     * the node itself, showing its caption and ID.
     */
    void addToNodeTree( NodeId nodeId );
    
    /**
     * @brief Removes a node from the node list tree view
     * @param nodeId Unique identifier of the node to remove
     */
    void removeFromNodeTree( NodeId nodeId );

    /**
     * @brief Adds a group to the node list tree view (under a "Groups" root)
     * @param groupId Unique identifier of the group to add
     */
    void addToGroupTree( GroupId groupId );

    /**
     * @brief Removes a group from the node list tree view
     * @param groupId Unique identifier of the group to remove
     */
    void removeFromGroupTree( GroupId groupId );

    // Settings persistence methods
    
    /**
     * @brief Loads application settings from INI file
     * 
     * Restores window state, dock widget visibility, and the last opened scene.
     */
    void loadSettings();
    
    /**
     * @brief Saves application settings to INI file
     * 
     * Persists window state, dock widget visibility, and current scene path.
     */
    void saveSettings();
    
    /**
     * @brief Recenters the current view to show all nodes centered
     * 
     * Calculates the bounding box of all nodes and centers the viewport on it.
     * Used after layout changes (e.g., showing/hiding dock widgets).
     */
    void recenterCurrentView();
    
    // Recent files management methods
    
    /**
     * @brief Updates the recent files list and rebuilds the menu
     * @param filename Path to the file that was just loaded
     * 
     * Adds the file to the top of the recent list, removes duplicates,
     * maintains max of 10 entries, and persists to settings.
     */
    void updateRecentFiles(const QString& filename);
    
    /**
     * @brief Adds a file to the recent files list
     * 
     * Updates the recent files list by moving the file to the top (most recent),
     * limiting the list to 10 items, saving settings, and updating the menu display.
     * 
     * @param filename The absolute path to the file to add
     */
    void addToRecentFiles(const QString& filename);
    
    /**
     * @brief Populates the recent files menu with current list
     * 
     * Creates QActions for each recent file and connects to onRecentFileTriggered.
     */
    void createRecentFilesMenu();
    
    /**
     * @brief Slot for recent file menu actions
     * 
     * Extracts the filename from the triggered action and calls loadScene.
     */
    void onRecentFileTriggered();
    
    // Helper methods for dynamic querying (avoiding stale cached pointers)
    
    /**
     * @brief Gets the currently selected node (if exactly one is selected)
     * @return SelectedNodeResult with hasSelection=true and valid nodeId if one node selected,
     *         or hasSelection=false if no selection or multiple selection
     * 
     * Design: Returns a struct to eliminate ambiguity. Since NodeId is unsigned int,
     * NodeId(0) is valid and cannot be used as a sentinel. The struct's hasSelection flag
     * provides unambiguous validity checking.
     * 
     * Example:
     * @code
     * auto result = getSelectedNodeId();
     * if (result.hasSelection) {
     *     // result.nodeId is valid, could be 0 or any other value
     *     doSomethingWith(result.nodeId);
     * }
     * @endcode
     */
    SelectedNodeResult getSelectedNodeId() const;
    
    /**
     * @brief Gets the delegate model for the currently selected node
     * @return Pointer to PBNodeDelegateModel, or nullptr if invalid selection
     * 
     * Design: Combines getCurrentModel() and getSelectedNodeId() for convenience.
     * Returns nullptr for safety if no valid single selection exists.
     */
    PBNodeDelegateModel* getSelectedNodeDelegateModel() const;
    
    /**
     * @brief Gets the view widget for the currently active tab
     * @return Pointer to PBFlowGraphicsView, or nullptr if no valid tab
     * 
     * Design: Queries ui->mpTabWidget->currentWidget() on demand. This is the
     * single source of truth for which scene is active. No caching means no
     * synchronization issues when tabs are switched or closed.
     */
    PBFlowGraphicsView* getCurrentView() const;
    
    /**
     * @brief Gets the graphics scene for the currently active tab
     * @return Pointer to PBDataFlowGraphicsScene, or nullptr if no valid view
     * 
     * Design: Retrieves scene from current view. Each view owns exactly one scene.
     */
    PBDataFlowGraphicsScene* getCurrentScene() const;
    
    /**
     * @brief Gets the data model for the currently active tab
     * @return Pointer to PBDataFlowGraphModel, or nullptr if no valid scene
     * 
     * Design: Retrieves model from current scene. Each scene references exactly
     * one model. The model outlives the scene (deleted last during cleanup).
     */
    PBDataFlowGraphModel* getCurrentModel() const;

    // Member variables
    
    Ui::MainWindow *ui;  ///< Auto-generated UI components
    
    /// Shared registry of all available node types (from plugins and built-ins)
    std::shared_ptr<NodeDelegateModelRegistry> mpDelegateModelRegistry;
    
    /// List of all open scenes (one per tab)
    /// Design: std::list allows stable iterators during insertion/deletion
    std::list<struct SceneProperty> mlSceneProperty;
    
    /// Iterator pointing to the currently active scene
    /// Updated in tabPageChanged() and closeScene()
    std::list<struct SceneProperty>::iterator mitSceneProperty;

    /// Flag to prevent modifications during application shutdown
    /// Used in closeScene() to skip creating a new empty scene when closing the last tab
    bool mbClossingApp{ false };
    
    /// Flag to prevent infinite loops during undo/redo operations
    /// Property browser updates trigger editorPropertyChanged(), but during undo/redo
    /// we don't want to create new undo commands for those programmatic changes
    bool mbApplyingUndoRedo{ false };

    // Node tree management maps
    
    /// Maps node model category names to their tree widget items (e.g., "Image Processing")
    QMap< QString, QTreeWidgetItem* > mMapModelCategoryToNodeTreeWidgetItem;
    
    /// Maps node model type names to their tree widget items (e.g., "GaussianBlurModel")
    QMap< QString, QTreeWidgetItem* > mMapModelNameToNodeTreeWidgetItem;
    
    /// Maps node IDs to their instance tree widget items (individual nodes in the scene)
    QMap< NodeId, QTreeWidgetItem* > mMapNodeIdToNodeTreeWidgetItem;
    
    /// Maps node IDs to their delegate model pointers (for quick access)
    QMap< NodeId, PBNodeDelegateModel*> mMapNodeIdToNodeDelegateModel;
    
    /// Maps node IDs to their graphics object pointers (for visual updates)
    QMap< NodeId, NodeGraphicsObject* > mMapNodeIdToNodeGraphicsObject;

    /// Maps group IDs to their tree widget items (Groups section in node list)
    QMap< GroupId, QTreeWidgetItem* > mMapGroupIdToNodeTreeWidgetItem;

    /// Root item for groups in the node list tree view
    QTreeWidgetItem *mGroupRootItem{nullptr};

    // Property browser components
    
    /// Property manager for creating variant properties (int, double, string, etc.)
    class QtVariantPropertyManager * mpVariantManager;
    
    /// Tree-based property browser widget for displaying node properties
    class QtTreePropertyBrowser * mpPropertyEditor;
    
    /// Maps Qt properties to node property IDs (for reverse lookup)
    QMap< QtProperty *, QString > mMapQtPropertyToPropertyId;
    
    /// Maps node property IDs to Qt properties (for updates)
    QMap< QString, QtProperty * > mMapPropertyIdToQtProperty;
    
    /// Remembers which property groups were expanded in the browser
    QMap< QString, bool > mMapPropertyIdToExpanded;
    
    /// List of group property managers (for grouping related properties)
    /// Must be kept alive for the lifetime of the properties
    QList< QtGroupPropertyManager * > mGroupPropertyManagerList;
    
    /// List of loaded plugin libraries
    /// Kept alive to prevent unloading plugin code while in use
    QList< QPluginLoader * > mPluginsList;

    QString msSettingFilename;  ///< Path to the settings INI file
    const QString msProgramName{ "CVDev" };  ///< Application name

    /// Recent files list (max 10 entries)
    QStringList msRecentFiles;
    
    /// Maximum number of recent files to track
    const int miMaxRecentFiles{10};

    // Property browser helper methods
    
    /**
     * @brief Updates the expanded/collapsed state of properties in the browser
     * 
     * Restores the expansion state from mMapPropertyIdToExpanded after rebuilding
     * the property browser.
     */
    void updatePropertyExpandState();
    
    /**
     * @brief Adds a property to the property browser
     * @param property The variant property to add
     * @param prop_id Unique identifier for the property
     * @param sub_text Optional group text (creates a group if provided)
     * 
     * If sub_text is provided, creates a group property and adds the property as a child.
     */
    void addProperty( QtVariantProperty *property, const QString & prop_id, const QString & sub_text );
    
    /**
     * @brief Clears all properties from the property browser
     * 
     * Deletes all property managers and clears all property maps.
     * Called when selection changes or nodes are deleted.
     */
    void clearPropertyBrowser();
};
