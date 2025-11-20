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
 * @file PBNodeGroupGraphicsItem.hpp
 * @brief Graphics item for rendering node group backgrounds
 * 
 * Provides visual representation of node groups as rounded rectangles
 * with labels, rendered behind grouped nodes in the scene.
 */

#pragma once

#include "CVDevLibrary.hpp"
#include "PBNodeGroup.hpp"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QObject>

/**
 * @class PBNodeGroupGraphicsItem
 * @brief Visual representation of a node group
 * 
 * Renders a rounded rectangle background with label for a group of nodes.
 * The item automatically calculates its bounding box from member node positions
 * and provides visual feedback for group membership.
 * 
 * **Features:**
 * - Rounded rectangle background with semi-transparent fill
 * - Group name label at top-left
 * - Padding around grouped nodes
 * - Z-order below nodes but above grid
 * - Updates on node movement
 * - Selectable and movable to drag entire group
 * 
 * **Usage Example:**
 * @code
 * PBNodeGroupGraphicsItem* item = new PBNodeGroupGraphicsItem(groupId);
 * item->setGroup(group);
 * item->updateBounds(nodePositions);
 * scene->addItem(item);
 * @endcode
 */
class CVDEVSHAREDLIB_EXPORT PBNodeGroupGraphicsItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT
    
public:
    /**
     * @brief Constructs a graphics item for a node group
     * @param groupId Unique identifier for the group
     * @param parent Optional parent item
     */
    explicit PBNodeGroupGraphicsItem(GroupId groupId, QGraphicsItem* parent = nullptr);

    /**
     * @brief Gets the group identifier
     * @return GroupId Associated group ID
     */
    GroupId groupId() const { return mGroupId; }
    
    /**
     * @brief Gets the set of node IDs in this group
     * @return Set of member node IDs
     */
    const std::set<NodeId>& nodeIds() const { return mNodeIds; }
    
    /**
     * @brief Sets the member node IDs for this group
     * @param nodeIds Set of node IDs belonging to this group
     */
    void setNodeIds(const std::set<NodeId>& nodeIds) { mNodeIds = nodeIds; }
    
    /**
     * @brief Checks if the group is currently minimized
     * @return true if minimized, false if expanded
     */
    bool isMinimized() const { return mMinimized; }
    
    /**
     * @brief Checks if the group position is locked
     * @return true if locked, false if unlocked
     */
    bool isLocked() const { return mLocked; }
    
    /**
     * @brief Sets the lock state
     * @param locked True to lock position, false to unlock
     */
    void setLocked(bool locked) { mLocked = locked; update(); }
    
    /**
     * @brief Gets the saved top-left position (used when minimized)
     * @return The saved anchor position
     */
    QPointF savedTopLeft() const { return mSavedTopLeft; }
    
    /**
     * @brief Sets the saved top-left position (used when minimized)
     * @param pos The new anchor position
     */
    void setSavedTopLeft(const QPointF &pos) { mSavedTopLeft = pos; }

    /**
     * @brief Updates visual properties from group data
     * @param group Source group with name and color
     * 
     * Updates the item's color, label text, and visual style
     * based on the group's properties.
     */
    void setGroup(const PBNodeGroup& group);

    /**
     * @brief Sets the label text color
     * @param color The color for the group label text
     */
    void setLabelColor(const QColor& color);

    /**
     * @brief Updates bounding rectangle from node positions
     * @param nodePositions Map of node IDs to their scene positions
     * @param nodeSizes Map of node IDs to their sizes (widths and heights)
     * 
     * Calculates the minimal bounding rectangle that encompasses all
     * member nodes with padding, and updates the item's geometry.
     */
    void updateBounds(const std::map<NodeId, QPointF>& nodePositions,
                      const std::map<NodeId, QSizeF>& nodeSizes);

    /**
     * @brief Horizontal padding around grouped nodes in pixels
     */
    static constexpr qreal PADDING_HORIZONTAL = 6.0;

    /**
     * @brief Vertical padding around grouped nodes in pixels
     */
    static constexpr qreal PADDING_VERTICAL = 2.0;
    
    /**
     * @brief Top padding for the group label area
     */
    static constexpr qreal LABEL_TOP_PADDING = 15.0;

    /**
     * @brief Corner radius for rounded rectangle
     */
    static constexpr qreal CORNER_RADIUS = 10.0;
    
    /**
     * @brief Size of the minimize button in top-left corner
     */
    static constexpr qreal MINIMIZE_BUTTON_SIZE = 16.0;
    
    /**
     * @brief Size of the lock button next to minimize button
     */
    static constexpr qreal LOCK_BUTTON_SIZE = 16.0;

