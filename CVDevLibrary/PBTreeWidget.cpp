#include <QMimeData>
#include "PBTreeWidget.hpp"
#include <QDebug>
#include <QMouseEvent>
#include <QDrag>

PBTreeWidget::PBTreeWidget(QWidget *parent) : QTreeWidget(parent)
{

}

void
PBTreeWidget::
mousePressEvent(QMouseEvent *event)
{
    QTreeWidget::mousePressEvent(event);

    if( event->button() == Qt::LeftButton )
    {
        auto dragItem = this->itemAt(event->pos());
        if( dragItem )
        {
            QMimeData *mime = new QMimeData;
            mime->setText(dragItem->text(0));
            QDrag *drag = new QDrag(this);
            drag->setMimeData(mime);
            drag->setPixmap(dragItem->icon(0).pixmap(32,32));
            drag->setHotSpot(QPoint(drag->pixmap().width()/2, drag->pixmap().height()/2));
            drag->exec(Qt::MoveAction);
        }
    }
}

void
PBTreeWidget::
dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);

    event->setDropAction(Qt::MoveAction);
    event->accept();
}
