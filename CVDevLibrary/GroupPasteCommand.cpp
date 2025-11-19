#include "GroupPasteCommand.hpp"
#include "PBDataFlowGraphModel.hpp"
#include "PBNodeGroup.hpp"

#include <QtNodes/internal/Definitions.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QClipboard>
#include <QApplication>

using namespace QtNodes;

static QPointF computeAverageNodePosition(const QJsonObject &sceneJson)
{
    QPointF averagePos(0, 0);

    QJsonArray nodesJsonArray = sceneJson["nodes"].toArray();
    if (nodesJsonArray.empty())
        return averagePos;

    for (QJsonValueRef node : nodesJsonArray) {
        QJsonObject nodeJson = node.toObject();
        averagePos += QPointF(nodeJson["position"].toObject()["x"].toDouble(),
                              nodeJson["position"].toObject()["y"].toDouble());
    }

    averagePos /= static_cast<double>(nodesJsonArray.size());
    return averagePos;
}

GroupPasteCommand::GroupPasteCommand(BasicGraphicsScene *scene,
                                     PBDataFlowGraphModel *model,
                                     QJsonObject const &sceneJson,
                                     QPointF pastePos)
    : mScene(scene)
    , mModel(model)
    , mSceneJson(sceneJson)
    , mPastePos(pastePos)
{
    setText("Paste Group-Aware");
}

void GroupPasteCommand::redo()
{
    if (!mModel || !mScene)
        return;

    // Prepare mapping oldId -> newId
    std::unordered_map<int, NodeId> mapNodeIds;

    QJsonArray nodesJsonArray = mSceneJson["nodes"].toArray();
    if (nodesJsonArray.empty())
        return;

    // Compute average position and offset nodes so paste position is centered
    QPointF averagePos = computeAverageNodePosition(mSceneJson);
    QPointF diff = mPastePos - averagePos;

    // Create nodes from JSON using new IDs
    for (QJsonValueRef nodeVal : nodesJsonArray) {
        QJsonObject nodeObj = nodeVal.toObject();

        int oldId = nodeObj["id"].toInt();
        // Allocate a new unique NodeId from the scene's graph model
        QtNodes::AbstractGraphModel &graphModel = mScene->graphModel();
        NodeId newId = graphModel.newNodeId();
        mapNodeIds[oldId] = newId;

        // Replace id and shift position
        nodeObj["id"] = static_cast<qint64>(newId);
        if (nodeObj.contains("position")) {
            QJsonObject pos = nodeObj["position"].toObject();
            double x = pos["x"].toDouble() + diff.x();
            double y = pos["y"].toDouble() + diff.y();
            QJsonObject newPos;
            newPos["x"] = x;
            newPos["y"] = y;
            nodeObj["position"] = newPos;
        }

        // Use model->loadNode to restore full node state (this will create the delegate and emit nodeCreated)
        mModel->loadNode(nodeObj);
        mCreatedNodeIds.push_back(newId);
    }

    // Create connections with remapped node ids
    QJsonArray connJsonArray = mSceneJson["connections"].toArray();
    for (QJsonValueRef connVal : connJsonArray) {
        QJsonObject connObj = connVal.toObject();
        ConnectionId oldCid = fromJson(connObj);

        // Map node ids
        ConnectionId newCid{mapNodeIds[oldCid.outNodeId], oldCid.outPortIndex, mapNodeIds[oldCid.inNodeId], oldCid.inPortIndex};
        mModel->addConnection(newCid);
        mCreatedConnections.push_back(newCid);
    }

    // If group metadata present, create group in model for pasted nodes
    if (mSceneJson.contains("group") && mSceneJson["group"].isObject()) {
        QJsonObject groupObj = mSceneJson["group"].toObject();

        // Build set of new node ids corresponding to group's node list
        QSet<int> oldNodeSet;
        QJsonArray oldNodeIds = groupObj["nodes"].toArray();
        for (QJsonValue v : oldNodeIds) oldNodeSet.insert(v.toInt());

        std::set<NodeId> newNodeSet;
        for (auto const &p : mapNodeIds) {
            if (oldNodeSet.contains(p.first))
                newNodeSet.insert(p.second);
        }

        if (!newNodeSet.empty()) {
            QString name = groupObj.value("name").toString(QString("Group"));
            GroupId gid = mModel->createGroup(name, newNodeSet);
            mCreatedGroupId = gid;

            // Apply color and minimized state if present
            if (groupObj.contains("color")) {
                QColor c(groupObj.value("color").toString());
                mModel->setGroupColor(gid, c);
            }
            if (groupObj.contains("minimized")) {
                bool mini = groupObj.value("minimized").toBool(false);
                mModel->setGroupMinimized(gid, mini);
            }

            // Save created group state for undo
            if (const PBNodeGroup* g = mModel->getGroup(gid))
                mCreatedGroupState = g->save();
        }
    }

    // Update connection graphics to ensure geometry recomputed
    for (auto nid : mCreatedNodeIds) {
        auto allConn = mModel->allConnectionIds(nid);
        for (const auto &cid : allConn) {
            if (auto* cgo = mScene->connectionGraphicsObject(cid)) {
                cgo->move();
                cgo->update();
            }
        }
    }
}

void GroupPasteCommand::undo()
{
    if (!mModel || !mScene)
        return;

    // Remove connections first
    for (auto const &cid : mCreatedConnections) {
        mModel->deleteConnection(cid);
    }

    // Remove nodes
    for (auto nid : mCreatedNodeIds) {
        mModel->deleteNode(nid);
    }

    // Remove created group if any
    if (mCreatedGroupId != static_cast<GroupId>(-1)) {
        mModel->dissolveGroup(mCreatedGroupId);
    }
}