Q_SIGNALS:
    /**
     * @brief Emitted when the group starts being dragged
     * @param groupId The ID of the group being moved
     */
    void groupMoveStarted(GroupId groupId);
    
    /**
     * @brief Emitted when the group is being dragged
     * @param groupId The ID of the group being moved
     * @param delta The movement delta in scene coordinates
     */
    void groupMoved(GroupId groupId, QPointF delta);
    
    /**
     * @brief Emitted when the group finishes being dragged
     * @param groupId The ID of the group being moved
     */
    void groupMoveFinished(GroupId groupId);
    
    /**
     * @brief Emitted when user double-clicks to toggle minimize
     * @param groupId The ID of the group to toggle
     */
    void toggleMinimizeRequested(GroupId groupId);
    
    /**
     * @brief Emitted when user clicks the lock button
     * @param groupId The ID of the group
     * @param locked The new lock state
     */
    void lockToggled(GroupId groupId, bool locked);
    
    /**
     * @brief Emitted when user requests to ungroup via context menu
     * @param groupId The ID of the group to dissolve
     */
    void ungroupRequested(GroupId groupId);
    
    /**
     * @brief Emitted when user requests to rename via context menu
     * @param groupId The ID of the group to rename
     */
    void renameRequested(GroupId groupId);
    
    /**
     * @brief Emitted when user requests to change color via context menu
     * @param groupId The ID of the group to recolor
     */
    void changeColorRequested(GroupId groupId);
    
    /**
     * @brief Emitted when user requests to change label color via context menu
     * @param groupId The ID of the group to recolor label
     */
    void changeLabelColorRequested(GroupId groupId);
    
    /**
     * @brief Emitted when user requests to copy the group via context menu
     * @param groupId The ID of the group to copy
     */
    void copyRequested(GroupId groupId);

    /**
     * @brief Emitted when user requests to cut the group via context menu
     * @param groupId The ID of the group to cut
     */
    void cutRequested(GroupId groupId);

public:
    /**
     * @brief Handles context menu to show group options
     * @param event Context menu event
     */
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    
    /**
     * @brief Brings this group to the front (highest z-value among groups)
     * 
     * Called when the group is clicked or selected to ensure it displays
     * above other groups while still remaining below nodes.
     */
    void bringToFront();

protected:
    /**
     * @brief Custom paint for rounded rectangle with label
     * @param painter QPainter for drawing
     * @param option Style options
     * @param widget Target widget (unused)
     */
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;
    
    /**
     * @brief Handles mouse move events to drag the entire group
     * @param event Mouse event
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    
    /**
     * @brief Handles mouse press events to prepare for dragging
     *        and to detect clicks on the top-left minimize button.
     * @param event Mouse event
     */
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    
    /**
     * @brief Handles mouse release events for context menu
     * @param event Mouse event
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    
    /**
     * @brief Intercept item changes to handle position changes
     * @param change The type of change
     * @param value The new value
     * @return The processed value
     */
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    
    /**
     * @brief Override shape to only accept clicks on border area (not interior)
     * This allows nodes inside to be selected without group interfering
     * @return The clickable shape (border path)
     */
    QPainterPath shape() const override;

private:
    GroupId mGroupId;                       ///< Associated group ID
    std::set<NodeId> mNodeIds;              ///< Member node IDs
    QGraphicsTextItem* mpLabel{nullptr};    ///< Group name label
    QString mName;                          ///< Group name
    QColor mColor;                          ///< Group color
    QColor mLabelColor{Qt::white};          ///< Group label text color (default white)
    QPointF mLastMousePos;                  ///< Last mouse position for drag calculation
    QPoint mContextMenuPos;                 ///< Screen position for context menu
    bool mMinimized{false};                 ///< Minimized state
    bool mUpdatingPosition{false};          ///< Flag to prevent recursive position updates
    bool mIsDragging{false};                ///< Flag to track if currently dragging
    bool mLocked{false};                    ///< Position lock state
    std::map<NodeId, qreal> mSavedNodeZ;    ///< Saved z-values for member nodes when raising
    QPointF mSavedTopLeft{0.0, 0.0};        ///< Saved top-left of expanded bounds for anchoring when minimized
    bool mHasSavedTopLeft{false};           ///< Whether `mSavedTopLeft` contains a valid value
    
};
