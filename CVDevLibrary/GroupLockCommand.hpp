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
