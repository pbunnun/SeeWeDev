//Copyright © 2026, NECTEC, all rights reserved

#include "ViewTransformCommand.hpp"
#include "PBFlowGraphicsView.hpp"

ViewTransformCommand::ViewTransformCommand(PBFlowGraphicsView* view, const QPointF& oldCenter, double oldScale, const QPointF& newCenter, double newScale, QUndoCommand* parent)
    : QUndoCommand(parent)
    , mView(view)
    , mOldCenter(oldCenter)
    , mOldScale(oldScale)
    , mNewCenter(newCenter)
    , mNewScale(newScale)
{
    setText(QObject::tr("Pan/Zoom View"));
}

void ViewTransformCommand::undo()
{
    if (mView) {
        mView->setProperty("isUndoingRedoing", true);
        mView->resetTransform();
        mView->scale(mOldScale, mOldScale);
        mView->centerOn(mOldCenter);
        mView->setProperty("isUndoingRedoing", false);
    }
}

void ViewTransformCommand::redo()
{
    if (mView) {
        mView->setProperty("isUndoingRedoing", true);
        mView->resetTransform();
        mView->scale(mNewScale, mNewScale);
        mView->centerOn(mNewCenter);
        mView->setProperty("isUndoingRedoing", false);
    }
}
