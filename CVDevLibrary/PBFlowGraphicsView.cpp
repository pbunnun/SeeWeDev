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

#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/UndoCommands.hpp>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/NodeDelegateModel>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QLineEdit>
#include <QWidgetAction>
#include <QTreeWidget>
#include <QHeaderView>
#include <QKeyEvent>
#include <set>
#include "PBFlowGraphicsView.hpp"
#include "PBNodeDelegateModel.hpp"
#include "PBDataFlowGraphicsScene.hpp"
#include "PBNodeGroupGraphicsItem.hpp"
#include "GroupCommands.hpp"
#include "PBNodeGroup.hpp"
#include "PBDataFlowGraphModel.hpp"
#include "GroupPasteCommand.hpp"
#include "PBDeleteCommand.hpp"

PBFlowGraphicsView::PBFlowGraphicsView(QtNodes::BasicGraphicsScene *scene, QWidget *parent)
    : QtNodes::GraphicsView(scene, parent),
      mpGraphicsScene(scene)
{
    mpDataFlowGraphicsScene = dynamic_cast<QtNodes::DataFlowGraphicsScene*>(mpGraphicsScene);
    setAcceptDrops(true);
}

void
PBFlowGraphicsView::
dragMoveEvent(QDragMoveEvent *event)
{
    event->setDropAction(Qt::MoveAction);
    event->accept();
}

void
PBFlowGraphicsView::
dropEvent(QDropEvent *event)
{
    // Use the graph model to create nodes, not the scene directly
    auto* dataFlowScene = dynamic_cast<QtNodes::DataFlowGraphicsScene*>(mpGraphicsScene);
    if (!dataFlowScene) {
        event->ignore();
        return;
    }

    auto& graphModel = dataFlowScene->graphModel();
    auto registry = dynamic_cast<QtNodes::DataFlowGraphModel&>(graphModel).dataModelRegistry();
    
    auto type = registry->create(event->mimeData()->text());
    if(type)
    {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0 ))
        QPoint pos = event->pos();
#else
        QPoint pos = event->position().toPoint();
#endif

        QPointF posView = this->mapToScene(pos);
        
        // Use CreateCommand to properly integrate with undo/redo system
        dataFlowScene->undoStack().push(new QtNodes::CreateCommand(dataFlowScene, 
                                                                    event->mimeData()->text(), 
                                                                    posView));
    }
    event->accept();
}

