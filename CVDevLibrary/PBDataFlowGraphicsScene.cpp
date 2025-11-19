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

#include "PBDataFlowGraphicsScene.hpp"
#include "PBNodePainter.hpp"
#include "PBConnectionPainter.hpp"
#include "PBNodeDelegateModel.hpp"
#include "PBNodeGeometry.hpp"
#include "PropertyChangeCommand.hpp"
#include "PBDataFlowGraphModel.hpp"
#include "PBNodeGroupGraphicsItem.hpp"
#include "PBFlowGraphicsView.hpp"

#include <QtNodes/internal/DataFlowGraphModel.hpp>
#include <QtNodes/internal/AbstractNodeGeometry.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/BasicGraphicsScene.hpp>
#include <QtNodes/internal/Definitions.hpp>

#include <QGraphicsSceneMouseEvent>
#include <QEvent>
#include <QApplication>
#include <QInputDialog>
#include <QColorDialog>
#include <QGraphicsView>
#include <QWidget>
#include <QTimer>
#include <cmath>
#include <algorithm>

using namespace QtNodes;

PBDataFlowGraphicsScene::PBDataFlowGraphicsScene(DataFlowGraphModel &graphModel, QObject *parent)
    : DataFlowGraphicsScene(graphModel, parent)
{
    // Set custom node painter
    setNodePainter(std::make_unique<PBNodePainter>());
    
    // Set custom connection painter for group-aware routing
    if (auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel)) {
        setConnectionPainter(std::make_unique<PBConnectionPainter>(*pbModel));
    }
    
    // Connect to model's group signals if it's our custom model
    if (auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel)) {
        connect(pbModel, &PBDataFlowGraphModel::groupCreated,
                this, &PBDataFlowGraphicsScene::updateGroupVisual);
        connect(pbModel, &PBDataFlowGraphModel::groupDissolved,
                this, [this](GroupId groupId) {
                    // Remove graphics item for dissolved group
                    auto it = mGroupItems.find(groupId);
                    if (it != mGroupItems.end()) {
                        removeItem(it->second);
                        delete it->second;
                        mGroupItems.erase(it);
                    }
                });
        connect(pbModel, &PBDataFlowGraphModel::groupUpdated,
                this, &PBDataFlowGraphicsScene::updateGroupVisual);
        
        // Also update groups when nodes move. Use queued connection so node
        // geometry updates (recomputeSize) occur first and the group visual
        // update runs after the node's visual has been updated.
        connect(&graphModel,
            &QtNodes::AbstractGraphModel::nodePositionUpdated,
            this,
            &PBDataFlowGraphicsScene::updateAllGroupVisuals,
            Qt::QueuedConnection);

        // Also update groups when node geometry changes (widget resize, etc.).
        // Use a queued connection so the BasicGraphicsScene's onNodeUpdated
        // handler (which recomputes node geometry) runs first.
        connect(&graphModel,
            &QtNodes::AbstractGraphModel::nodeUpdated,
            this,
            &PBDataFlowGraphicsScene::updateAllGroupVisuals,
            Qt::QueuedConnection);
        
        // Additionally, listen to scene change notifications (regions that
        // were repainted/updated). During interactive widget resize the
        // scene's changed regions are emitted frequently — use this to
        // trigger group visual updates so the group's bounding rect follows
        // member node resizes live without modifying NodeEditor internals.
        connect(this, &QGraphicsScene::changed, this, [this](const QList<QRectF>&){
            // Debounce: call updateAllGroupVisuals directly; it's cheap
            // relative to painting and will early-return if no groups exist.
            updateAllGroupVisuals();
        });
    }
}

void PBDataFlowGraphicsScene::installCustomGeometry()
{
    // Access the geometry through non-const reference
    AbstractNodeGeometry &geom = nodeGeometry();
    
    // Replace using placement new (workaround since _nodeGeometry is private)
    geom.~AbstractNodeGeometry();  // Destroy current
    new (&geom) PBNodeGeometry(graphModel());  // Placement new with our custom geometry
}

void PBDataFlowGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // Check if any locked nodes are being moved - prevent the event entirely
    auto items = selectedItems();
    for (auto *item : items) {
        if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject*>(item)) {
            NodeId nodeId = ngo->nodeId();
            auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&graphModel());
            if (dataFlowModel) {
                auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
                if (delegateModel && delegateModel->isLockPosition()) {
                    // Node is locked - don't process this event at all
                    event->ignore();
                    return;
                }
            }
        }
    }
    
    // Call base implementation first
    DataFlowGraphicsScene::mouseMoveEvent(event);
    
    // Apply snap to grid for position only (not resize)
    if (mbSnapToGrid && QApplication::mouseButtons() == Qt::LeftButton) {
        // Get all selected items
        auto items = selectedItems();
        for (auto *item : items) {
            if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject*>(item)) {
                // Snap position to grid
                QPointF pos = ngo->pos();
                qreal xSnapped = std::floor(pos.x() / miGridSize) * miGridSize;
                qreal ySnapped = std::floor(pos.y() / miGridSize) * miGridSize;
                
                if (pos.x() != xSnapped || pos.y() != ySnapped) {
                    ngo->setPos(xSnapped, ySnapped);
                }
            }
        }
    }
}

void PBDataFlowGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // Check if release is on any node's checkbox
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    
    if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject*>(item)) {
        NodeId nodeId = ngo->nodeId();
        QPointF nodePos = ngo->mapFromScene(event->scenePos());
        
        // Check if released over any checkbox
        QRectF minimizeCheckboxRect = getMinimizeCheckboxRect(nodeId);
        QRectF lockCheckboxRect = getLockCheckboxRect(nodeId);
        QRectF enableCheckboxRect = getEnableCheckboxRect(nodeId);
        
        if (minimizeCheckboxRect.contains(nodePos) || 
            lockCheckboxRect.contains(nodePos) || 
            enableCheckboxRect.contains(nodePos)) {
            // Force cursor back to arrow after checkbox interaction
            QTimer::singleShot(0, [ngo]() {
                ngo->setCursor(QCursor(Qt::ArrowCursor));
            });
        }
    }
    
    // Call base implementation
    DataFlowGraphicsScene::mouseReleaseEvent(event);
}

void PBDataFlowGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel());
    if (!pbModel) {
        return;
    }
    
    // Check if there's a group at the context menu position
    QList<QGraphicsItem *> items = this->items(event->scenePos());
    for (QGraphicsItem *item : items) {
        if (auto *groupItem = qgraphicsitem_cast<PBNodeGroupGraphicsItem*>(item)) {
            // Found a group at this position, delegate context menu to it
            groupItem->contextMenuEvent(event);
            // Check if the event was accepted by the group
            if (event->isAccepted()) {
                return;
            }
        }
    }
    
    // No group found or event not accepted, use default behavior
    DataFlowGraphicsScene::contextMenuEvent(event);
}

QRectF PBDataFlowGraphicsScene::getEnableCheckboxRect(NodeId nodeId) const
{
    auto ngo = const_cast<PBDataFlowGraphicsScene*>(this)->nodeGraphicsObject(nodeId);
    if (!ngo)
        return QRectF();
    
    AbstractNodeGeometry &geometry = const_cast<AbstractNodeGeometry&>(nodeGeometry());
    QSize size = geometry.size(nodeId);
    
    QPointF checkboxPos(CHECKBOX_MARGIN, size.height() - CHECKBOX_SIZE - CHECKBOX_MARGIN);
    return QRectF(checkboxPos, QSizeF(CHECKBOX_SIZE, CHECKBOX_SIZE));
}

