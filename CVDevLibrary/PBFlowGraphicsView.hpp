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
 * @file PBFlowGraphicsView.hpp
 * @brief Custom graphics view for node-based dataflow graph visualization.
 *
 * This file defines the PBFlowGraphicsView class, which extends QtNodes::GraphicsView
 * with custom interaction features for the CVDev visual programming environment.
 *
 * **Key Features:**
 * - **Node Selection:** Multi-node selection and query
 * - **View Navigation:** Center on nodes or coordinates
 * - **Drag & Drop:** Support for node creation from palette
 * - **Context Menus:** Custom right-click menu support
 * - **Connection Visibility:** Show/hide specific connections
 * - **Keyboard Shortcuts:** Custom key handling for graph operations
 *
 * **Enhancements Over Base Class:**
 * The base QtNodes::GraphicsView provides basic graph viewing. PBFlowGraphicsView adds:
 * - selectedNodes() for querying current selection
 * - center_on() for programmatic view navigation
 * - getGraphicsObject() for direct node access
 * - clearSelection() for selection management
 * - showConnections() for connection visibility control
 * - Custom event handlers for enhanced interaction
 *
 * **Common Use Cases:**
 * - Interactive graph editing in main window
 * - Node palette drag-and-drop operations
 * - Programmatic view navigation (zoom to node, center on area)
 * - Custom context menu operations
 * - Connection highlighting and filtering
 * - Keyboard-driven graph manipulation
 *
 * **Integration Pattern:**
 * @code
 * // Setup in main window
 * auto* scene = new QtNodes::DataFlowGraphicsScene(graphModel);
 * auto* view = new PBFlowGraphicsView(scene);
 * 
 * // Navigation
 * view->center_on(nodeId);
 * 
 * // Selection
 * auto selected = view->selectedNodes();
 * for (NodeId id : selected) {
 *     // Process selected nodes
 * }
 * @endcode
 *
 * **Event Handling:**
 * - Context menu events for custom menus
 * - Drag/drop events for node creation from palette
 * - Key press events for shortcuts (delete, copy, paste, etc.)
 *
 * @see QtNodes::GraphicsView for base class
 * @see QtNodes::DataFlowGraphicsScene for scene management
 * @see PBNodePainter for node rendering
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QtNodes/GraphicsView>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/Definitions>
#include <QtNodes/DataFlowGraphicsScene>
#include <QDragMoveEvent>
#include <unordered_set>

using QtNodes::NodeId;
using QtNodes::NodeGraphicsObject;

// - center_on() needs to work with NodeId
// - Custom clipboard operations (cut/copy/paste) need reimplementation

