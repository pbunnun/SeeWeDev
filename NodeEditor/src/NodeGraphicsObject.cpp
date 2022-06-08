#include "NodeGraphicsObject.hpp"

#include <iostream>
#include <cstdlib>

#include <QtWidgets/QtWidgets>
#include <QtWidgets/QGraphicsEffect>

#include "ConnectionGraphicsObject.hpp"
#include "ConnectionState.hpp"

#include "FlowScene.hpp"
#include "NodePainter.hpp"

#include "Node.hpp"
#include "NodeDataModel.hpp"
#include "NodeConnectionInteraction.hpp"

#include "StyleCollection.hpp"

using QtNodes::NodeGraphicsObject;
using QtNodes::Node;
using QtNodes::FlowScene;

NodeGraphicsObject::
NodeGraphicsObject(FlowScene &scene,
                   Node& node)
  : _scene(scene)
  , _node(node)
  , _locked(false)
  , _locked_position(false)
  , _proxyWidget(nullptr)
{
  _scene.addItem(this);

  setFlag(QGraphicsItem::ItemDoesntPropagateOpacityToChildren, true);
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsFocusable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

  setCacheMode( QGraphicsItem::DeviceCoordinateCache );

  auto const &nodeStyle = node.nodeDataModel()->nodeStyle();

  {
    auto effect = new QGraphicsDropShadowEffect;
    effect->setOffset(4, 4);
    effect->setBlurRadius(20);
    effect->setColor(nodeStyle.ShadowColor);

    setGraphicsEffect(effect);
  }

  setOpacity(static_cast<qreal>(nodeStyle.Opacity));

  setAcceptHoverEvents(true);

  setZValue(0);

  embedQWidget();

  // connect to the move signals to emit the move signals in FlowScene
  auto onMoveSlot = [this] {
    _scene.nodeMoved(_node, pos());
  };
  connect(this, &QGraphicsObject::xChanged, this, onMoveSlot);
  connect(this, &QGraphicsObject::yChanged, this, onMoveSlot);
  connect(_node.nodeDataModel(), &NodeDataModel::setToolTipTextSignal, this, [this](const QString toolTipText ) {
      setToolTip(toolTipText);
  });
}


NodeGraphicsObject::
~NodeGraphicsObject()
{
  _scene.removeItem(this);
}


Node&
NodeGraphicsObject::
node()
{
  return _node;
}


Node const&
NodeGraphicsObject::
node() const
{
  return _node;
}


void
NodeGraphicsObject::
embedQWidget()
{
  NodeGeometry & geom = _node.nodeGeometry();

  if (auto w = _node.nodeDataModel()->embeddedWidget())
  {
    _proxyWidget = new QGraphicsProxyWidget(this);

    _proxyWidget->setWidget(w);

    _proxyWidget->setPreferredWidth(5);

    QSize size = w->size();
    QSize correct = size;
    correct = correct.expandedTo(geom.minimumEmbeddedSize());
    correct = correct.boundedTo(geom.maximumEmbeddedSize());
    if (size != correct )
    {
        size = correct;
        w->resize(size);
    }

    geom.recalculateSize();

/*    if (w->sizePolicy().verticalPolicy() & QSizePolicy::ExpandFlag)
    {
      // If the widget wants to use as much vertical space as possible, set it to have the geom's equivalentWidgetHeight.
      _proxyWidget->setMinimumHeight(geom.equivalentWidgetHeight());
    } */

    _proxyWidget->setMinimumSize(size);
    _proxyWidget->setMaximumSize(size);
    _proxyWidget->setPos(geom.widgetPosition());

    update();

    _proxyWidget->setOpacity(1.0);
    _proxyWidget->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
  }
}

void
NodeGraphicsObject::
set_embeddedWidgetSize(QSize widgetSize)
{
  auto & geom = _node.nodeGeometry();

  if( auto w = _node.nodeDataModel()->embeddedWidget() )
  {
     prepareGeometryChange();

     w->resize(widgetSize);
     geom.recalculateSize();

     _proxyWidget->setMinimumSize(widgetSize);
     _proxyWidget->setMaximumSize(widgetSize);

     update();
  }
}

QRectF
NodeGraphicsObject::
boundingRect() const
{
  return _node.nodeGeometry().boundingRect();
}


void
NodeGraphicsObject::
setGeometryChanged()
{
  prepareGeometryChange();
}


void
NodeGraphicsObject::
moveConnections() const
{
  NodeState const & nodeState = _node.nodeState();

  for (PortType portType: {PortType::In, PortType::Out})
  {
    auto const & connectionEntries =
      nodeState.getEntries(portType);

    for (auto const & connections : connectionEntries)
    {
      for (auto & con : connections)
        con.second->getConnectionGraphicsObject().move();
    }
  }
}


void
NodeGraphicsObject::
lock(bool locked)
{
  _locked = locked;

  setFlag(QGraphicsItem::ItemIsMovable, !locked);
  setFlag(QGraphicsItem::ItemIsFocusable, !locked);
  setFlag(QGraphicsItem::ItemIsSelectable, !locked);
}