void
PBFlowGraphicsView::
contextMenuEvent(QContextMenuEvent *event)
{
    if( !mpDataFlowGraphicsScene )
        return;

    auto& graphModel = mpDataFlowGraphicsScene->graphModel();
    
    // Check if clicking on a node or connection
    QGraphicsItem* item = itemAt(event->pos());
    
    // Check for group first. If the clicked item is a child (e.g. caption/label),
    // climb the parent chain to find the enclosing PBNodeGroupGraphicsItem so
    // clicks near the group's caption still open the group context menu.
    auto* pbScene = dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene);
    if (pbScene) {
        PBNodeGroupGraphicsItem* groupItem = nullptr;
        QGraphicsItem* probe = item;
        while (probe) {
            groupItem = dynamic_cast<PBNodeGroupGraphicsItem*>(probe);
            if (groupItem)
                break;
            probe = probe->parentItem();
        }
        if (groupItem) {
            // Ensure the clicked group is selected so copy/cut/delete operate on it
            if (!groupItem->isSelected()) {
                mpDataFlowGraphicsScene->clearSelection();
                groupItem->setSelected(true);
            }

            // Context menu for group: Copy/Cut/Delete above Expand/Minimize
            QMenu groupMenu;

            QAction* copyAction = groupMenu.addAction("Copy");
            copyAction->setIcon(QIcon(":/icons/tango/16x16/edit-copy.png"));
            copyAction->setIconVisibleInMenu(true);

            QAction* cutAction = groupMenu.addAction("Cut");
            cutAction->setIcon(QIcon(":/icons/tango/16x16/edit-cut.png"));
            cutAction->setIconVisibleInMenu(true);

            QAction* deleteAction = groupMenu.addAction("Delete");
            deleteAction->setIcon(QIcon(":/icons/tango/16x16/edit-delete.png"));
            deleteAction->setIconVisibleInMenu(true);
            groupMenu.addSeparator();

            QAction* minimizeAction = groupMenu.addAction(groupItem->isMinimized() ? "Expand Group" : "Minimize Group");
            groupMenu.addSeparator();
            QAction* renameAction = groupMenu.addAction("Rename Group...");
            QAction* colorAction = groupMenu.addAction("Change Color...");
            QAction* labelColorAction = groupMenu.addAction("Change Label Color...");
            groupMenu.addSeparator();
            QAction* ungroupAction = groupMenu.addAction("Ungroup");

            QAction* selectedAction = groupMenu.exec(event->globalPos());

            if (selectedAction == copyAction) {
                // Use view-level handler to perform group-aware copy
                this->triggerCopy();
            } else if (selectedAction == cutAction) {
                this->triggerCut();
            } else if (selectedAction == deleteAction) {
                this->triggerDelete();
            } else if (selectedAction == minimizeAction) {
                groupItem->toggleMinimizeRequested(groupItem->groupId());
            } else if (selectedAction == renameAction) {
                groupItem->renameRequested(groupItem->groupId());
            } else if (selectedAction == colorAction) {
                groupItem->changeColorRequested(groupItem->groupId());
            } else if (selectedAction == labelColorAction) {
                groupItem->changeLabelColorRequested(groupItem->groupId());
            } else if (selectedAction == ungroupAction) {
                groupItem->ungroupRequested(groupItem->groupId());
            }

            return;
        }
    }
    
    // Check for connection
    auto* connectionItem = dynamic_cast<QtNodes::ConnectionGraphicsObject*>(item);
    if (connectionItem)
    {
        // Select the connection if it's not already selected
        if (!connectionItem->isSelected())
        {
            mpDataFlowGraphicsScene->clearSelection();
            connectionItem->setSelected(true);
        }
        
        // Context menu for connection: Delete only
        QMenu connectionMenu;
        QAction* deleteAction = new QAction("Delete Connection", &connectionMenu);
        deleteAction->setIcon(QIcon(":/icons/tango/16x16/edit-delete.png"));
        deleteAction->setIconVisibleInMenu(true);  // Force icon to show on macOS
        connectionMenu.addAction(deleteAction);
        
        QAction* selectedAction = connectionMenu.exec(event->globalPos());
        
        if (selectedAction == deleteAction)
        {
            // Delete the connection using the graph model
            QtNodes::ConnectionId connectionId = connectionItem->connectionId();
            graphModel.deleteConnection(connectionId);
        }
        
        return;
    }
    
    // Check for node
    auto* nodeItem = dynamic_cast<QtNodes::NodeGraphicsObject*>(item);
    
    if (nodeItem)
    {
        // Select the node if it's not already selected
        // This ensures copy/cut/delete operations work on the right-clicked node
        if (!nodeItem->isSelected())
        {
            mpDataFlowGraphicsScene->clearSelection();
            nodeItem->setSelected(true);
        }
        
        // Context menu for node: Copy, Cut, Delete
        QMenu nodeMenu;
        
        QAction* copyAction = new QAction("Copy", &nodeMenu);
        copyAction->setIcon(QIcon(":/icons/tango/16x16/edit-copy.png"));
        copyAction->setIconVisibleInMenu(true);  // Force icon to show on macOS
        nodeMenu.addAction(copyAction);
        
        QAction* cutAction = new QAction("Cut", &nodeMenu);
        cutAction->setIcon(QIcon(":/icons/tango/16x16/edit-cut.png"));
        cutAction->setIconVisibleInMenu(true);  // Force icon to show on macOS
        nodeMenu.addAction(cutAction);
        
        nodeMenu.addSeparator();
        
        QAction* deleteAction = new QAction("Delete", &nodeMenu);
        deleteAction->setIcon(QIcon(":/icons/tango/16x16/edit-delete.png"));
        deleteAction->setIconVisibleInMenu(true);  // Force icon to show on macOS
        nodeMenu.addAction(deleteAction);

        // Bring to Front / Send to Back actions
        QAction* bringToFrontAction = new QAction("Bring to Front", &nodeMenu);
        QAction* sendToBackAction   = new QAction("Send to Back", &nodeMenu);
        nodeMenu.addAction(bringToFrontAction);
        nodeMenu.addAction(sendToBackAction);
        
        QAction* selectedAction = nodeMenu.exec(event->globalPos());
        
        if (selectedAction == copyAction)
        {
            // Use NodeEditor v3's built-in CopyCommand
            mpDataFlowGraphicsScene->undoStack().push(new QtNodes::CopyCommand(mpDataFlowGraphicsScene));
        }
        else if (selectedAction == cutAction)
        {
            // Copy then delete using undo commands
            mpDataFlowGraphicsScene->undoStack().push(new QtNodes::CopyCommand(mpDataFlowGraphicsScene));
            mpDataFlowGraphicsScene->undoStack().push(new PBDeleteCommand(dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene)));
        }
        else if (selectedAction == deleteAction)
        {
            // Use CVDev's PBDeleteCommand which preserves group membership on undo
            mpDataFlowGraphicsScene->undoStack().push(new PBDeleteCommand(dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene)));
        }
        else if (selectedAction == bringToFrontAction)
        {
            // Raise the selected node above others persistently by
            // adjusting stacking order among items sharing the same z.
            auto* scene = dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene);
            if (scene)
            {
                // Keep z-values as-is and reorder stacking so nodeItem is on top
                // When z is equal, QGraphicsScene uses insertion/stacking order.
                // Use stackBefore to make other nodes sit below the target.
                for (QGraphicsItem* gi : scene->items())
                {
                    if (auto* n = dynamic_cast<QtNodes::NodeGraphicsObject*>(gi))
                    {
                        if (n == nodeItem)
                            continue;
                        // Only adjust nodes at the same zValue to preserve groups/edges order
                        if (qFuzzyCompare(n->zValue(), nodeItem->zValue()))
                        {
                            n->stackBefore(nodeItem);
                        }
                    }
                }
            }
        }
        else if (selectedAction == sendToBackAction)
        {
            // Push the selected node below others at the same z-value
            auto* scene = dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene);
            if (scene)
            {
                for (QGraphicsItem* gi : scene->items())
                {
                    if (auto* n = dynamic_cast<QtNodes::NodeGraphicsObject*>(gi))
                    {
                        if (n == nodeItem)
                            continue;
                        if (qFuzzyCompare(n->zValue(), nodeItem->zValue()))
                        {
                            // Place selected node before others → goes underneath
                            nodeItem->stackBefore(n);
                        }
                    }
                }
            }
        }
        
        return;
    }
    
    // Context menu for empty space: Paste + node creation list
    auto registry = dynamic_cast<QtNodes::DataFlowGraphModel&>(graphModel).dataModelRegistry();
    
    if (!registry) {
        return;
    }

    QMenu modelMenu;
    
    // Add Paste option at the top
    QAction* pasteAction = new QAction("Paste", &modelMenu);
    pasteAction->setIcon(QIcon(":/icons/tango/16x16/edit-paste.png"));
    pasteAction->setIconVisibleInMenu(true);  // Force icon to show on macOS
    
    // Check if clipboard has pasteable data
    QClipboard const *clipboard = QApplication::clipboard();
    QMimeData const *mimeData = clipboard->mimeData();
    bool hasClipboardData = mimeData->hasFormat("application/qt-nodes-graph") ||
                            mimeData->hasFormat("application/qt-nodes-graph-with-group") ||
                            (mimeData->hasText() && !mimeData->text().isEmpty());
    pasteAction->setEnabled(hasClipboardData);
    
    modelMenu.addAction(pasteAction);
    modelMenu.addSeparator();
    
    auto skipText = QStringLiteral("skip me");

    // Add filter box to the context menu
    auto *txtBox = new QLineEdit(&modelMenu);
    txtBox->setPlaceholderText(QStringLiteral("Filter"));
    txtBox->setClearButtonEnabled(true);

    auto *txtBoxAction = new QWidgetAction(&modelMenu);
    txtBoxAction->setDefaultWidget(txtBox);
    modelMenu.addAction(txtBoxAction);

    // Add result treeview to the context menu
    auto *treeView = new QTreeWidget(&modelMenu);
    treeView->header()->close();

    auto *treeViewAction = new QWidgetAction(&modelMenu);
    treeViewAction->setDefaultWidget(treeView);
    modelMenu.addAction(treeViewAction);

    // Build category tree
    QMap<QString, QTreeWidgetItem*> topLevelItems;
    for (auto const &cat : registry->categories())
    {
        auto item = new QTreeWidgetItem(treeView);
        item->setText(0, cat);
        item->setData(0, Qt::UserRole, skipText);
        topLevelItems[cat] = item;
    }

    // Add models to categories with icons
    // For each registered model, create a temporary instance to extract its minPixmap
    // and use it as an icon in the context menu for better visual identification
    for (auto const &assoc : registry->registeredModelsCategoryAssociation())
    {
        auto parent = topLevelItems[assoc.second];
        auto item = new QTreeWidgetItem(parent);
        item->setText(0, assoc.first);
        item->setData(0, Qt::UserRole, assoc.first);
        
        // Create a temporary instance of the model to get its minPixmap
        auto tempModel = registry->create(assoc.first);
        if (tempModel)
        {
            // Try to cast to PBNodeDelegateModel to access minPixmap
            auto* pbModel = dynamic_cast<PBNodeDelegateModel*>(tempModel.get());
            if (pbModel)
            {
                QPixmap minPixmap = pbModel->minPixmap();
                if (!minPixmap.isNull())
                {
                    // Resize to icon size (16x16) for menu display
                    QPixmap iconPixmap = minPixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    item->setIcon(0, QIcon(iconPixmap));
                }
            }
        }
    }

    treeView->expandAll();

    // Store the click position for later use
    QPoint clickPos = event->pos();

    // Handle item selection - create node using CreateCommand for undo/redo support
    auto* dataFlowScene = dynamic_cast<QtNodes::DataFlowGraphicsScene*>(mpGraphicsScene);
    connect(treeView, &QTreeWidget::itemClicked, [this, dataFlowScene, skipText, clickPos, &modelMenu](QTreeWidgetItem *item, int)
    {
        QString modelName = item->data(0, Qt::UserRole).toString();

        if (modelName == skipText)
        {
            return;
        }

        QPointF posView = this->mapToScene(clickPos);
        
        // Use CreateCommand to properly integrate with undo/redo system
        dataFlowScene->undoStack().push(new QtNodes::CreateCommand(dataFlowScene, modelName, posView));

        modelMenu.close();
    });

    // Handle Paste action - use view-level handler so group-aware MIME is honored
    connect(pasteAction, &QAction::triggered, [this]()
    {
        if (!mpDataFlowGraphicsScene)
            return;

        // Use our centralized paste handler which recognizes group-aware MIME
        this->triggerPaste();
    });

    // Setup filtering
    connect(txtBox, &QLineEdit::textChanged, [&topLevelItems](const QString &text)
    {
        for (auto& topLvlItem : topLevelItems)
        {
            bool shouldHideCategory = true;
            for (int i = 0; i < topLvlItem->childCount(); ++i)
            {
                auto child = topLvlItem->child(i);
                auto modelName = child->data(0, Qt::UserRole).toString();
                const bool match = (modelName.contains(text, Qt::CaseInsensitive));
                if (match)
                    shouldHideCategory = false;
                child->setHidden(!match);
            }
            topLvlItem->setHidden(shouldHideCategory);
        }
    });

    // Make sure the text box gets focus so the user doesn't have to click on it
    txtBox->setFocus();

    modelMenu.exec(event->globalPos());
}

