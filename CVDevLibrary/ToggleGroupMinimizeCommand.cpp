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

#include "ToggleGroupMinimizeCommand.hpp"
#include "PBDataFlowGraphModel.hpp"

ToggleGroupMinimizeCommand::ToggleGroupMinimizeCommand(QtNodes::BasicGraphicsScene *scene,
                                                       GroupId groupId,
                                                       bool oldMinimized,
                                                       bool newMinimized,
                                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _scene(scene)
    , _groupId(groupId)
    , _oldMinimized(oldMinimized)
    , _newMinimized(newMinimized)
{
    setText(newMinimized ? "Minimize group" : "Expand group");
}

void ToggleGroupMinimizeCommand::undo()
{
    if (!_scene)
        return;

    if (auto *pbModel = dynamic_cast<PBDataFlowGraphModel*>(&_scene->graphModel())) {
        pbModel->setGroupMinimized(_groupId, _oldMinimized);
    }
}

void ToggleGroupMinimizeCommand::redo()
{
    if (!_scene)
        return;

    if (auto *pbModel = dynamic_cast<PBDataFlowGraphModel*>(&_scene->graphModel())) {
        pbModel->setGroupMinimized(_groupId, _newMinimized);
    }
}

int ToggleGroupMinimizeCommand::id() const
{
    static const int idValue = qRegisterMetaType<ToggleGroupMinimizeCommand*>();
    return idValue;
}

bool ToggleGroupMinimizeCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id())
        return false;

    auto otherCmd = static_cast<const ToggleGroupMinimizeCommand *>(other);

    // Only merge if operating on the same group
    if (otherCmd->_groupId != _groupId)
        return false;

    // Collapse to the latest requested state
    _newMinimized = otherCmd->_newMinimized;
    
    // Update the text
    setText(_newMinimized ? "Minimize group" : "Expand group");
    
    return true;
}