QRectF PBDataFlowGraphicsScene::getLockCheckboxRect(NodeId nodeId) const
{
    auto ngo = const_cast<PBDataFlowGraphicsScene*>(this)->nodeGraphicsObject(nodeId);
    if (!ngo)
        return QRectF();
    
    AbstractNodeGeometry &geometry = const_cast<AbstractNodeGeometry&>(nodeGeometry());
    QSize size = geometry.size(nodeId);
    
    QPointF checkboxPos(size.width() - CHECKBOX_SIZE - CHECKBOX_MARGIN, CHECKBOX_MARGIN);
    return QRectF(checkboxPos, QSizeF(CHECKBOX_SIZE, CHECKBOX_SIZE));
}

QRectF PBDataFlowGraphicsScene::getMinimizeCheckboxRect(NodeId nodeId) const
{
    auto ngo = const_cast<PBDataFlowGraphicsScene*>(this)->nodeGraphicsObject(nodeId);
    if (!ngo)
        return QRectF();
    
    QPointF checkboxPos(CHECKBOX_MARGIN, CHECKBOX_MARGIN);
    return QRectF(checkboxPos, QSizeF(CHECKBOX_SIZE, CHECKBOX_SIZE));
}

void PBDataFlowGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Check if click is on any node's enable/disable or lock checkbox
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    
    if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject*>(item)) {
        NodeId nodeId = ngo->nodeId();
        
        // Convert scene position to node-local coordinates
        QPointF nodePos = ngo->mapFromScene(event->scenePos());
        
        // Check if node is locked - prevent resize handle interaction
        auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&graphModel());
        if (dataFlowModel) {
            auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
            if (delegateModel && delegateModel->isLockPosition()) {
                // Check if clicking on resize handle
                AbstractNodeGeometry &geometry = nodeGeometry();
                QRect resizeRect = geometry.resizeHandleRect(nodeId);
                if (resizeRect.contains(nodePos.toPoint())) {
                    // Locked node - don't allow resize, just consume the event
                    event->accept();
                    return;
                }
            }
        }
        
        // Check minimize checkbox (top-left)
        QRectF minimizeCheckboxRect = getMinimizeCheckboxRect(nodeId);
        if (minimizeCheckboxRect.contains(nodePos)) {
            // Check if the node can be minimized
            auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&graphModel());
            if (dataFlowModel) {
                auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
                
                if (delegateModel && !delegateModel->canMinimize()) {
                    // Node cannot be minimized, ignore the click
                    DataFlowGraphicsScene::mousePressEvent(event);
                    return;
                }
            }
            
            // Only allow toggling if the node is selected
            if (!ngo->isSelected()) {
                // Node is not selected, just pass the event to select it
                DataFlowGraphicsScene::mousePressEvent(event);
                return;
            }
            
            // Node is selected, toggle the minimize state via undo command
            if (dataFlowModel) {
                auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
                
                if (delegateModel) {
                    bool currentState = delegateModel->isMinimize();
                    bool newState = !currentState;
                    
                    // Get the old value for undo
                    QString propId = "minimize";
                    QVariant oldValue = delegateModel->getModelPropertyValue(propId);
                    QVariant newValue = newState;
                    
                    // Create and push undo command
                    auto *cmd = new PropertyChangeCommand(this,
                                                          nodeId,
                                                          delegateModel,
                                                          propId,
                                                          oldValue,
                                                          newValue);
                    undoStack().push(cmd);
                    
                    // Don't propagate this event further
                    event->accept();
                    return;
                }
            }
        }
        
        // Check lock checkbox (top-right)
        QRectF lockCheckboxRect = getLockCheckboxRect(nodeId);
        if (lockCheckboxRect.contains(nodePos)) {
            // Only allow toggling if the node is selected
            if (!ngo->isSelected()) {
                // Node is not selected, just pass the event to select it
                DataFlowGraphicsScene::mousePressEvent(event);
                return;
            }
            
            // Node is selected, toggle the lock state via undo command
            auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&graphModel());
            if (dataFlowModel) {
                auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
                
                if (delegateModel) {
                    bool currentState = delegateModel->isLockPosition();
                    bool newState = !currentState;
                    
                    // Get the old value for undo
                    QString propId = "lock_position";
                    QVariant oldValue = delegateModel->getModelPropertyValue(propId);
                    QVariant newValue = newState;
                    
                    // Create and push undo command
                    auto *cmd = new PropertyChangeCommand(this,
                                                          nodeId,
                                                          delegateModel,
                                                          propId,
                                                          oldValue,
                                                          newValue);
                    undoStack().push(cmd);
                    
                    // Don't propagate this event further
                    event->accept();
                    return;
                }
            }
        }
        
        // Check enable checkbox (bottom-left)
        QRectF enableCheckboxRect = getEnableCheckboxRect(nodeId);
        if (enableCheckboxRect.contains(nodePos)) {
            // Only allow toggling if the node is selected
            if (!ngo->isSelected()) {
                // Node is not selected, just pass the event to select it
                DataFlowGraphicsScene::mousePressEvent(event);
                return;
            }
            
            // Node is selected, toggle the enable state via undo command
            auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&graphModel());
            if (dataFlowModel) {
                auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
                
                if (delegateModel) {
                    bool currentState = delegateModel->isEnable();
                    bool newState = !currentState;
                    
                    // Get the old value for undo
                    QString propId = "enable";
                    QVariant oldValue = delegateModel->getModelPropertyValue(propId);
                    QVariant newValue = newState;
                    
                    // Create and push undo command
                    auto *cmd = new PropertyChangeCommand(this,
                                                          nodeId,
                                                          delegateModel,
                                                          propId,
                                                          oldValue,
                                                          newValue);
                    undoStack().push(cmd);
                    
                    // Don't propagate this event further
                    event->accept();
                    return;
                }
            }
        }
    }
    
    // If not clicking checkbox, handle normally
    DataFlowGraphicsScene::mousePressEvent(event);
}