void
PBFlowGraphicsView::
center_on( NodeId nodeId )
{
    // Need to get node position from the graph model
    if( !mpDataFlowGraphicsScene )
        return;

    auto& graphModel = mpDataFlowGraphicsScene->graphModel();
    
    // Get node position from model
    QPointF nodePos = graphModel.nodeData(nodeId, QtNodes::NodeRole::Position).value<QPointF>();
    
    // Get node size if available
    QSize nodeSize = graphModel.nodeData(nodeId, QtNodes::NodeRole::Size).value<QSize>();
    
    // Calculate center point
    QPointF centerPoint = nodePos + QPointF(nodeSize.width() / 2.0, nodeSize.height() / 2.0);
    
    // Center view on this point
    centerOn(centerPoint);
}

void
PBFlowGraphicsView::
center_on( QPointF const & center_pos )
{
    centerOn(center_pos);
}

std::vector<NodeId>
PBFlowGraphicsView::
selectedNodes()
{
    std::vector<NodeId> selected;
    
    // Safety check - scene might be nullptr during destruction
    if (!mpGraphicsScene) 
        return selected;
    
    auto selectedItems = mpGraphicsScene->selectedItems();
    for (auto *item : selectedItems)
    {
        //Safely check if item is valid before casting
        if (!item) 
            continue;
        
        if (auto *nodeObj = dynamic_cast<QtNodes::NodeGraphicsObject *>(item))
        {
            auto nodeId = nodeObj->nodeId();
            selected.push_back(nodeId);
        }
    }
    return selected;
}