/**
 * @class PBFlowGraphicsView
 * @brief Enhanced graphics view for interactive node graph visualization and editing.
 *
 * Extends QtNodes::GraphicsView with custom features for the CVDev visual programming
 * environment, including improved node selection, view navigation, drag-and-drop,
 * and custom event handling.
 *
 * **Core Functionality:**
 * - **Selection Management:** Query and clear node selections
 * - **View Navigation:** Center viewport on nodes or coordinates
 * - **Node Access:** Retrieve graphics objects for nodes
 * - **Connection Control:** Show/hide specific connections
 * - **Event Processing:** Custom context menus, drag-drop, keyboard
 *
 * **Inheritance:**
 * @code
 * QGraphicsView
 *   └── QtNodes::GraphicsView
 *         └── PBFlowGraphicsView
 * @endcode
 *
 * **Typical Usage:**
 * @code
 * // Create view with scene
 * auto* scene = new QtNodes::DataFlowGraphicsScene(model);
 * auto* view = new PBFlowGraphicsView(scene, parentWidget);
 * 
 * // Query selected nodes
 * std::vector<NodeId> selected = view->selectedNodes();
 * 
 * // Navigate to specific node
 * view->center_on(targetNodeId);
 * 
 * // Access node graphics
 * NodeGraphicsObject* nodeGfx = view->getGraphicsObject(nodeId);
 * 
 * // Control connection visibility
 * std::unordered_set<ConnectionId> connections = {...};
 * view->showConnections(connections, true);  // Show
 * view->showConnections(connections, false); // Hide
 * @endcode
 *
 * **Drag-and-Drop Support:**
 * The view handles drag-and-drop of node types from the palette:
 * @code
 * // User drags node type from PBTreeWidget palette
 * // 1. dragMoveEvent() validates drop location
 * // 2. dropEvent() creates node at drop position
 * // 3. New node appears in graph at mouse position
 * @endcode
 *
 * **Context Menu Integration:**
 * @code
 * void PBFlowGraphicsView::contextMenuEvent(QContextMenuEvent* event) {
 *     // Custom context menu for:
 *     // - Copy/paste operations
 *     // - Delete nodes
 *     // - Node-specific actions
 * }
 * @endcode
 *
 * **Keyboard Shortcuts:**
 * @code
 * void PBFlowGraphicsView::keyPressEvent(QKeyEvent* event) {
 *     // Common shortcuts:
 *     // - Delete: Remove selected nodes
 *     // - Ctrl+C: Copy selected nodes
 *     // - Ctrl+V: Paste nodes
 *     // - Ctrl+A: Select all
 * }
 * @endcode
 *
 * **View Navigation Examples:**
 * @code
 * // Center on specific node (e.g., after search)
 * NodeId foundNode = findNodeByName("ImageLoader");
 * view->center_on(foundNode);
 * 
 * // Center on coordinates (e.g., after loading graph)
 * QPointF graphCenter = calculateGraphCenter();
 * view->center_on(graphCenter);
 * @endcode
 *
 * **Selection Management:**
 * @code
 * // Get current selection
 * auto selected = view->selectedNodes();
 * qDebug() << "Selected" << selected.size() << "nodes";
 * 
 * // Clear selection
 * view->clearSelection();
 * 
 * // Process selected nodes
 * for (NodeId id : selected) {
 *     auto* gfxObj = view->getGraphicsObject(id);
 *     // Work with graphics object
 * }
 * @endcode
 *
 * **Connection Visibility Control:**
 * @code
 * // Highlight specific data flow path
 * std::unordered_set<ConnectionId> pathConnections;
 * // ... populate with connection IDs in path ...
 * 
 * // Show only these connections (hide others)
 * view->showConnections(pathConnections, true);
 * 
 * // Restore all connections
 * view->showConnections(pathConnections, false);
 * @endcode
 *
 * @see QtNodes::GraphicsView for base class functionality
 * @see QtNodes::DataFlowGraphicsScene for scene management
 * @see NodeGraphicsObject for node graphics representation
 * @see PBTreeWidget for node palette integration
 */
class CVDEVSHAREDLIB_EXPORT PBFlowGraphicsView : public QtNodes::GraphicsView
{
    Q_OBJECT
public:
    /**
     * @brief Constructs an enhanced graphics view for node graph visualization.
     *
     * Initializes the view with a QtNodes scene and parent widget. Sets up
     * custom drag-drop acceptance, event filters, and viewport configuration.
     *
     * @param scene Pointer to QtNodes::BasicGraphicsScene managing the graph model
     * @param parent Optional parent widget for Qt ownership hierarchy (default: nullptr)
     *
     * @note The scene should be initialized with a GraphModel before creating the view.
     * @note View takes ownership of viewport and event handling configuration.
     *
     * **Example:**
     * @code
     * // Create model and scene
     * auto model = std::make_shared<DataFlowGraphModel>();
     * auto* scene = new QtNodes::DataFlowGraphicsScene(*model, this);
     * 
     * // Create view
     * auto* view = new PBFlowGraphicsView(scene, mainWindow);
     * 
     * // Set as central widget
     * mainWindow->setCentralWidget(view);
     * @endcode
     */
    explicit PBFlowGraphicsView(QtNodes::BasicGraphicsScene *scene, QWidget *parent = nullptr);

    /**
     * @brief Retrieves all currently selected node IDs in the graph.
     *
     * Returns a vector of NodeId values for all selected nodes, suitable for
     * batch operations, clipboard actions, or property editing.
     *
     * @return std::vector<NodeId> Vector of selected node IDs (empty if no selection)
     *
     * **Example:**
     * @code
     * // Get selected nodes
     * std::vector<NodeId> selected = view->selectedNodes();
     * 
     * if (selected.empty()) {
     *     qDebug() << "No nodes selected";
     *     return;
     * }
     * 
     * // Process selection
     * for (NodeId id : selected) {
     *     auto* node = model->nodeData(id);
     *     // Operate on node data
     * }
     * @endcode
     *
     * **Use Cases:**
     * - Copy/cut/paste operations
     * - Bulk property changes
     * - Selection count display
     * - Clipboard serialization
     *
     * @see clearSelection() to deselect all nodes
     * @see getGraphicsObject() to access individual node graphics
     */
    std::vector<NodeId> selectedNodes();

