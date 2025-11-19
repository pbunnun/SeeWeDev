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
 * @file PBDataFlowGraphicsScene.hpp
 * @brief Custom graphics scene for interactive dataflow graph rendering.
 *
 * This file defines the PBDataFlowGraphicsScene class, which extends QtNodes'
 * DataFlowGraphicsScene with custom features including checkbox interaction,
 * snap-to-grid, and custom node geometry.
 *
 * **Key Features:**
 * - **Checkbox Interaction:** Enable/disable, lock, and minimize controls
 * - **Snap-to-Grid:** Align nodes to grid for organized layouts
 * - **Custom Geometry:** PBNodeGeometry for node sizing
 * - **Custom Painting:** PBNodePainter for node rendering
 * - **Mouse Handling:** Custom event processing for checkboxes
 *
 * **Checkbox Functionality:**
 * @code
 * +--------------------------------+
 * | [✓][🔒][−] Node Caption       | <- Three checkboxes in header
 * +--------------------------------+
 * 
 * Enable (✓):    Toggle node processing
 * Lock (🔒):     Prevent node movement
 * Minimize (−):  Collapse to header only
 * @endcode
 *
 * **Common Use Cases:**
 * - Render dataflow graphs with custom node appearance
 * - Handle user interactions with node checkboxes
 * - Provide snap-to-grid for neat graph layouts
 * - Calculate checkbox hit regions for clicks
 *
 * **Integration Pattern:**
 * @code
 * // Setup scene with model and custom features
 * auto model = std::make_shared<PBDataFlowGraphModel>(registry);
 * auto* scene = new PBDataFlowGraphicsScene(*model);
 * 
 * // Enable snap-to-grid
 * scene->setSnapToGrid(true);
 * 
 * // Install custom geometry
 * scene->installCustomGeometry();
 * 
 * // Use with view
 * auto* view = new PBFlowGraphicsView(scene);
 * @endcode
 *
 * **Mouse Event Flow:**
 * @code
 * // User clicks on node
 * mousePressEvent()
 *   → Check if click in checkbox region
 *   → getEnableCheckboxRect() / getLockCheckboxRect() / getMinimizeCheckboxRect()
 *   → Toggle checkbox state
 *   → Update node appearance
 *   → Trigger node update
 * @endcode
 *
 * **Snap-to-Grid Behavior:**
 * @code
 * // Node movement with snap enabled
 * mouseMoveEvent()
 *   → Calculate new position
 *   → if (isSnapToGrid())
 *       → Round to nearest grid point
 *       → position = (pos / gridSize) * gridSize
 *   → Update node position
 * @endcode
 *
 * @see QtNodes::DataFlowGraphicsScene for base scene functionality
 * @see PBNodePainter for custom node rendering
 * @see PBNodeGeometry for node size calculations
 */

#pragma once

#include "CVDevLibrary.hpp"
#include <QtNodes/internal/DataFlowGraphicsScene.hpp>
#include "PBNodeGroup.hpp"
#include "PBNodeGroupGraphicsItem.hpp"
#include <map>

namespace QtNodes {
    class DataFlowGraphModel;
}

class QGraphicsSceneMouseEvent;