NodeGraphicsObject*
PBFlowGraphicsView::
getGraphicsObject(NodeId id)
{
    // Safety check - scene might be nullptr during destruction
    if (!mpGraphicsScene) 
        return nullptr;
    return mpGraphicsScene->nodeGraphicsObject(id);
}

void
PBFlowGraphicsView::
clearSelection()
{
    // Safety check - scene might be nullptr during destruction
    if (!mpGraphicsScene)
        return;
    mpGraphicsScene->clearSelection();
}

void
PBFlowGraphicsView::
showConnections(std::unordered_set<QtNodes::ConnectionId> & connectionIds, bool bShow = false)
{
    // Safety check - scene might be nullptr during destruction
    if (!mpGraphicsScene)
        return;
    
    for( auto connectionId : connectionIds )
    {
        auto* connectionGraphicsObject = mpGraphicsScene->connectionGraphicsObject( connectionId );
        if (connectionGraphicsObject) {  // Additional safety check
            connectionGraphicsObject->setVisible( bShow );
            connectionGraphicsObject->setEnabled( bShow );
        }
    }
}

void
PBFlowGraphicsView::
keyPressEvent(QKeyEvent *event)
{
    if (!mpDataFlowGraphicsScene)
    {
        QtNodes::GraphicsView::keyPressEvent(event);
        return;
    }
    
    auto& graphModel = mpDataFlowGraphicsScene->graphModel();
    
    // Check for Delete key (Windows/Linux) or Backspace key (Mac)
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        // Get all selected items
        auto selectedItems = mpGraphicsScene->selectedItems();
        
        if (!selectedItems.isEmpty())
        {
            bool hasSelection = false;
            // Check if any connections are selected
            for (auto *item : selectedItems)
            {
                auto *connectionItem = dynamic_cast<QtNodes::ConnectionGraphicsObject*>(item);
                if ( connectionItem )
                {
                    hasSelection = true;
                    break;
                }
            }
            
            // Check if any nodes are selected
            if (!hasSelection)
            {
                // Check if any group is selected
                for (auto *item : selectedItems)
                {
                    if (auto *groupItem = dynamic_cast<PBNodeGroupGraphicsItem*>(item))
                    {
                        // Delete the group and its member nodes with undo support
                        if (auto *pbScene = dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene)) {
                            auto &graphModel = pbScene->graphModel();
                            auto *pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel);
                            if (pbModel) {
                                const PBNodeGroup* group = pbModel->getGroup(groupItem->groupId());
                                if (group) {
                                    mpDataFlowGraphicsScene->undoStack().push(
                                        new GroupDeleteCommand(pbScene, pbModel, *group));
                                    event->accept();
                                    return;
                                }
                            }
                        }
                    }
                }

                for (auto *item : selectedItems)
                {
                    if (auto *nodeItem = dynamic_cast<QtNodes::NodeGraphicsObject*>(item))
                    {
                        NodeId nodeId = nodeItem->nodeId();
                        
                        // Get the delegate model - need to cast graphModel to DataFlowGraphModel
                        auto* dataFlowModel = dynamic_cast<QtNodes::DataFlowGraphModel*>(&graphModel);
                        if (dataFlowModel)
                        {
                            auto* delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
                            if (delegateModel && delegateModel->embeddedWidget())
                            {
                                if (delegateModel->isEditableEmbeddedWidgetSelected())
                                {
                                    QtNodes::GraphicsView::keyPressEvent(event);
                                    return;
                                }
                            }
                        }
                        hasSelection = true;
                        break;
                    }
                }
            }
            
            // If we have selected nodes or connections, delete them using DeleteCommand
            if (hasSelection)
            {
                mpDataFlowGraphicsScene->undoStack().push(new PBDeleteCommand(dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene)));
                event->accept();
                return;
            }
        }
    }
    
    // If not handled, pass to base class
    QtNodes::GraphicsView::keyPressEvent(event);
}

