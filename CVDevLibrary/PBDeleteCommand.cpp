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

#include "PBDeleteCommand.hpp"

#include "PBDataFlowGraphicsScene.hpp"
#include "PBDataFlowGraphModel.hpp"

#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>

#include <QJsonArray>
#include <QJsonDocument>

#include <set>

using namespace QtNodes;

static void insertSerializedItems(const QJsonObject &json, PBDataFlowGraphicsScene *scene)
{
    auto &graphModel = scene->graphModel();

    QJsonArray const &nodesJsonArray = json["nodes"].toArray();

    for (QJsonValue node : nodesJsonArray) {
        QJsonObject obj = node.toObject();

        graphModel.loadNode(obj);

        auto id = obj["id"].toInt();
        scene->nodeGraphicsObject(id)->setZValue(1.0);
        scene->nodeGraphicsObject(id)->setSelected(true);
    }

    QJsonArray const &connJsonArray = json["connections"].toArray();

    for (QJsonValue connection : connJsonArray) {
        QJsonObject connJson = connection.toObject();

        ConnectionId connId = fromJson(connJson);

        // Restore the connection
        graphModel.addConnection(connId);

        scene->connectionGraphicsObject(connId)->setSelected(true);
    }
}

static void deleteSerializedItems(QJsonObject &sceneJson, PBDataFlowGraphModel &graphModel)
{
    QJsonArray connectionJsonArray = sceneJson["connections"].toArray();

    for (QJsonValueRef connection : connectionJsonArray) {
        QJsonObject connJson = connection.toObject();

        ConnectionId connId = fromJson(connJson);

        graphModel.deleteConnection(connId);
    }

    QJsonArray nodesJsonArray = sceneJson["nodes"].toArray();

    for (QJsonValueRef node : nodesJsonArray) {
        QJsonObject nodeJson = node.toObject();
        graphModel.deleteNode(nodeJson["id"].toInt());
    }
}

PBDeleteCommand::PBDeleteCommand(PBDataFlowGraphicsScene *scene)
    : _scene(scene)
{
    if (!_scene)
        return;

    auto &graphModelBase = _scene->graphModel();
    auto *pbModel = dynamic_cast<PBDataFlowGraphModel *>(&graphModelBase);
    if (!pbModel)
        return;

    QJsonArray connJsonArray;
    QJsonArray nodesJsonArray;
    QJsonArray groupsJsonArray;
    std::set<GroupId> touchedGroups;

    // Delete the selected connections first, ensuring they won't be
    // automatically deleted when selected nodes are deleted
    for (QGraphicsItem *item : _scene->selectedItems()) {
        if (auto c = qgraphicsitem_cast<ConnectionGraphicsObject *>(item)) {
            auto const &cid = c->connectionId();
            connJsonArray.append(toJson(cid));
        }
    }

    // Collect nodes and touched groups
    for (QGraphicsItem *item : _scene->selectedItems()) {
        if (auto n = qgraphicsitem_cast<NodeGraphicsObject *>(item)) {
            for (auto const &cid : pbModel->allConnectionIds(n->nodeId())) {
                connJsonArray.append(toJson(cid));
            }

            nodesJsonArray.append(pbModel->saveNode(n->nodeId()));

            auto gid = pbModel->getPBNodeGroup(n->nodeId());
            if (gid != InvalidGroupId)
                touchedGroups.insert(gid);
        }
    }

    // Serialize touched groups
    for (auto gid : touchedGroups) {
        if (const auto *group = pbModel->getGroup(gid)) {
            groupsJsonArray.append(group->save());
        }
    }

    if (connJsonArray.isEmpty() && nodesJsonArray.isEmpty())
        setObsolete(true);

    _sceneJson["nodes"] = nodesJsonArray;
    _sceneJson["connections"] = connJsonArray;
    if (!groupsJsonArray.isEmpty())
        _sceneJson["groups"] = groupsJsonArray;
}

void PBDeleteCommand::undo()
{
    if (!_scene)
        return;

    insertSerializedItems(_sceneJson, _scene);

    // Restore groups
    if (_sceneJson.contains("groups") && _sceneJson["groups"].isArray()) {
        auto &graphModelBase = _scene->graphModel();
        auto *pbModel = dynamic_cast<PBDataFlowGraphModel *>(&graphModelBase);
        if (pbModel) {
            QJsonArray groupsJsonArray = _sceneJson["groups"].toArray();
            for (QJsonValue gval : groupsJsonArray) {
                if (!gval.isObject())
                    continue;
                PBNodeGroup group;
                group.load(gval.toObject());
                pbModel->restoreGroup(group);
            }
        }
    }
}

void PBDeleteCommand::redo()
{
    if (!_scene)
        return;

    auto &graphModelBase = _scene->graphModel();
    auto *pbModel = dynamic_cast<PBDataFlowGraphModel *>(&graphModelBase);
    if (!pbModel)
        return;

    deleteSerializedItems(_sceneJson, *pbModel);
}