/**
 * @class PBDataFlowGraphicsScene
 * @brief Custom graphics scene with checkbox interaction and snap-to-grid.
 *
 * Extends QtNodes::DataFlowGraphicsScene to provide interactive checkboxes
 * for node control, snap-to-grid positioning, and custom geometry/painting.
 *
 * **Core Functionality:**
 * - **Checkbox Hit Testing:** Calculate click regions for enable/lock/minimize
 * - **Mouse Event Handling:** Process clicks on checkboxes
 * - **Snap-to-Grid:** Align nodes to configurable grid
 * - **Custom Rendering:** Integrate PBNodePainter and PBNodeGeometry
 *
 * **Inheritance:**
 * @code
 * QObject + QGraphicsScene
 *   └── BasicGraphicsScene
 *         └── DataFlowGraphicsScene
 *               └── PBDataFlowGraphicsScene
 * @endcode
 *
 * **Typical Usage:**
 * @code
 * // Create scene with model
 * auto model = std::make_shared<PBDataFlowGraphModel>(registry);
 * auto* scene = new PBDataFlowGraphicsScene(*model, parent);
 * 
 * // Configure features
 * scene->setSnapToGrid(true);        // Enable grid snapping
 * scene->installCustomGeometry();     // Use custom node sizing
 * 
 * // Connect to view
 * auto* view = new PBFlowGraphicsView(scene);
 * view->setScene(scene);
 * @endcode
 *
 * **Checkbox Layout:**
 * @code
 * Node Header:
 * +--------------------------------+
 * | [✓][🔒][−] Caption            |
 * +--------------------------------+
 *   ^   ^   ^
 *   |   |   |
 *   |   |   +-- Minimize (getMinimizeCheckboxRect)
 *   |   +------ Lock     (getLockCheckboxRect)
 *   +---------- Enable   (getEnableCheckboxRect)
 * 
 * Each checkbox: 8×8 pixels with 4px margin
 * @endcode
 *
 * **Snap-to-Grid Example:**
 * @code
 * scene->setSnapToGrid(true);  // Enable snapping
 * 
 * // When user drags node:
 * // Without snap: position = (123.7, 456.2)
 * // With snap:    position = (120.0, 450.0)  // Rounded to 15px grid
 * @endcode
 *
 * @see DataFlowGraphicsScene for base scene functionality
 * @see PBNodePainter for rendering checkboxes
 * @see PBNodeGeometry for node layout
 */