void
PBFlowGraphicsView::
triggerCopy()
{
    if (!mpDataFlowGraphicsScene)
        return;

    // If the selection corresponds to a PB group (all nodes of a group selected
    // or the group item selected), serialize nodes + connections + group metadata
    // to the clipboard under a custom MIME type so paste can recreate the group.
    auto& graphModel = mpDataFlowGraphicsScene->graphModel();

    // Prefer explicit group-item selection: if a PBNodeGroupGraphicsItem is selected,
    // copy the whole group. Otherwise fallback to selecting nodes and check if
    // they all belong to the same group.
    std::vector<NodeId> selectedNodes;
    PBDataFlowGraphModel* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel);

    // Check for explicit group item selection first
    GroupId explicitGroupId = InvalidGroupId;
    for (QGraphicsItem* it : mpDataFlowGraphicsScene->selectedItems()) {
        if (auto *groupItem = dynamic_cast<PBNodeGroupGraphicsItem*>(it)) {
            explicitGroupId = groupItem->groupId();
            break;
        }
    }

    if (explicitGroupId != InvalidGroupId && pbModel) {
        if (const PBNodeGroup* grp = pbModel->getGroup(explicitGroupId)) {
            for (auto nid : grp->nodes())
                selectedNodes.push_back(nid);
        }
    } else {
        // Collect selected node IDs
        for (QGraphicsItem* it : mpDataFlowGraphicsScene->selectedItems()) {
            if (auto *n = dynamic_cast<QtNodes::NodeGraphicsObject*>(it)) {
                selectedNodes.push_back(n->nodeId());
            }
        }
    }

    if (!selectedNodes.empty() && pbModel) {
        // Check if all selected nodes belong to the same group
        GroupId groupId = InvalidGroupId;
        bool allInSameGroup = true;
        for (auto nid : selectedNodes) {
            GroupId g = pbModel->getPBNodeGroup(nid);
            if (g == InvalidGroupId) { allInSameGroup = false; break; }
            if (groupId == InvalidGroupId) groupId = g;
            else if (groupId != g) { allInSameGroup = false; break; }
        }

        if (allInSameGroup && groupId != InvalidGroupId) {
            const PBNodeGroup* grp = pbModel->getGroup(groupId);
            bool treatAsWholeGroup = false;
            if (explicitGroupId != InvalidGroupId) {
                treatAsWholeGroup = true;
            } else if (grp && selectedNodes.size() == static_cast<size_t>(grp->nodes().size())) {
                treatAsWholeGroup = true;
            }

            if (treatAsWholeGroup) {
                // Build JSON: nodes, connections, group
                QJsonObject out;
                QJsonArray nodesArr;
                QJsonArray connArr;

                std::set<NodeId> selSet(selectedNodes.begin(), selectedNodes.end());

                for (auto nid : selectedNodes) {
                    nodesArr.append(pbModel->saveNode(nid));
                    for (auto const &cid : pbModel->allConnectionIds(nid)) {
                        // only include connections between selected nodes
                        if (selSet.count(cid.inNodeId) && selSet.count(cid.outNodeId))
                            connArr.append(QtNodes::toJson(cid));
                    }
                }

                out["nodes"] = nodesArr;
                out["connections"] = connArr;

                if (grp) {
                    out["group"] = grp->save();
                }

                QJsonDocument doc(out);
                QByteArray bytes = doc.toJson();

                QMimeData* mime = new QMimeData();
                mime->setData("application/qt-nodes-graph-with-group", bytes);
                mime->setText(QString::fromUtf8(bytes));
                QClipboard *clipboard = QApplication::clipboard();
                clipboard->setMimeData(mime);

                return;
            }
            }
        }

    // Fallback: use NodeEditor v3's built-in CopyCommand
    mpDataFlowGraphicsScene->undoStack().push(new QtNodes::CopyCommand(mpDataFlowGraphicsScene));
}

