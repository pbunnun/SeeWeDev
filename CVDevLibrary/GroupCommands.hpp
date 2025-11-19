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