class CVDEVSHAREDLIB_EXPORT PBDataFlowGraphicsScene : public QtNodes::DataFlowGraphicsScene
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a custom graphics scene for dataflow graphs.
     *
     * Initializes the scene with a graph model and sets up custom rendering.
     *
     * @param graphModel Reference to the DataFlowGraphModel containing node data
     * @param parent Optional parent QObject for Qt ownership (default: nullptr)
     *
     * **Example:**
     * @code
     * auto model = std::make_shared<PBDataFlowGraphModel>(registry);
     * auto* scene = new PBDataFlowGraphicsScene(*model, mainWindow);
     * 
     * // Configure
     * scene->setSnapToGrid(true);
     * scene->installCustomGeometry();
     * 
     * // Use with view
     * auto* view = new PBFlowGraphicsView(scene);
     * setCentralWidget(view);
     * @endcode
     */
    PBDataFlowGraphicsScene(QtNodes::DataFlowGraphModel &graphModel, QObject *parent = nullptr);

    /**
     * @brief Installs custom node geometry calculator.
     *
     * Replaces the default geometry with PBNodeGeometry for custom node
     * sizing that accounts for checkboxes and embedded widgets.
     *
     * **Example:**
     * @code
     * auto* scene = new PBDataFlowGraphicsScene(*model);
     * scene->installCustomGeometry();  // Use PBNodeGeometry
     * 
     * // Nodes now sized with custom logic:
     * // - Checkbox area in header
     * // - Embedded widget dimensions
     * // - Port label widths
     * @endcode
     *
     * @note Should be called once during scene initialization
     * @see PBNodeGeometry for sizing calculations
     */
    void installCustomGeometry();

    /**
     * @brief Returns the bounding rectangle for the enable checkbox.
     *
     * Calculates the clickable region for the enable/disable checkbox
     * in the node's header area.
     *
     * @param nodeId Unique identifier of the node
     * @return QRectF Rectangle in scene coordinates
     *
     * **Checkbox Position:**
     * @code
     * // Top-left corner of node header
     * QRectF rect = getEnableCheckboxRect(nodeId);
     * // rect.topLeft() ≈ node position + (4px, 4px)
     * // rect.size() = (8px, 8px)
     * @endcode
     *
     * **Usage in Mouse Handler:**
     * @code
     * void PBDataFlowGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
     *     QPointF clickPos = event->scenePos();
     *     
     *     for (NodeId id : allNodeIds) {
     *         if (getEnableCheckboxRect(id).contains(clickPos)) {
     *             toggleNodeEnabled(id);
     *             break;
     *         }
     *     }
     * }
     * @endcode
     *
     * @see CHECKBOX_SIZE for checkbox dimensions
     * @see CHECKBOX_MARGIN for spacing
     */
    QRectF getEnableCheckboxRect(QtNodes::NodeId nodeId) const;
    
    /**
     * @brief Returns the bounding rectangle for the lock checkbox.
     *
     * Calculates the clickable region for the position lock checkbox
     * in the node's header area.
     *
     * @param nodeId Unique identifier of the node
     * @return QRectF Rectangle in scene coordinates
     *
     * **Checkbox Position:**
     * @code
     * // Center of node header
     * // Positioned after enable checkbox with margin
     * QRectF rect = getLockCheckboxRect(nodeId);
     * @endcode
     *
     * **Lock Behavior:**
     * @code
     * if (getLockCheckboxRect(nodeId).contains(clickPos)) {
     *     bool locked = isNodeLocked(nodeId);
     *     setNodeLocked(nodeId, !locked);
     *     
     *     if (!locked) {
     *         // Prevent node movement
     *         nodeGraphicsObject->setFlag(ItemIsMovable, false);
     *     }
     * }
     * @endcode
     */
    QRectF getLockCheckboxRect(QtNodes::NodeId nodeId) const;
    
    /**
     * @brief Returns the bounding rectangle for the minimize checkbox.
     *
     * Calculates the clickable region for the minimize/expand checkbox
     * in the node's header area.
     *
     * @param nodeId Unique identifier of the node
     * @return QRectF Rectangle in scene coordinates
     *
     * **Checkbox Position:**
     * @code
     * // Right side of node header
     * // Positioned after lock checkbox with margin
     * QRectF rect = getMinimizeCheckboxRect(nodeId);
     * @endcode
     *
     * **Minimize Behavior:**
     * @code
     * if (getMinimizeCheckboxRect(nodeId).contains(clickPos)) {
     *     bool minimized = isNodeMinimized(nodeId);
     *     setNodeMinimized(nodeId, !minimized);
     *     
     *     if (minimized) {
     *         // Expand: Show ports and widget
     *         geometry->recomputeSize(nodeId);
     *     } else {
     *         // Minimize: Show only header
     *         geometry->recomputeSize(nodeId);
     *     }
     * }
     * @endcode
     */
    QRectF getMinimizeCheckboxRect(QtNodes::NodeId nodeId) const;

    /**
     * @brief Enables or disables snap-to-grid for node positioning.
     *
     * @param snap True to enable grid snapping, false to disable
     *
     * **Example:**
     * @code
     * // Enable snap for organized layouts
     * scene->setSnapToGrid(true);
     * 
     * // Disable for free positioning
     * scene->setSnapToGrid(false);
     * @endcode
     *
     * @see isSnapToGrid() to query current state
     * @see gridSize() for grid spacing
     */
    void setSnapToGrid(bool snap) { mbSnapToGrid = snap; }
    
    /**
     * @brief Checks if snap-to-grid is enabled.
     *
     * @return bool True if snapping is enabled, false otherwise
     *
     * **Example:**
     * @code
     * if (scene->isSnapToGrid()) {
     *     // Round position to grid
     *     int grid = scene->gridSize();
     *     QPointF snapped(
     *         qRound(pos.x() / grid) * grid,
     *         qRound(pos.y() / grid) * grid
     *     );
     *     node->setPosition(snapped);
     * }
     * @endcode
     */
    bool isSnapToGrid() const { return mbSnapToGrid; }
    
    /**
     * @brief Returns the grid size for snap-to-grid.
     *
     * @return int Grid spacing in pixels (default: 15)
     *
     * **Example:**
     * @code
     * int grid = scene->gridSize();  // 15
     * 
     * // Snap position to grid
     * QPointF snapped(
     *     qRound(pos.x() / grid) * grid,
     *     qRound(pos.y() / grid) * grid
     * );
     * @endcode
     */
    int gridSize() const { return miGridSize; }

    // ========== Node Grouping API ==========
    
    /**
     * @brief Updates visual representation of a group
     * @param groupId Group to update
     * 
     * Refreshes the group background graphics item based on current
     * node positions and group properties.
     */
    void updateGroupVisual(GroupId groupId);
    
    /**
     * @brief Updates all group visuals
     * 
     * Refreshes all group background graphics items. Called after
     * node movements or bulk changes.
     */
    void updateAllGroupVisuals();

    /**
     * @brief Gets the graphics item for a group
     * @param groupId Group identifier
     * @return PBNodeGroupGraphicsItem* Pointer to the group's graphics item, or nullptr if not found
     * 
     * Returns the graphics item that renders the group's visual background.
     * Can be used to query the group's bounding rectangle.
     */
    PBNodeGroupGraphicsItem* getGroupGraphicsItem(GroupId groupId) const;