void
PBFlowGraphicsView::
triggerCut()
{
    if (!mpDataFlowGraphicsScene)
        return;

    // Attempt group-aware cut: if the current selection represents a whole group
    // (either the group item is selected or all selected nodes belong to the same group),
    // serialize the group to the clipboard using our custom MIME and then perform
    // an undoable group delete (so Cut is undoable as a single operation).
    auto& graphModel = mpDataFlowGraphicsScene->graphModel();
    PBDataFlowGraphModel* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel);

    // Determine selected node IDs (or explicit group id)
    std::vector<NodeId> selectedNodes;
    GroupId explicitGroupId = InvalidGroupId;
    for (QGraphicsItem* it : mpDataFlowGraphicsScene->selectedItems()) {
        if (auto *groupItem = dynamic_cast<PBNodeGroupGraphicsItem*>(it)) {
            explicitGroupId = groupItem->groupId();
            break;
        }
    }

    if (explicitGroupId != InvalidGroupId && pbModel) {
        if (const PBNodeGroup* grp = pbModel->getGroup(explicitGroupId)) {
            for (auto nid : grp->nodes())
                selectedNodes.push_back(nid);
        }
    } else {
        for (QGraphicsItem* it : mpDataFlowGraphicsScene->selectedItems()) {
            if (auto *n = dynamic_cast<QtNodes::NodeGraphicsObject*>(it)) {
                selectedNodes.push_back(n->nodeId());
            }
        }
    }

    if (!selectedNodes.empty() && pbModel) {
        // Check if all selected nodes belong to the same group
        GroupId groupId = InvalidGroupId;
        bool allInSameGroup = true;
        for (auto nid : selectedNodes) {
            GroupId g = pbModel->getPBNodeGroup(nid);
            if (g == InvalidGroupId) { allInSameGroup = false; break; }
            if (groupId == InvalidGroupId) groupId = g;
            else if (groupId != g) { allInSameGroup = false; break; }
        }

        if (allInSameGroup && groupId != InvalidGroupId) {
            const PBNodeGroup* grp = pbModel->getGroup(groupId);
            bool treatAsWholeGroup = false;
            if (explicitGroupId != InvalidGroupId) {
                treatAsWholeGroup = true;
            } else if (grp && selectedNodes.size() == static_cast<size_t>(grp->nodes().size())) {
                treatAsWholeGroup = true;
            }

            if (treatAsWholeGroup) {
                // Serialize group (nodes + connections + group metadata) to clipboard
                QJsonObject out;
                QJsonArray nodesArr;
                QJsonArray connArr;

                std::set<NodeId> selSet(selectedNodes.begin(), selectedNodes.end());

                for (auto nid : selectedNodes) {
                    nodesArr.append(pbModel->saveNode(nid));
                    for (auto const &cid : pbModel->allConnectionIds(nid)) {
                        if (selSet.count(cid.inNodeId) && selSet.count(cid.outNodeId))
                            connArr.append(QtNodes::toJson(cid));
                    }
                }

                out["nodes"] = nodesArr;
                out["connections"] = connArr;

                if (grp) {
                    out["group"] = grp->save();
                }

                QJsonDocument doc(out);
                QByteArray bytes = doc.toJson();

                QMimeData* mime = new QMimeData();
                mime->setData("application/qt-nodes-graph-with-group", bytes);
                mime->setText(QString::fromUtf8(bytes));
                QClipboard *clipboard = QApplication::clipboard();
                clipboard->setMimeData(mime);

                // Perform an undoable group delete
                if (auto *pbScene = dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene)) {
                    if (const PBNodeGroup* group = pbModel->getGroup(groupId)) {
                        mpDataFlowGraphicsScene->undoStack().push(
                            new GroupDeleteCommand(pbScene, pbModel, *group));
                        return;
                    }
                }
            }
        }
    }

    // Fallback: default copy + delete (for non-group selections)
    mpDataFlowGraphicsScene->undoStack().push(new QtNodes::CopyCommand(mpDataFlowGraphicsScene));
    mpDataFlowGraphicsScene->undoStack().push(new PBDeleteCommand(dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene)));
}