    /**
     * @brief Centers the viewport on a specific node.
     *
     * Smoothly pans the view to center on the specified node, useful for
     * navigation after search results, error highlighting, or initial graph load.
     *
     * @param nodeId NodeId of the target node to center on
     *
     * @note If the node doesn't exist, the view remains unchanged.
     * @note Animation and zoom level depend on QGraphicsView settings.
     *
     * **Example:**
     * @code
     * // Center on specific node (e.g., after search)
     * NodeId targetId = findNodeByName("VideoCapture");
     * if (targetId != InvalidNodeId) {
     *     view->center_on(targetId);
     * }
     * @endcode
     *
     * @see center_on(const QPointF&) for coordinate-based centering
     */
    void center_on( NodeId nodeId );

    /**
     * @brief Centers the viewport on a specific scene coordinate.
     *
     * Pans the view to center on the given point in scene coordinates,
     * useful for graph layout centering or specific position focusing.
     *
     * @param center_pos Scene position (QPointF) to center on
     *
     * **Example:**
     * @code
     * // Center on graph bounding box center
     * QRectF bounds = scene->itemsBoundingRect();
     * QPointF center = bounds.center();
     * view->center_on(center);
     * @endcode
     *
     * @see center_on(NodeId) for node-based centering
     */
    void center_on( QPointF const & center_pos );

    /**
     * @brief Retrieves the graphics object for a specific node.
     *
     * Returns the NodeGraphicsObject that renders and handles events for
     * the specified node. Useful for custom rendering or direct graphics
     * manipulation.
     *
     * @param id NodeId of the target node
     * @return NodeGraphicsObject* Pointer to graphics object (nullptr if node not found)
     *
     * @warning The returned pointer becomes invalid if the node is deleted.
     * @note Do not delete the returned pointer - it's owned by the scene.
     *
     * **Example:**
     * @code
     * // Access node graphics for custom highlighting
     * NodeId nodeId = getSelectedNode();
     * NodeGraphicsObject* gfxObj = view->getGraphicsObject(nodeId);
     * 
     * if (gfxObj) {
     *     // Custom graphics operation
     *     gfxObj->setOpacity(0.5);  // Make semi-transparent
     *     gfxObj->update();          // Force redraw
     * }
     * @endcode
     *
     * **Use Cases:**
     * - Custom node highlighting
     * - Animation effects
     * - Direct graphics property access
     * - Bounding box queries
     *
     * @see selectedNodes() to get IDs of selected nodes
     */
    NodeGraphicsObject* getGraphicsObject(NodeId id);

    /**
     * @brief Clears all node and connection selections in the graph.
     *
     * Deselects all selected items (nodes and connections) in the scene,
     * typically used after operations or to reset selection state.
     *
     * **Example:**
     * @code
     * // Clear selection after copy operation
     * copySelectedNodes();
     * view->clearSelection();
     * @endcode
     *
     * @see selectedNodes() to query current selection before clearing
     */
    void clearSelection();

    /**
     * @brief Controls visibility of specific connections in the graph.
     *
     * Shows or hides the specified connections, useful for highlighting
     * data flow paths or debugging specific signal routes.
     *
     * @param connections Set of ConnectionId values to show/hide (passed by reference)
     * @param visible True to show connections, false to hide them
     *
     * **Example:**
     * @code
     * // Highlight a specific data flow path
     * std::unordered_set<QtNodes::ConnectionId> pathConnections;
     * pathConnections.insert(conn1);
     * pathConnections.insert(conn2);
     * 
     * // Show only these connections (hide others)
     * view->showConnections(pathConnections, true);
     * 
     * // Later, restore all visibility
     * view->showConnections(pathConnections, false);
     * @endcode
     *
     * **Use Cases:**
     * - Debugging signal flow
     * - Highlighting error paths
     * - Teaching/demonstration mode
     * - Connection filtering
     *
     * @note Hidden connections are not deleted, only visually hidden.
     * @see QtNodes::ConnectionGraphicsObject for connection graphics
     */
    void showConnections(std::unordered_set<QtNodes::ConnectionId>&, bool);

