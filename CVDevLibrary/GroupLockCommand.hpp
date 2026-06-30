//Copyright © 2025 - 2026, NECTEC, all rights reserved

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
#include <QtNodes/BasicGraphicsScene>
#include "PBDataFlowGraphModel.hpp"

class GroupLockCommand : public QUndoCommand {
public:
    GroupLockCommand(QtNodes::BasicGraphicsScene *scene,
                     GroupId groupId,
                     bool oldLocked,
                     bool newLocked,
                     QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override;
    bool mergeWith(const QUndoCommand *other) override;

private:
    QtNodes::BasicGraphicsScene *_scene{nullptr};
    GroupId _groupId{InvalidGroupId};
    bool _oldLocked{false};
    bool _newLocked{false};
};