void
PBFlowGraphicsView::
triggerPaste()
{
    if (!mpDataFlowGraphicsScene)
        return;

    // Check clipboard for our custom group-aware MIME
    QClipboard const *clipboard = QApplication::clipboard();
    QMimeData const *mimeData = clipboard->mimeData();
    QPointF pastePosition = scenePastePosition();

    if (mimeData && mimeData->hasFormat("application/qt-nodes-graph-with-group")) {
        QByteArray data = mimeData->data("application/qt-nodes-graph-with-group");
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();

            PBDataFlowGraphModel* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&mpDataFlowGraphicsScene->graphModel());
            if (pbModel) {
                // Use GroupPasteCommand to perform paste and make it undoable
                mpDataFlowGraphicsScene->undoStack().push(new GroupPasteCommand(mpDataFlowGraphicsScene, pbModel, obj, pastePosition));
                return;
            }
        }
    }

    // Fallback to default NodeEditor paste
    mpDataFlowGraphicsScene->undoStack().push(new QtNodes::PasteCommand(mpDataFlowGraphicsScene, pastePosition));
}

void
PBFlowGraphicsView::
triggerDelete()
{
    if (!mpDataFlowGraphicsScene)
        return;

    // If the selection represents a PB group (explicit group item selected
    // or all selected nodes belong to the same group), perform an undoable
    // group delete so the whole group and its members are deleted together.
    auto& graphModel = mpDataFlowGraphicsScene->graphModel();
    PBDataFlowGraphModel* pbModel = dynamic_cast<PBDataFlowGraphModel*>(&graphModel);

    std::vector<NodeId> selectedNodes;
    GroupId explicitGroupId = InvalidGroupId;
    for (QGraphicsItem* it : mpDataFlowGraphicsScene->selectedItems()) {
        if (auto *groupItem = dynamic_cast<PBNodeGroupGraphicsItem*>(it)) {
            explicitGroupId = groupItem->groupId();
            break;
        }
    }

    if (explicitGroupId != InvalidGroupId && pbModel) {
        if (const PBNodeGroup* grp = pbModel->getGroup(explicitGroupId)) {
            for (auto nid : grp->nodes())
                selectedNodes.push_back(nid);
        }
    } else {
        for (QGraphicsItem* it : mpDataFlowGraphicsScene->selectedItems()) {
            if (auto *n = dynamic_cast<QtNodes::NodeGraphicsObject*>(it)) {
                selectedNodes.push_back(n->nodeId());
            }
        }
    }

    if (!selectedNodes.empty() && pbModel) {
        // Check if all selected nodes belong to the same group
        GroupId groupId = InvalidGroupId;
        bool allInSameGroup = true;
        for (auto nid : selectedNodes) {
            GroupId g = pbModel->getPBNodeGroup(nid);
            if (g == InvalidGroupId) { allInSameGroup = false; break; }
            if (groupId == InvalidGroupId) groupId = g;
            else if (groupId != g) { allInSameGroup = false; break; }
        }

        if (allInSameGroup && groupId != InvalidGroupId) {
            const PBNodeGroup* grp = pbModel->getGroup(groupId);
            bool treatAsWholeGroup = false;
            if (explicitGroupId != InvalidGroupId) {
                treatAsWholeGroup = true;
            } else if (grp && selectedNodes.size() == static_cast<size_t>(grp->nodes().size())) {
                treatAsWholeGroup = true;
            }

            if (treatAsWholeGroup) {
                if (auto *pbScene = dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene)) {
                    if (const PBNodeGroup* group = pbModel->getGroup(groupId)) {
                        mpDataFlowGraphicsScene->undoStack().push(
                            new GroupDeleteCommand(pbScene, pbModel, *group));
                        return;
                    }
                }
            }
        }
    }

    // Fallback: default deletion for ordinary selections
    mpDataFlowGraphicsScene->undoStack().push(new PBDeleteCommand(dynamic_cast<PBDataFlowGraphicsScene*>(mpDataFlowGraphicsScene)));
}