void
NodeGraphicsObject::
lock_position(bool locked_position)
{
  _locked_position = locked_position;

  setFlag(QGraphicsItem::ItemIsMovable, !locked_position);
}


void
NodeGraphicsObject::
paint(QPainter * painter,
      QStyleOptionGraphicsItem const* option,
      QWidget* )
{
  painter->setClipRect(option->exposedRect);

  NodePainter::paint(painter, _node, _scene);
}


QVariant
NodeGraphicsObject::
itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (change == ItemPositionChange && scene())
  {
    QPointF newPos = value.toPointF();

    if (QApplication::mouseButtons() == Qt::LeftButton && _scene.IsSnap2Grid())
    {
      auto diam = _node.nodeDataModel()->nodeStyle().ConnectionPointDiameter - 1;
      qreal xV = std::floor(newPos.x()/15)*15 - diam;
      qreal yV = std::floor(newPos.y()/15)*15 - diam;

      moveConnections();
      return QPointF(xV, yV);
    }
    else
    {
      moveConnections();
      return newPos;
    }
  }
  else if(change == ItemSelectedChange && scene() )
  {
    if( value.toBool() )
        setZValue(10);
    else
        setZValue(0);

    return QGraphicsItem::itemChange(change, value);
  }
  else
  {
    return QGraphicsItem::itemChange(change, value);
  }
}


void
NodeGraphicsObject::
mousePressEvent(QGraphicsSceneMouseEvent * event)
{
  if (_locked)
    return;

  // deselect all other items after this one is selected
  if (!isSelected() &&
      !(event->modifiers() & Qt::ControlModifier))
  {
    _scene.clearSelection();
  }

  for (PortType portToCheck: {PortType::In, PortType::Out})
  {
    NodeGeometry const & nodeGeometry = _node.nodeGeometry();

    // TODO do not pass sceneTransform
    int const portIndex = nodeGeometry.checkHitScenePoint(portToCheck,
                                                    event->scenePos(),
                                                    sceneTransform());

    if (portIndex != INVALID)
    {
      NodeState const & nodeState = _node.nodeState();

      std::unordered_map<QUuid, Connection*> connections =
        nodeState.connections(portToCheck, portIndex);

      // start dragging existing connection
      if (!connections.empty() && portToCheck == PortType::In)
      {
        auto con = connections.begin()->second;

        NodeConnectionInteraction interaction(_node, *con, _scene);

        interaction.disconnect(portToCheck);
      }
      else // initialize new Connection
      {
        if (portToCheck == PortType::Out)
        {
          auto const outPolicy = _node.nodeDataModel()->portOutConnectionPolicy(portIndex);
          if (!connections.empty() &&
              outPolicy == NodeDataModel::ConnectionPolicy::One)
          {
            _scene.deleteConnection( *connections.begin()->second );
          }
        }

        // todo add to FlowScene
        auto connection = _scene.createConnection(portToCheck,
                                                  _node,
                                                  portIndex);

        _node.nodeState().setConnection(portToCheck,
                                        portIndex,
                                        *connection);

        connection->getConnectionGraphicsObject().grabMouse();
      }
    }
  }

  auto pos     = event->pos();
  auto & geom  = _node.nodeGeometry();
  auto & state = _node.nodeState();

  if (_node.nodeDataModel()->resizable() &&
      geom.resizeRect().contains(QPoint(static_cast<int>(pos.x()),
                                        static_cast<int>(pos.y()))))
  {
    state.setResizing(NodeState::RESIZING);
    if( auto w = _node.nodeDataModel()->embeddedWidget() )
    {
      _pressMousePos = event->pos();
      _pressEmbeddedWidgetSize = w->size();

      if( _scene.IsSnap2Grid() )
      {
        auto diam = _node.nodeDataModel()->nodeStyle().ConnectionPointDiameter;
        _boundingSize.setWidth(2*diam+geom.width() - w->width());
        _boundingSize.setHeight(2*diam+geom.height() - w->height());
      }
    }
  }
  else if(geom.minimizeRect().contains(QPoint(static_cast<int>(pos.x()), static_cast<int>(pos.y()))))
  {
    _node.nodeDataModel()->setMinimize(!_node.nodeDataModel()->isMinimize());
    if( !_node.nodeDataModel()->isMinimize() )
        geom.recalculateSize();
    update();
  }
  else if(geom.enableRect().contains(QPoint(static_cast<int>(pos.x()), static_cast<int>(pos.y()))))
  {
    _node.nodeDataModel()->setEnable(!_node.nodeDataModel()->isEnable());
    update();
  }
  else if(geom.lock_positionRect().contains(QPoint(static_cast<int>(pos.x()), static_cast<int>(pos.y()))))
  {
    _node.nodeDataModel()->setLockPosition(!_node.nodeDataModel()->isLockPosition());
    lock_position(_node.nodeDataModel()->isLockPosition());
    update();
  }

}