// ========== Node Grouping Implementation ==========

void
PBDataFlowGraphicsScene::
updateGroupVisual(GroupId groupId)
{
    auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel());
    if (!pbModel) {
        return;
    }
    
    const PBNodeGroup* group = pbModel->getGroup(groupId);
    if (!group) {
        return;
    }
    
    // Create graphics item if it doesn't exist
    PBNodeGroupGraphicsItem* item = nullptr;
    auto it = mGroupItems.find(groupId);
    if (it == mGroupItems.end()) {
        item = new PBNodeGroupGraphicsItem(groupId);
        addItem(item);
        mGroupItems[groupId] = item;
        
        // Connect signals
        connect(item, &PBNodeGroupGraphicsItem::groupMoved,
                this, &PBDataFlowGraphicsScene::onGroupMoved);
        connect(item, &PBNodeGroupGraphicsItem::toggleMinimizeRequested,
                this, &PBDataFlowGraphicsScene::onToggleGroupMinimize);
        connect(item, &PBNodeGroupGraphicsItem::ungroupRequested,
                this, &PBDataFlowGraphicsScene::onUngroupRequested);
        connect(item, &PBNodeGroupGraphicsItem::renameRequested,
                this, &PBDataFlowGraphicsScene::onRenameRequested);
        connect(item, &PBNodeGroupGraphicsItem::changeColorRequested,
                this, &PBDataFlowGraphicsScene::onChangeColorRequested);
        // Forward copy/cut requests from the group's context menu to the view
        connect(item, &PBNodeGroupGraphicsItem::copyRequested, this, [this](GroupId){
            auto vlist = views();
            if (!vlist.isEmpty()) {
                if (auto *pv = dynamic_cast<PBFlowGraphicsView*>(vlist.first())) {
                    pv->triggerCopy();
                }
            }
        });
        connect(item, &PBNodeGroupGraphicsItem::cutRequested, this, [this](GroupId){
            auto vlist = views();
            if (!vlist.isEmpty()) {
                if (auto *pv = dynamic_cast<PBFlowGraphicsView*>(vlist.first())) {
                    pv->triggerCut();
                }
            }
        });
    } else {
        item = it->second;
    }
    
    // Update group properties
    item->setGroup(*group);
    
    // Handle node visibility based on minimized state
    for (NodeId nodeId : group->nodes()) {
        if (auto* ngo = nodeGraphicsObject(nodeId)) {
            ngo->setVisible(!group->isMinimized());
        }
    }
    
    // Collect node positions and sizes for all nodes in the group
    std::map<NodeId, QPointF> nodePositions;
    std::map<NodeId, QSizeF> nodeSizes;
    
    for (NodeId nodeId : group->nodes()) {
        // Check if node still exists in the model
        auto allNodes = pbModel->allNodeIds();
        if (std::find(allNodes.begin(), allNodes.end(), nodeId) == allNodes.end()) {
            continue;
        }
        
        // Get node graphics object
        auto* ngo = nodeGraphicsObject(nodeId);
        if (!ngo) {
            continue;
        }
        
        // Get node's scene bounding rect to ensure consistent coordinate system
        QRectF sceneBounds = ngo->sceneBoundingRect();
        nodePositions[nodeId] = sceneBounds.topLeft();
        nodeSizes[nodeId] = sceneBounds.size();
    }
    
    // Update bounds
    item->updateBounds(nodePositions, nodeSizes);

    // After group bounds updated, other connections that route to the group's
    // boundary may need to be recomputed and repainted. Ensure all
    // connections for nodes in this group are moved so their Connection
    // Graphics Objects update their geometry.
    for (NodeId nodeId : group->nodes()) {
        if (auto *ngo = nodeGraphicsObject(nodeId)) {
            ngo->moveConnections();
        }
    }
}

