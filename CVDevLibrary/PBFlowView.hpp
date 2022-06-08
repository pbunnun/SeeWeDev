#ifndef PBFLOWVIEW_H
#define PBFLOWVIEW_H

#pragma once

#include "CVDevLibrary.hpp"

#include "nodes/internal/Export.hpp"
#include <nodes/FlowView>
#include <nodes/Node>
#include <QDragMoveEvent>

using QtNodes::Node;

class CVDEVSHAREDLIB_EXPORT PBFlowView : public QtNodes::FlowView
{
    Q_OBJECT
public:
    explicit PBFlowView( QWidget *parent = nullptr );
    void center_on( const Node * pNode );
    void center_on( QPointF & center_pos );

protected Q_SLOTS:
    void contextMenuEvent( QContextMenuEvent *event ) override;

    void dragMoveEvent( QDragMoveEvent *event ) override;

    void dropEvent( QDropEvent *event ) override;
};

#endif // PBFLOWVIEW_H