void
NodeGraphicsObject::
mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
  auto & geom  = _node.nodeGeometry();
  auto & state = _node.nodeState();

  if (state.resizing() == NodeState::RESIZING)
  {
    auto diff = event->pos() - _pressMousePos;

    if (auto w = _node.nodeDataModel()->embeddedWidget())
    {
      auto pos = event->pos().toPoint();
      prepareGeometryChange();

      const QSize size = _pressEmbeddedWidgetSize;
      QSize newSize = size + QSize(diff.x(), diff.y());

      if( _scene.IsSnap2Grid() )
      {
        auto newWidth = std::floor((_boundingSize.width() + newSize.width())/15)*15;
        auto newHeight = std::floor((_boundingSize.height() + newSize.height())/15)*15;

        newSize.setWidth(newWidth - _boundingSize.width());
        newSize.setHeight(newHeight - _boundingSize.height());
      }

      const QSize minSize = geom.minimumEmbeddedSize();
      const QSize maxSize = geom.maximumEmbeddedSize();
      if ((newSize.width() < minSize.width() && newSize.height() < minSize.height()) ||
          (newSize.width() > maxSize.width() && newSize.height() > maxSize.height()))
      {
        event->ignore();
        if( !geom.resizeRect().contains(pos) )
        {
          auto cur_pos = mapToScene(geom.resizeRect().center()).toPoint();
          cur_pos = _scene.views().first()->mapFromScene(cur_pos);
          cur_pos = _scene.views().first()->mapToGlobal(cur_pos);
          QCursor::setPos( cur_pos );
        }
        return;
      }
/*
      qDebug() << event->screenPos() << event->pos()
               << mapToScene(event->pos().toPoint())
               << _scene.views().first()->mapToGlobal(event->pos().toPoint())
               << _scene.views().first()->mapToGlobal(mapToScene(event->pos().toPoint()).toPoint())
               << _scene.views().first()->mapFromScene(mapToScene(event->pos().toPoint()))
               << _scene.views().first()->mapToGlobal(_scene.views().first()->mapFromScene(mapToScene(event->pos().toPoint())));
*/
      newSize = newSize.expandedTo(minSize);
      newSize = newSize.boundedTo(maxSize);

      w->resize(newSize);
      geom.recalculateSize();

      _proxyWidget->setMinimumSize(newSize);
      _proxyWidget->setMaximumSize(newSize);
      _proxyWidget->setPos(geom.widgetPosition());

      /*
      if( !geom.resizeRect().contains(pos) )
      {
          auto cur_pos = mapToScene(geom.resizeRect().center()).toPoint();
          cur_pos = _scene.views().first()->mapFromScene(cur_pos);
          cur_pos = _scene.views().first()->mapToGlobal(cur_pos);
          QCursor::setPos( cur_pos );
      }
      */

      update();

      moveConnections();

      event->accept();
    }
  }
  else if(state.resizing() ==  NodeState::NOT_RESIZING)
  {
    QGraphicsObject::mouseMoveEvent(event);

    if (event->lastPos() != event->pos())
      moveConnections();

    event->ignore();
  }

  QRectF r = scene()->sceneRect();

  r = r.united(mapToScene(boundingRect()).boundingRect());

  scene()->setSceneRect(r);
}


void
NodeGraphicsObject::
mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  auto & state = _node.nodeState();

  state.setResizing(NodeState::NOT_RESIZING);

  QGraphicsObject::mouseReleaseEvent(event);

  _scene.nodeMoveFinished(_node, pos());
  // position connections precisely after fast node move
  moveConnections();

  _scene.nodeClicked(node());
}


void
NodeGraphicsObject::
hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
  // bring all the colliding nodes to background
  QList<QGraphicsItem *> overlapItems = collidingItems();

  for (QGraphicsItem *item : overlapItems)
  {
    if (item->zValue() > 0.0 && !item->isSelected())
    {
      item->setZValue(0.0);
    }
  }

  // bring this node forward
  if( !isSelected() )
    setZValue(1.0);

  _node.nodeGeometry().setHovered(true);
  update();
  _scene.nodeHovered(node(), event->screenPos());
  event->accept();
}


void
NodeGraphicsObject::
hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
  _node.nodeGeometry().setHovered(false);
  update();
  _scene.nodeHoverLeft(node());
  event->accept();
}


void
NodeGraphicsObject::
hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
  auto pos    = event->pos();
  auto & geom = _node.nodeGeometry();

  if (_node.nodeDataModel()->resizable() &&
      geom.resizeRect().contains(QPoint(pos.x(), pos.y())))
  {
    setCursor(QCursor(Qt::SizeFDiagCursor));
  }
  else
  {
    setCursor(QCursor());
  }

  event->accept();
}


void
NodeGraphicsObject::
mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  QGraphicsItem::mouseDoubleClickEvent(event);

  _scene.nodeDoubleClicked(node());
}


void
NodeGraphicsObject::
contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  _scene.nodeContextMenu(node(), mapToScene(event->pos()));
}


void
NodeGraphicsObject::
move_embeddedWidget()
{
  if( _proxyWidget )
    _proxyWidget->setPos(_node.nodeGeometry().widgetPosition());
}