void
PBDataFlowGraphicsScene::
updateAllGroupVisuals()
{
    auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel());
    if (!pbModel) {
        return;
    }
    
    for (const auto& pair : pbModel->groups()) {
        updateGroupVisual(pair.first);
    }
}

PBNodeGroupGraphicsItem*
PBDataFlowGraphicsScene::
getGroupGraphicsItem(GroupId groupId) const
{
    auto it = mGroupItems.find(groupId);
    if (it != mGroupItems.end()) {
        return it->second;
    }
    return nullptr;
}

void
PBDataFlowGraphicsScene::
onGroupMoved(GroupId groupId, QPointF delta)
{
    auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel());
    if (!pbModel) {
        return;
    }
    
    const PBNodeGroup* group = pbModel->getGroup(groupId);
    if (!group) {
        return;
    }
    
    // Move all nodes in the group
    for (NodeId nodeId : group->nodes()) {
        if (auto* ngo = nodeGraphicsObject(nodeId)) {
            QPointF currentPos = ngo->pos();
            QPointF newPos = currentPos + delta;
            ngo->setPos(newPos);
            
            // Update the model with the new position
            graphModel().setNodeData(nodeId, QtNodes::NodeRole::Position, newPos);
        }
    }
    
    // Update the group visual to reflect new node positions
    // This will reposition the group item to match the nodes
    updateGroupVisual(groupId);
}

