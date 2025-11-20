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

#include "PBDataFlowGraphicsScene.hpp"
#include "PBDataFlowGraphModel.hpp"
#include "PBNodeGroup.hpp"

#include <QUndoCommand>
#include <QString>
#include <set>

class GroupCreateCommand : public QUndoCommand
{
public:
    GroupCreateCommand(PBDataFlowGraphicsScene *scene,
                       PBDataFlowGraphModel *model,
                       const QString &groupName,
                       const std::set<NodeId> &nodes);

    void undo() override;
    void redo() override;

private:
    PBDataFlowGraphicsScene *mpScene = nullptr;
    PBDataFlowGraphModel *mpModel = nullptr;
    QString mGroupName;
    std::set<NodeId> mNodeIds;
    GroupId mGroupId = InvalidGroupId;
    PBNodeGroup mGroupState;
};

class GroupDissolveCommand : public QUndoCommand
{
public:
    GroupDissolveCommand(PBDataFlowGraphicsScene *scene,
                         PBDataFlowGraphModel *model,
                         const PBNodeGroup &group);

    void undo() override;
    void redo() override;

private:
    PBDataFlowGraphicsScene *mpScene = nullptr;
    PBDataFlowGraphModel *mpModel = nullptr;
    PBNodeGroup mGroupState;
};

class GroupDeleteCommand : public QUndoCommand
{
public:
    GroupDeleteCommand(PBDataFlowGraphicsScene *scene,
                       PBDataFlowGraphModel *model,
                       const PBNodeGroup &group);

    void undo() override;
    void redo() override;

private:
    PBDataFlowGraphicsScene *mpScene = nullptr;
    PBDataFlowGraphModel *mpModel = nullptr;
    PBNodeGroup mGroupState;
    QJsonObject mSceneJson; // stores nodes and connections
};
