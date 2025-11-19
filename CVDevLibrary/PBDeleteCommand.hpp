#pragma once

#include <QUndoCommand>
#include <QtCore/QJsonObject>

class PBDataFlowGraphicsScene;
class PBDataFlowGraphModel;

class PBDeleteCommand : public QUndoCommand
{
public:
    PBDeleteCommand(PBDataFlowGraphicsScene *scene);

    void undo() override;
    void redo() override;

private:
    PBDataFlowGraphicsScene *_scene;
    QJsonObject _sceneJson;
};