void
PBDataFlowGraphicsScene::
onToggleGroupMinimize(GroupId groupId)
{
    auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel());
    if (!pbModel) {
        return;
    }
    
    // Toggle the minimized state in the model
    if (pbModel->toggleGroupMinimized(groupId)) {
        // The groupUpdated signal will trigger updateGroupVisual
        // which will update node visibility and group bounds
        updateGroupVisual(groupId);
        
        // Update all connections to/from nodes in this group
        const PBNodeGroup* group = pbModel->getGroup(groupId);
        if (group) {
            for (NodeId nodeId : group->nodes()) {
                // Update all connections for this node
                auto allConnections = graphModel().allConnectionIds(nodeId);
                for (const auto& connectionId : allConnections) {
                    if (auto* cgo = connectionGraphicsObject(connectionId)) {
                        // Recompute endpoints and then request a repaint. Calling
                        // both ensures the old and new geometry areas are invalidated
                        // and the view's background cache is refreshed, which
                        // prevents trailing artefacts when hiding connections.
                        cgo->move();
                        cgo->update();
                    }
                }
            }

            // Also force a scene-level update to ensure any background cache
            // is repainted now that several connection items have changed.
            this->update();
            
            // Additionally, update a slightly larger area around the group's
            // bounding rect and force the view viewport to repaint that area
            // as well. This helps clear trailing artefacts when device- or
            // background-caching is enabled on the view.
            PBNodeGroupGraphicsItem* groupItem = getGroupGraphicsItem(groupId);
            if (groupItem) {
                QRectF bounds = groupItem->sceneBoundingRect();
                const qreal margin = 64.0; // pixels to inflate the repaint area (increased)
                QRectF expanded = bounds.adjusted(-margin, -margin, margin, margin);

                // Request scene to update the expanded area
                this->update(expanded);

                // Also request each view's viewport to update the corresponding
                // widget rectangle so view-level caches are invalidated.
                auto vlist = views();
                for (QGraphicsView* v : vlist) {
                    if (!v) continue;
                    // Map scene rect to viewport widget coordinates
                    QPoint tl = v->mapFromScene(expanded.topLeft());
                    QPoint br = v->mapFromScene(expanded.bottomRight());
                    QRect viewRect(tl, br);
                    viewRect = viewRect.normalized();

                    if (v->viewport()) {
                        v->viewport()->update(viewRect);
                    }
                }
            }
        }
    }
}

void
PBDataFlowGraphicsScene::
onUngroupRequested(GroupId groupId)
{
    auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel());
    if (!pbModel) {
        return;
    }
    
    // Simply dissolve the group - no confirmation needed from context menu
    pbModel->dissolveGroup(groupId);
}

void
PBDataFlowGraphicsScene::
onRenameRequested(GroupId groupId)
{
    // Forward this to MainWindow via the view
    auto viewsList = views();
    if (!viewsList.isEmpty()) {
        auto* view = viewsList.first();
        if (view) {
            // Get the widget parent for the dialog
            QWidget* parentWidget = qobject_cast<QWidget*>(view->parent());
            
            auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel());
            if (!pbModel) {
                return;
            }
            
            const PBNodeGroup* group = pbModel->getGroup(groupId);
            if (!group) {
                return;
            }
            
            bool ok;
            QString newName = QInputDialog::getText(parentWidget,
                                                    "Rename Group",
                                                    "Enter new name for group:",
                                                    QLineEdit::Normal,
                                                    group->name(),
                                                    &ok);
            
            if (ok && !newName.isEmpty()) {
                pbModel->setGroupName(groupId, newName);
            }
        }
    }
}

void
PBDataFlowGraphicsScene::
onChangeColorRequested(GroupId groupId)
{
    // Forward this to view for color dialog
    auto viewsList = views();
    if (!viewsList.isEmpty()) {
        auto* view = viewsList.first();
        if (view) {
            // Get the widget parent for the dialog
            QWidget* parentWidget = qobject_cast<QWidget*>(view->parent());
            
            auto* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel());
            if (!pbModel) {
                return;
            }
            
            const PBNodeGroup* group = pbModel->getGroup(groupId);
            if (!group) {
                return;
            }
            
            QColor newColor = QColorDialog::getColor(group->color(), parentWidget, "Select Group Color",
                                                      QColorDialog::ShowAlphaChannel);
            
            if (newColor.isValid()) {
                pbModel->setGroupColor(groupId, newColor);
            }
        }
    }
}