protected:
    /**
     * @brief Handles mouse press events for checkbox interaction.
     *
     * Processes mouse clicks to detect checkbox activation and toggle
     * node states (enabled, locked, minimized).
     *
     * @param event Mouse press event containing position and button
     *
     * **Processing Flow:**
     * @code
     * 1. Get click position in scene coordinates
     * 2. Find node at click position
     * 3. Check if click in any checkbox rectangle
     * 4. Toggle appropriate checkbox state
     * 5. Update node visual state
     * 6. Trigger repaint
     * @endcode
     *
     * @note Base class handles node selection and dragging
     */
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    /**
     * @brief Handles mouse move events with snap-to-grid support.
     *
     * Processes mouse movement during node dragging, applying snap-to-grid
     * if enabled.
     *
     * @param event Mouse move event containing position
     *
     * **Snap-to-Grid Application:**
     * @code
     * if (isSnapToGrid() && isDraggingNode) {
     *     QPointF pos = event->scenePos();
     *     int grid = gridSize();
     *     
     *     QPointF snapped(
     *         qRound(pos.x() / grid) * grid,
     *         qRound(pos.y() / grid) * grid
     *     );
     *     
     *     node->setPosition(snapped);
     * }
     * @endcode
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

    /**
     * @brief Handles mouse release events after dragging.
     *
     * Processes mouse button release, finalizing node position with
     * final snap-to-grid application if enabled.
     *
     * @param event Mouse release event containing position
     *
     * @note Ensures final position is grid-aligned after drag
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    /**
     * @brief Handles context menu requests in the scene
     * @param event Context menu event containing position
     * 
     * Checks if a group is at the context menu position and delegates
     * to the group's context menu. Otherwise, shows the default scene menu.
     */
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private Q_SLOTS:
    /**
     * @brief Handles group dragging by moving all member nodes
     * @param groupId ID of the group being moved
     * @param delta Movement delta in scene coordinates
     * 
     * Moves all nodes in the group by the specified delta, maintaining
     * their relative positions.
     */
    void onGroupMoved(GroupId groupId, QPointF delta);
    
    /**
     * @brief Handles toggling group minimize state
     * @param groupId ID of the group to toggle
     * 
     * Toggles the minimized state and updates node visibility.
     */
    void onToggleGroupMinimize(GroupId groupId);
    
    /**
     * @brief Handles ungroup request from context menu
     * @param groupId ID of the group to dissolve
     */
    void onUngroupRequested(GroupId groupId);
    
    /**
     * @brief Handles rename request from context menu
     * @param groupId ID of the group to rename
     */
    void onRenameRequested(GroupId groupId);
    
    /**
     * @brief Handles change color request from context menu
     * @param groupId ID of the group to recolor
     */
    void onChangeColorRequested(GroupId groupId);

private:
    /**
     * @brief Checkbox size in pixels.
     *
     * Defines the width and height of each checkbox.
     * Value: 8.0 pixels (matches resize handle size)
     */
    static constexpr double CHECKBOX_SIZE = 8.0;

    /**
     * @brief Margin around checkboxes in pixels.
     *
     * Defines spacing between checkboxes and from node edges.
     * Value: 4.0 pixels
     */
    static constexpr double CHECKBOX_MARGIN = 4.0;
    
    /**
     * @brief Snap-to-grid enabled flag.
     */
    bool mbSnapToGrid{false};

    /**
     * @brief Grid size in pixels for snap-to-grid.
     *
     * Default: 15 pixels
     */
    int miGridSize{15};
    
    /**
     * @brief Map of group graphics items
     */
    std::map<GroupId, PBNodeGroupGraphicsItem*> mGroupItems;
};
