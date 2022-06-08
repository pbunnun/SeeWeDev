#include "FlowView.hpp"

#include <QtWidgets/QGraphicsScene>

#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtWidgets/QMenu>

#include <QtCore/QRectF>
#include <QtCore/QPointF>

#include <QtOpenGL>
#include <QtWidgets>

#include <QDebug>
#include <iostream>
#include <cmath>

#include "FlowScene.hpp"
#include "DataModelRegistry.hpp"
#include "Node.hpp"
#include "NodeGraphicsObject.hpp"
#include "ConnectionGraphicsObject.hpp"
#include "StyleCollection.hpp"

using QtNodes::FlowView;
using QtNodes::FlowScene;

FlowView::
FlowView(QWidget *parent)
  : QGraphicsView(parent)
  , _clearSelectionAction(Q_NULLPTR)
  , _deleteSelectionAction(Q_NULLPTR)
  , _undoAction(Q_NULLPTR)
  , _redoAction(Q_NULLPTR)
  , _scene(Q_NULLPTR)
{
  setDragMode(QGraphicsView::ScrollHandDrag);
  setRenderHint(QPainter::Antialiasing);

  auto const &flowViewStyle = StyleCollection::flowViewStyle();

  setBackgroundBrush(flowViewStyle.BackgroundColor);

  //setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  //setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  setCacheMode(QGraphicsView::CacheBackground);
  setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

  //setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
}


FlowView::
FlowView(FlowScene *scene, QWidget *parent)
  : FlowView(parent)
{
  setScene(scene);
}


QAction*
FlowView::
clearSelectionAction() const
{
  return _clearSelectionAction;
}


QAction*
FlowView::
deleteSelectionAction() const
{
  return _deleteSelectionAction;
}


void
FlowView::
addAnchor(int index)
{
  qreal x1, y1, x2, y2;
  sceneRect().getCoords(&x1, &y1, &x2, &y2);

  Anchor a;
  a.position = QPointF((x2+x1)*0.5, (y1+y2)*0.5);
  a.scale = 10;

  _scene->_anchors[index] = a;
}


void
FlowView::
goToAnchor(int index)
{
  qreal x1, y1, x2, y2;
  sceneRect().getCoords(&x1, &y1, &x2, &y2);
  QPointF currentPosition = QPointF((x2+x1)*0.5, (y1+y2)*0.5);

  QPointF diff = _scene->_anchors[index].position - currentPosition;

  setSceneRect(sceneRect().translated(diff.x(), diff.y()));
}


void
FlowView::
setScene(FlowScene *scene)
{
  _scene = scene;
  QGraphicsView::setScene(_scene);

  // setup actions
  delete _clearSelectionAction;
  _clearSelectionAction = new QAction(QStringLiteral("Clear Selection"), this);
  _clearSelectionAction->setShortcut(Qt::Key_Escape);
  connect(_clearSelectionAction, &QAction::triggered, _scene, &QGraphicsScene::clearSelection);
  addAction(_clearSelectionAction);

  delete _deleteSelectionAction;
  _deleteSelectionAction = new QAction(QStringLiteral("Delete Selection"), this);
  _deleteSelectionAction->setShortcut(Qt::Key_Delete);
  //_deleteSelectionAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  connect(_deleteSelectionAction, &QAction::triggered, this, &FlowView::deleteSelectedNodes);
  addAction(_deleteSelectionAction);

  delete _undoAction;
  _undoAction = new QAction(QStringLiteral("Undo"), this);
  //_undoAction->setShortcut(QKeySequence(tr("Ctrl+Z")));
  //_undoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  connect(_undoAction, &QAction::triggered, _scene, &FlowScene::Undo);
  addAction(_undoAction);

  delete _redoAction;
  _redoAction = new QAction(QStringLiteral("Redo"), this);
  //_redoAction->setShortcut(QKeySequence(tr("Ctrl+Y")));
  //_redoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  connect(_redoAction, &QAction::triggered, _scene, &FlowScene::Redo);
  addAction(_redoAction);

  for( int i = 0; i < 10; i++ )
  {
      QAction * anchorAct = new QAction(QStringLiteral("Add Anchor"), this);
      QString sequenceString = QString("Ctrl+") + QString::number(i);
      anchorAct->setShortcut(QKeySequence(sequenceString));
      anchorAct->setShortcutContext(Qt::WidgetWithChildrenShortcut);
      connect(anchorAct, &QAction::triggered, _scene, [this, i]() {
          addAnchor(i);
      });
      addAction(anchorAct);
      _anchorActions.push_back(anchorAct);

      QAction * gotoanchorAct = new QAction(QStringLiteral("Go to Anchor"), this);
      gotoanchorAct->setShortcut(QKeySequence(tr(std::to_string(i).c_str())));
      gotoanchorAct->setShortcutContext(Qt::WidgetWithChildrenShortcut);
      connect(gotoanchorAct, &QAction::triggered, _scene, [this, i]() {
          goToAnchor(i);
      });
      addAction(gotoanchorAct);
      _anchorActions.push_back(gotoanchorAct);
  }
}


