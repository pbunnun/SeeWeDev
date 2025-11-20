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
#include <QJsonObject>
#include <QPointF>
#include <vector>

#include <QtNodes/Definitions>

namespace QtNodes {
    class BasicGraphicsScene;
}

class PBDataFlowGraphModel;
using GroupId = unsigned int;

class GroupPasteCommand : public QUndoCommand
{
public:
    GroupPasteCommand(QtNodes::BasicGraphicsScene* scene,
                      PBDataFlowGraphModel* model,
                      QJsonObject const &sceneJson,
                      QPointF pastePos);

    void undo() override;
    void redo() override;

private:
    QtNodes::BasicGraphicsScene* mScene;
    PBDataFlowGraphModel* mModel;
    QJsonObject mSceneJson; // nodes, connections, optional group
    QPointF mPastePos;

    std::vector<QtNodes::NodeId> mCreatedNodeIds;
    std::vector<QtNodes::ConnectionId> mCreatedConnections;
    GroupId mCreatedGroupId{static_cast<GroupId>(-1)};
    QJsonObject mCreatedGroupState;
};
