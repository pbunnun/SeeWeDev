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