void
FlowView::
contextMenuEvent(QContextMenuEvent *event)
{
  if (itemAt(event->pos()))
  {
    QGraphicsView::contextMenuEvent(event);
    return;
  }

  QMenu modelMenu;

  auto skipText = QStringLiteral("skip me");

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
  for (auto const &cat : _scene->registry().categories())
  {
    auto item = new QTreeWidgetItem(treeView);
    item->setText(0, cat);
    item->setData(0, Qt::UserRole, skipText);
    topLevelItems[cat] = item;
  }

  for (auto const &assoc : _scene->registry().registeredModelsCategoryAssociation())
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

    auto type = _scene->registry().create(modelName);

    if (type)
    {
      auto& node = _scene->createNode(std::move(type));

      node.nodeDataModel()->late_constructor();

      QPoint pos = event->pos();

      QPointF posView = this->mapToScene(pos);

      node.nodeGraphicsObject().setPos(posView);

      _scene->nodePlaced(node);

      _scene->UpdateHistory();
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


void
FlowView::
wheelEvent(QWheelEvent *event)
{
  QPoint delta = event->angleDelta();

  if (delta.y() == 0)
  {
    event->ignore();
    return;
  }

  double const d = delta.y() / std::abs(delta.y());

  if (d > 0.0)
    scaleUp();
  else
    scaleDown();
}


void
FlowView::
scaleUp()
{
  double const step   = 1.2;
  double const factor = std::pow(step, 1.0);

  QTransform t = transform();

  if (t.m11() > 2.0)
    return;

  scale(factor, factor);
}


void
FlowView::
scaleDown()
{
  double const step   = 1.2;
  double const factor = std::pow(step, -1.0);

  QTransform t = transform();
  if (t.m11() < 0.2 )
      return;

  scale(factor, factor);
}


void
FlowView::
deleteSelectedNodes()
{
  // Delete the selected connections first, ensuring that they won't be
  // automatically deleted when selected nodes are deleted (deleting a node
  // deletes some connections as well)
  for (QGraphicsItem * item : _scene->selectedItems())
  {
    if (auto c = qgraphicsitem_cast<ConnectionGraphicsObject*>(item))
      _scene->deleteConnection(c->connection());
  }

  // Delete the nodes; this will delete many of the connections.
  // Selected connections were already deleted prior to this loop, otherwise
  // qgraphicsitem_cast<NodeGraphicsObject*>(item) could be a use-after-free
  // when a selected connection is deleted by deleting the node.
  for (QGraphicsItem * item : _scene->selectedItems())
  {
    if (auto n = qgraphicsitem_cast<NodeGraphicsObject*>(item))
      _scene->removeNode(n->node());
  }

  _scene->UpdateHistory();
}


void
FlowView::
copySelectedNodes()
{
  if( !_scene->selectedNodes().empty() )
    copy();
}


void
FlowView::
cutSelectedNodes()
{
  if( !_scene->selectedNodes().empty() )
    cut();
}


void
FlowView::
keyPressEvent(QKeyEvent *event)
{
  switch (event->key())
  {
    case Qt::Key_Shift:
      setDragMode(QGraphicsView::RubberBandDrag);
      break;

    case Qt::Key_C:
      if (event->modifiers() & Qt::ControlModifier) {
          copy();
          return;
      }
      break;

    case Qt::Key_X:
      if (event->modifiers() & Qt::ControlModifier) {
          cut();
          return;
      }
      break;

    case Qt::Key_V:
      if (event->modifiers() & Qt::ControlModifier) {
          paste();
          return;
      }
      break;

    default:
      break;
  }

  QGraphicsView::keyPressEvent(event);
}


void
FlowView::
keyReleaseEvent(QKeyEvent *event)
{
  switch (event->key())
  {
    case Qt::Key_Shift:
      setDragMode(QGraphicsView::ScrollHandDrag);
      break;

    default:
      break;
  }
  QGraphicsView::keyReleaseEvent(event);
}


void
FlowView::
mousePressEvent(QMouseEvent *event)
{
  QGraphicsView::mousePressEvent(event);
  if (event->button() == Qt::LeftButton)
  {
    _clickPos = mapToScene(event->pos());
  }
}


void
FlowView::
mouseMoveEvent(QMouseEvent *event)
{
  QGraphicsView::mouseMoveEvent(event);
  if (scene()->mouseGrabberItem() == nullptr && event->buttons() == Qt::LeftButton)
  {
    // Make sure shift is not being pressed
    if ((event->modifiers() & Qt::ShiftModifier) == 0)
    {
      QPointF difference = _clickPos - mapToScene(event->pos());
      setSceneRect(sceneRect().translated(difference.x(), difference.y()));
    }
  }
}


void
FlowView::
drawBackground(QPainter* painter, const QRectF& r)
{
  QGraphicsView::drawBackground(painter, r);

  auto drawGrid =
    [&](double gridStep)
    {
      QRect   windowRect = rect();
      QPointF tl = mapToScene(windowRect.topLeft());
      QPointF br = mapToScene(windowRect.bottomRight());

      double left   = std::floor(tl.x() / gridStep - 0.5);
      double right  = std::floor(br.x() / gridStep + 1.0);
      double bottom = std::floor(tl.y() / gridStep - 0.5);
      double top    = std::floor (br.y() / gridStep + 1.0);

      // vertical lines
      for (int xi = int(left); xi <= int(right); ++xi)
      {
        QLineF line(xi * gridStep, bottom * gridStep,
                    xi * gridStep, top * gridStep );

        painter->drawLine(line);
      }

      // horizontal lines
      for (int yi = int(bottom); yi <= int(top); ++yi)
      {
        QLineF line(left * gridStep, yi * gridStep,
                    right * gridStep, yi * gridStep );
        painter->drawLine(line);
      }
    };

  auto const &flowViewStyle = StyleCollection::flowViewStyle();

  QPen pfine(flowViewStyle.FineGridColor, 1.0);

  painter->setPen(pfine);
  drawGrid(15);

  QPen p(flowViewStyle.CoarseGridColor, 1.0);

  painter->setPen(p);
  drawGrid(150);
}


void
FlowView::
showEvent(QShowEvent *event)
{
  _scene->setSceneRect(this->rect());
  QGraphicsView::showEvent(event);
}


FlowScene *
FlowView::
scene()
{
  return _scene;
}


QString
FlowView::
nodeMimeType() const
{
  return "application/x-nodeeditor-nodes";
}


void
FlowView::
copy()
{
  QClipboard *clipboard = QApplication::clipboard();
  QMimeData *mimeData = new QMimeData();

  QByteArray data = scene()->copyNodes(scene()->selectedNodes());
  mimeData->setData(nodeMimeType(), data);
  mimeData->setText(data);

  clipboard->setMimeData(mimeData);
  _bPaste = true;
}


void
FlowView::
cut()
{
  copy();
  deleteSelectedNodes();
}


void
FlowView::
pasteNodes()
{
  paste();
}


void
FlowView::
paste()
{
  const QClipboard *clipboard = QApplication::clipboard();
  const QMimeData *mimeData = clipboard->mimeData();

  if (mimeData->hasFormat(nodeMimeType()))
  {
     scene()->pasteNodes(mimeData->data(nodeMimeType()), mapToScene(QWidget::mapFromGlobal(QCursor::pos())));
  }
  else if (mimeData->hasText())
  {
     scene()->pasteNodes(mimeData->text().toUtf8(), mapToScene(QWidget::mapFromGlobal(QCursor::pos())));
  }
}