    /**
     * @brief Trigger copy action programmatically.
     *
     * This forwards to the scene's copy command so menu actions can delegate
     * copy behavior to the active view without creating duplicate shortcuts.
     */
    void triggerCopy();
    /** Trigger cut (copy then delete) via the scene's undo stack. */
    void triggerCut();
    /** Trigger paste at the view's paste position via the scene's undo stack. */
    void triggerPaste();
    /** Trigger delete of selected items via the scene's undo stack. */
    void triggerDelete();

protected:
    /**
     * @brief Handles context menu requests for the graph view.
     *
     * Displays a custom context menu with node/connection-specific actions
     * such as copy, paste, delete, and node property access.
     *
     * @param event Context menu event containing mouse position and modifiers
     *
     * @note Override this to customize context menu items.
     *
     * **Typical Menu Items:**
     * - Copy selected nodes
     * - Paste from clipboard
     * - Delete selected items
     * - Node properties
     * - Add comment/annotation
     *
     * @see QGraphicsView::contextMenuEvent() for base implementation
     */
    void contextMenuEvent( QContextMenuEvent *event ) override;

    /**
     * @brief Handles drag move events for node palette drag-and-drop.
     *
     * Validates drag operations when user drags node types from the palette
     * over the graph view, providing visual feedback for valid drop locations.
     *
     * @param event Drag move event containing mime data and position
     *
     * @note Accepts drag if mime data contains valid node type information.
     * @see dropEvent() for final drop handling
     * @see PBTreeWidget for node palette drag source
     */
    void dragMoveEvent( QDragMoveEvent *event ) override;

    /**
     * @brief Handles drop events to create new nodes from palette.
     *
     * Creates a new node of the specified type at the drop location when
     * user drops a node type from the palette onto the graph view.
     *
     * @param event Drop event containing node type mime data and position
     *
     * **Drop Process:**
     * 1. Extract node type from mime data
     * 2. Convert drop position to scene coordinates
     * 3. Create new node via GraphModel
     * 4. Position node at drop location
     *
     * **Example Flow:**
     * @code
     * // User drags "ImageLoader" from palette
     * // 1. dragMoveEvent validates drop location
     * // 2. dropEvent creates new ImageLoader node
     * // 3. Node appears at mouse position
     * @endcode
     *
     * @see dragMoveEvent() for drag validation
     * @see PBTreeWidget::mousePressEvent() for drag initiation
     */
    void dropEvent( QDropEvent *event ) override;

    /**
     * @brief Handles keyboard shortcuts for graph editing.
     *
     * Processes keyboard events for common graph operations:
     * - Delete: Remove selected nodes
     * - Ctrl+C: Copy selected nodes
     * - Ctrl+V: Paste nodes from clipboard
     * - Ctrl+X: Cut selected nodes
     * - Ctrl+A: Select all nodes
     * - Arrow keys: Navigate view
     *
     * @param event Key press event containing key code and modifiers
     *
     * **Example Shortcuts:**
     * @code
     * void PBFlowGraphicsView::keyPressEvent(QKeyEvent* event) {
     *     if (event->key() == Qt::Key_Delete) {
     *         deleteSelectedNodes();
     *         event->accept();
     *         return;
     *     }
     *     // ... other shortcuts ...
     *     GraphicsView::keyPressEvent(event);  // Base class handling
     * }
     * @endcode
     *
     * @note Unhandled keys are passed to base class implementation.
     * @see QGraphicsView::keyPressEvent() for base key handling
     */
    void keyPressEvent( QKeyEvent *event ) override;
    
private:
    /**
     * @brief Pointer to the base graphics scene managing items.
     *
     * Stores reference to QtNodes::BasicGraphicsScene for generic scene access.
     */
    QtNodes::BasicGraphicsScene * mpGraphicsScene {nullptr};

    /**
     * @brief Pointer to the dataflow-specific graphics scene.
     *
     * Stores reference to QtNodes::DataFlowGraphicsScene for dataflow operations,
     * connection management, and node creation from drag-drop.
     */
    QtNodes::DataFlowGraphicsScene* mpDataFlowGraphicsScene {nullptr};
};
