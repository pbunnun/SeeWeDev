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

#include "MoveGroupCommand.hpp"
#include "PBDataFlowGraphicsScene.hpp"
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/AbstractGraphModel.hpp>

#include <QVariant>

MoveGroupCommand::MoveGroupCommand(QtNodes::BasicGraphicsScene *scene,
                                   GroupId groupId,
                                   const std::map<QtNodes::NodeId, QPointF> &oldPositions,
                                   const std::map<QtNodes::NodeId, QPointF> &newPositions,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , _scene(scene)
    , _groupId(groupId)
    , _oldPositions(oldPositions)
    , _newPositions(newPositions)
{
    setText("Move group");
    
    // Capture the group's savedTopLeft position before and after the move
    // This is critical for minimized groups which anchor their visual position to this saved point
    if (auto *pbScene = dynamic_cast<PBDataFlowGraphicsScene*>(scene)) {
        if (auto *groupItem = pbScene->getGroupGraphicsItem(groupId)) {
            // At construction time, we're at the "after move" state
            // We need to calculate what the old savedTopLeft was
            _newSavedTopLeft = groupItem->savedTopLeft();
            
            // Calculate the delta from old to new node positions
            if (!oldPositions.empty() && !newPositions.empty()) {
                auto oldIt = oldPositions.begin();
                auto newIt = newPositions.find(oldIt->first);
                if (newIt != newPositions.end()) {
                    QPointF delta = oldIt->second - newIt->second;
                    _oldSavedTopLeft = _newSavedTopLeft + delta;
                }
            }
        }
    }
}

void MoveGroupCommand::undo()
{
    if (!_scene)
        return;

    auto &model = _scene->graphModel();

    for (const auto &p : _oldPositions) {
        const QtNodes::NodeId nodeId = p.first;
        const QPointF pos = p.second;

        // Apply old position to the node
        if (auto *ngo = _scene->nodeGraphicsObject(nodeId)) {
            ngo->setPos(pos);
        }
        
        // Update the model with the old position
        model.setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    }
    
    // Restore the group's savedTopLeft anchor for minimized state
    if (auto *pbScene = dynamic_cast<PBDataFlowGraphicsScene*>(_scene)) {
        if (auto *groupItem = pbScene->getGroupGraphicsItem(_groupId)) {
            groupItem->setSavedTopLeft(_oldSavedTopLeft);
            groupItem->update();
        }
    }
}

void MoveGroupCommand::redo()
{
    if (!_scene)
        return;

    auto &model = _scene->graphModel();

    for (const auto &p : _newPositions) {
        const QtNodes::NodeId nodeId = p.first;
        const QPointF pos = p.second;

        // Apply new position to the node
        if (auto *ngo = _scene->nodeGraphicsObject(nodeId)) {
            ngo->setPos(pos);
        }
        
        // Update the model with the new position
        model.setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    }
    
    // Restore the group's savedTopLeft anchor for minimized state
    if (auto *pbScene = dynamic_cast<PBDataFlowGraphicsScene*>(_scene)) {
        if (auto *groupItem = pbScene->getGroupGraphicsItem(_groupId)) {
            groupItem->setSavedTopLeft(_newSavedTopLeft);
            groupItem->update();
        }
    }
}

int MoveGroupCommand::id() const
{
    // Arbitrary unique id for this command type
    static const int idValue = qRegisterMetaType<MoveGroupCommand*>();
    return idValue;
}

bool MoveGroupCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id())
        return false;

    auto otherCmd = static_cast<const MoveGroupCommand *>(other);

    // Merge only if operating on the same group
    if (otherCmd->_groupId != _groupId)
        return false;

    // Merge only if operating on the same set of nodes
    if (otherCmd->_oldPositions.size() != _oldPositions.size())
        return false;

    for (const auto &p : _oldPositions) {
        auto it = otherCmd->_oldPositions.find(p.first);
        if (it == otherCmd->_oldPositions.end())
            return false;
    }

    // Update our newPositions to the other's newPositions (take latest)
    _newPositions = otherCmd->_newPositions;

    return true;
}
