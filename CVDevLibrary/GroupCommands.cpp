#include "GroupCommands.hpp"
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtNodes/DataFlowGraphModel>
#include <QtCore/QJsonArray>

GroupCreateCommand::GroupCreateCommand(PBDataFlowGraphicsScene *scene,
                                       PBDataFlowGraphModel *model,
                                       const QString &groupName,
                                       const std::set<NodeId> &nodes)
    : mpScene(scene)
    , mpModel(model)
    , mGroupName(groupName)
    , mNodeIds(nodes)
{
    setText("Group Nodes");
}

void GroupCreateCommand::undo()
{
    if (!mpModel || !mpScene)
        return;

    if (mGroupId != InvalidGroupId) {
        // Get group before dissolving to update connections
        const PBNodeGroup* group = mpModel->getGroup(mGroupId);
        if (group) {
            // Update all connections for nodes in this group
            for (NodeId nodeId : group->nodes()) {
                auto allConnections = mpModel->allConnectionIds(nodeId);
                for (const auto& connectionId : allConnections) {
                    if (auto* cgo = mpScene->connectionGraphicsObject(connectionId)) {
                        cgo->move();
                    }
                }
            }
        }
        mpModel->dissolveGroup(mGroupId);
    }
}

void GroupCreateCommand::redo()
{
    if (!mpModel || !mpScene)
        return;

    if (mGroupId == InvalidGroupId)
    {
        mGroupId = mpModel->createGroup(mGroupName, mNodeIds);
        if (mGroupId == InvalidGroupId)
            return;

        if (const PBNodeGroup *group = mpModel->getGroup(mGroupId))
            mGroupState = *group;
    }
    else
    {
        mpModel->restoreGroup(mGroupState);
    }
    
    // Update all connections for nodes in this group
    for (NodeId nodeId : mNodeIds) {
        auto allConnections = mpModel->allConnectionIds(nodeId);
        for (const auto& connectionId : allConnections) {
            if (auto* cgo = mpScene->connectionGraphicsObject(connectionId)) {
                cgo->move();
            }
        }
    }
}

GroupDissolveCommand::GroupDissolveCommand(PBDataFlowGraphicsScene *scene,
                                           PBDataFlowGraphModel *model,
                                           const PBNodeGroup &group)
    : mpScene(scene)
    , mpModel(model)
    , mGroupState(group)
{
    setText("Ungroup Nodes");
}

void GroupDissolveCommand::undo()
{
    if (!mpModel || !mpScene)
        return;

    mpModel->restoreGroup(mGroupState);
    
    // Update all connections for nodes in this group
    for (NodeId nodeId : mGroupState.nodes()) {
        auto allConnections = mpModel->allConnectionIds(nodeId);
        for (const auto& connectionId : allConnections) {
            if (auto* cgo = mpScene->connectionGraphicsObject(connectionId)) {
                cgo->move();
            }
        }
    }
}

void GroupDissolveCommand::redo()
{
    if (!mpModel || !mpScene)
        return;

    if (mGroupState.id() != InvalidGroupId) {
        // Update all connections for nodes before dissolving
        for (NodeId nodeId : mGroupState.nodes()) {
            auto allConnections = mpModel->allConnectionIds(nodeId);
            for (const auto& connectionId : allConnections) {
                if (auto* cgo = mpScene->connectionGraphicsObject(connectionId)) {
                    cgo->move();
                }
            }
        }
        mpModel->dissolveGroup(mGroupState.id());
    }
}

GroupDeleteCommand::GroupDeleteCommand(PBDataFlowGraphicsScene *scene,
                                       PBDataFlowGraphModel *model,
                                       const PBNodeGroup &group)
    : mpScene(scene)
    , mpModel(model)
    , mGroupState(group)
{
    setText("Delete Group and Members");

    if (!mpModel)
        return;

    // Serialize nodes and connections for undo
    QJsonArray nodesJsonArray;
    QJsonArray connJsonArray;

    for (NodeId nodeId : mGroupState.nodes()) {
        nodesJsonArray.append(mpModel->saveNode(nodeId));

        for (auto const &cid : mpModel->allConnectionIds(nodeId)) {
            connJsonArray.append(QtNodes::toJson(cid));
        }
    }

    mSceneJson["nodes"] = nodesJsonArray;
    mSceneJson["connections"] = connJsonArray;
}

void GroupDeleteCommand::undo()
{
    if (!mpModel || !mpScene)
        return;

    // Restore nodes
    QJsonArray nodesJsonArray = mSceneJson["nodes"].toArray();
    for (QJsonValue nodeVal : nodesJsonArray) {
        QJsonObject obj = nodeVal.toObject();
        mpModel->loadNode(obj);
    }

    // Restore connections
    QJsonArray connJsonArray = mSceneJson["connections"].toArray();
    for (QJsonValue connVal : connJsonArray) {
        QJsonObject connObj = connVal.toObject();
        QtNodes::ConnectionId cid = QtNodes::fromJson(connObj);
        mpModel->addConnection(cid);
    }

    // Restore group
    mpModel->restoreGroup(mGroupState);

    // Update connection graphics
    for (NodeId nid : mGroupState.nodes()) {
        for (auto const &cid : mpModel->allConnectionIds(nid)) {
            if (auto* cgo = mpScene->connectionGraphicsObject(cid)) {
                cgo->move();
            }
        }
    }
}

void GroupDeleteCommand::redo()
{
    if (!mpModel || !mpScene)
        return;

    // Delete connections first
    QJsonArray connJsonArray = mSceneJson["connections"].toArray();
    for (QJsonValueRef connection : connJsonArray) {
        QJsonObject connObj = connection.toObject();
        QtNodes::ConnectionId cid = QtNodes::fromJson(connObj);
        mpModel->deleteConnection(cid);
    }

    // Delete nodes
    QJsonArray nodesJsonArray = mSceneJson["nodes"].toArray();
    for (QJsonValueRef nodeVal : nodesJsonArray) {
        QJsonObject nodeObj = nodeVal.toObject();
        mpModel->deleteNode(nodeObj["id"].toInt());
    }

    // Remove group from model (dissolve)
    if (mGroupState.id() != InvalidGroupId) {
        mpModel->dissolveGroup(mGroupState.id());
    }
}
