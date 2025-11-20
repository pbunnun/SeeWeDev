#include "GroupLockCommand.hpp"

GroupLockCommand::GroupLockCommand(QtNodes::BasicGraphicsScene *scene,
                                   GroupId groupId,
                                   bool oldLocked,
                                   bool newLocked,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , _scene(scene)
    , _groupId(groupId)
    , _oldLocked(oldLocked)
    , _newLocked(newLocked)
{
    setText("Toggle group lock");
}

void GroupLockCommand::undo()
{
    if (!_scene)
        return;

    if (auto *pbModel = dynamic_cast<PBDataFlowGraphModel*>(&_scene->graphModel())) {
        pbModel->setGroupLocked(_groupId, _oldLocked);
    }
}

void GroupLockCommand::redo()
{
    if (!_scene)
        return;

    if (auto *pbModel = dynamic_cast<PBDataFlowGraphModel*>(&_scene->graphModel())) {
        pbModel->setGroupLocked(_groupId, _newLocked);
    }
}

int GroupLockCommand::id() const
{
    static const int idValue = qRegisterMetaType<GroupLockCommand*>();
    return idValue;
}

bool GroupLockCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id())
        return false;

    auto otherCmd = static_cast<const GroupLockCommand *>(other);

    // Only merge if operating on the same group
    if (otherCmd->_groupId != _groupId)
        return false;

    // Collapse to the latest requested state
    _newLocked = otherCmd->_newLocked;
    return true;
}
