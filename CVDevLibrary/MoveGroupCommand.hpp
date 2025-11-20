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

#include <QUndoCommand>
#include <QtNodes/internal/Definitions.hpp>
#include <map>
#include <QPointF>
#include "PBNodeGroup.hpp"

namespace QtNodes {
class BasicGraphicsScene;
}

/**
 * @brief Undo command for moving a node group with all its member nodes.
 *
 * Records original and new positions for all nodes in a group and applies them on
 * undo/redo. Supports snap-to-grid when moving groups.
 */
class MoveGroupCommand : public QUndoCommand
{
public:
    MoveGroupCommand(QtNodes::BasicGraphicsScene *scene,
                     GroupId groupId,
                     const std::map<QtNodes::NodeId, QPointF> &oldPositions,
                     const std::map<QtNodes::NodeId, QPointF> &newPositions,
                     QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override;
    bool mergeWith(const QUndoCommand *other) override;

private:
    QtNodes::BasicGraphicsScene *_scene{nullptr};
    GroupId _groupId;

    // Maps node id -> position
    std::map<QtNodes::NodeId, QPointF> _oldPositions;
    std::map<QtNodes::NodeId, QPointF> _newPositions;
    
    // Saved top-left position of the group (for minimized state)
    QPointF _oldSavedTopLeft;
    QPointF _newSavedTopLeft;
};
