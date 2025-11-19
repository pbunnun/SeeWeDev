//Copyright © 2025, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include <QMimeData>
#include "PBTreeWidget.hpp"
#include <QDebug>
#include <QMouseEvent>
#include <QDrag>
#include <QApplication>

PBTreeWidget::PBTreeWidget(QWidget *parent) : QTreeWidget(parent)
{

}

void
PBTreeWidget::
mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // Store the position for drag distance calculation
        mDragStartPosition = event->pos();
    }
    
    // Let base class handle the event (selection, expansion, etc.)
    QTreeWidget::mousePressEvent(event);
}

void
PBTreeWidget::
mouseMoveEvent(QMouseEvent *event)
{
    // Only start drag if left button is pressed and moved beyond threshold
    if (!(event->buttons() & Qt::LeftButton))
    {
        QTreeWidget::mouseMoveEvent(event);
        return;
    }
    
    // Check if mouse moved beyond drag start distance
    if ((event->pos() - mDragStartPosition).manhattanLength() 
        < QApplication::startDragDistance())
    {
        QTreeWidget::mouseMoveEvent(event);
        return;
    }
    
    // Now initiate drag operation
    auto dragItem = this->itemAt(mDragStartPosition);
    if (dragItem)
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

void
PBTreeWidget::
dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);

    event->setDropAction(Qt::MoveAction);
    event->accept();
}
