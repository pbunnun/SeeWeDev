//Copyright © 2026, NECTEC, all rights reserved

#pragma once

#include "CVDevLibrary.hpp"
#include <QUndoCommand>
#include <QPointF>

class PBFlowGraphicsView;

class CVDEVSHAREDLIB_EXPORT ViewTransformCommand : public QUndoCommand
{
public:
    ViewTransformCommand(PBFlowGraphicsView* view, const QPointF& oldCenter, double oldScale, const QPointF& newCenter, double newScale, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    PBFlowGraphicsView* mView;
    QPointF mOldCenter;
    double mOldScale;
    QPointF mNewCenter;
    double mNewScale;
};
