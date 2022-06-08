#include "PBFlowView.hpp"
#include <nodes/FlowScene>
#include <nodes/Node>
#include <QMimeData>
#include <QDebug>
#include <QMenu>
#include <QLineEdit>
#include <QWidgetAction>
#include <QTreeWidget>
#include <QHeaderView>

PBFlowView::PBFlowView(QWidget *parent) : QtNodes::FlowView(parent)
{
    setAcceptDrops(true);
}

void
PBFlowView::
dragMoveEvent(QDragMoveEvent *event)
{
    event->setDropAction(Qt::MoveAction);
    event->accept();
}


void
PBFlowView::
dropEvent(QDropEvent *event)
{
    auto type = scene()->registry().create(event->mimeData()->text());
    if(type)
    {
        auto& node = scene()->createNode(std::move(type));

        node.nodeDataModel()->late_constructor();

        QPoint pos = event->pos();

        QPointF posView = this->mapToScene(pos) - QPointF(node.nodeGeometry().width()/2, 0);

        node.nodeGraphicsObject().setPos(posView);

        Q_EMIT scene()->nodePlaced(node);

        scene()->UpdateHistory();
    }
    event->accept();
}


void
PBFlowView::
contextMenuEvent(QContextMenuEvent *event)
{
    if(!itemAt(event->pos()) || scene()->selectedItems().isEmpty())
    {
        QMenu modelMenu;

        auto skipText = QStringLiteral("skip me");
        if( _bPaste )
        {
            auto * pasteAction = new QAction(tr("Paste"), &modelMenu);
            connect(pasteAction, &QAction::triggered, this, &FlowView::pasteNodes);
            pasteAction->setIcon(QIcon(":/icons/tango/32x32/edit-paste.png"));
            modelMenu.addAction(pasteAction);
            modelMenu.addSeparator();
        }
        //Add filterbox to the context menu
        auto *txtBox = new QLineEdit(&modelMenu);

        txtBox->setPlaceholderText(QStringLiteral("Filter"));
        txtBox->setClearButtonEnabled(true);

        auto *txtBoxAction = new QWidgetAction(&modelMenu);
        txtBoxAction->setDefaultWidget(txtBox);

        modelMenu.addAction(txtBoxAction);

        //Add result treeview to the context menu
        auto *treeView = new QTreeWidget(&modelMenu);
        treeView->header()->close();

        auto *treeViewAction = new QWidgetAction(&modelMenu);
        treeViewAction->setDefaultWidget(treeView);

        modelMenu.addAction(treeViewAction);

        QMap<QString, QTreeWidgetItem*> topLevelItems;
        for (auto const &cat : scene()->registry().categories())
        {
            auto item = new QTreeWidgetItem(treeView);
            item->setText(0, cat);
            item->setData(0, Qt::UserRole, skipText);
            topLevelItems[cat] = item;
        }

        for (auto const &assoc : scene()->registry().registeredModelsCategoryAssociation())
        {
            auto parent = topLevelItems[assoc.second];
            auto item   = new QTreeWidgetItem(parent);
            item->setText(0, assoc.first);
            item->setData(0, Qt::UserRole, assoc.first);
        }

        treeView->expandAll();

        connect(treeView, &QTreeWidget::itemClicked, &modelMenu, [&](QTreeWidgetItem *item, int)
        {
            QString modelName = item->data(0, Qt::UserRole).toString();

            if (modelName == skipText)
            {
                return;
            }

            auto type = scene()->registry().create(modelName);

            if (type)
            {
                auto& node = scene()->createNode(std::move(type));

                node.nodeDataModel()->late_constructor();

                QPoint pos = event->pos();

                QPointF posView = this->mapToScene(pos);

                node.nodeGraphicsObject().setPos(posView);

                Q_EMIT scene()->nodePlaced(node);

                scene()->UpdateHistory();
            }
            else
            {
                qDebug() << "Model not found";
            }

            modelMenu.close();
        });

        //Setup filtering
        connect(txtBox, &QLineEdit::textChanged, [&](const QString &text)
        {
            for (auto& topLvlItem : topLevelItems)
            {
                bool shouldHideCategory = true;
                for (int i = 0; i < topLvlItem->childCount(); ++i)
                {
                    auto child = topLvlItem->child(i);
                    auto modelName = child->data(0, Qt::UserRole).toString();
                    const bool match = (modelName.contains(text, Qt::CaseInsensitive));
                    if( match )
                        shouldHideCategory = false;
                    child->setHidden(!match);
                }
                topLvlItem->setHidden(shouldHideCategory);
            }
        });

        // make sure the text box gets focus so the user doesn't have to click on it
        txtBox->setFocus();

        modelMenu.exec(event->globalPos());

    }
    else
    {
        QMenu modelMenu;

        auto * deleteAction = new QAction(tr("Delete"), &modelMenu);
        deleteAction->setIcon(QIcon(":/icons/tango/32x32/edit-delete.png"));
        connect(deleteAction, &QAction::triggered, this, &QtNodes::FlowView::deleteSelectedNodes);

        auto * copyAction = new QAction(tr("Copy"), &modelMenu);
        copyAction->setIcon(QIcon(":/icons/tango/32x32/edit-copy.png"));
        connect(copyAction, &QAction::triggered, this, &QtNodes::FlowView::copySelectedNodes);

        auto * cutAction = new QAction(tr("Cut"), &modelMenu);
        cutAction->setIcon(QIcon(":/icons/tango/32x32/edit-cut.png"));
        connect(cutAction, &QAction::triggered, this, &QtNodes::FlowView::cutSelectedNodes);

        modelMenu.addAction(copyAction);
        modelMenu.addAction(cutAction);
        modelMenu.addSeparator();
        modelMenu.addAction(deleteAction);

        modelMenu.exec(event->globalPos());
    }
}

void
PBFlowView::
center_on( const Node * pNode )
{
    QPointF difference(10, 10);
    int max_loop = 5;
    while( difference.manhattanLength() > 2 && max_loop-- > 0 )
    {
        auto nodeRect = pNode->nodeGraphicsObject().sceneBoundingRect();
        auto pos = QPointF( nodeRect.x() + nodeRect.width()/2., nodeRect.y() + nodeRect.height()/2. );
        /*
         * mapToScene accepts only an integer value caused precision error when the scene is in zoom-in.
         * To reduce the error, this loop is needed to call many times so that the error will convert to zero.
         */
        auto target = mapToScene( width()/2, height()/2 );
        difference = pos - target;
        setSceneRect( sceneRect().translated( difference.x(), difference.y() ) );
    }

}

void
PBFlowView::
center_on( QPointF & center_pos )
{
    auto target = mapToScene( width()/2, height()/2 );
    auto difference = center_pos - target;
    setSceneRect( sceneRect().translated( difference.x(), difference.y() ) );
}
