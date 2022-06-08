#include "NodeDataModel.hpp"

#include "StyleCollection.hpp"

using QtNodes::NodeDataModel;
using QtNodes::NodeStyle;

NodeDataModel::
NodeDataModel()
  : _minimize(false), _enable(false), _draw_entries(false), _lock_position(false), _draw_connection_point(true),
    _nodeStyle(StyleCollection::nodeStyle()), _minPixmap(":NodeEditor.png")
{
  // Derived classes can initialize specific style here
}


QJsonObject
NodeDataModel::
save() const
{
  QJsonObject modelJson;

  modelJson["name"] = name();

  QJsonObject params;
  params["minimize"] = isMinimize();
  params["enable"] = isEnable();
  params["draw_entries"] = isDrawEntries();
  params["lock_position"] = isLockPosition();

  modelJson["params"] = params;
  return modelJson;
}


void
NodeDataModel::
restore(QJsonObject const &p)
{
  QJsonObject params = p["params"].toObject();
  QJsonValue v = params["minimize"];
  if( !v.isUndefined() )
  {
    _minimize = v.toBool();
  }

  v = params["enable"];
  if( !v.isUndefined() )
  {
    _enable = v.toBool();
  }

  v = params["lock_position"];
  if( !v.isUndefined() )
  {
    _lock_position = v.toBool();
  }

  v = params["draw_entries"];
  if( !v.isUndefined() )
  {
    _draw_entries = v.toBool();
  }
}


NodeStyle const&
NodeDataModel::
nodeStyle() const
{
  return _nodeStyle;
}


void
NodeDataModel::
setNodeStyle(NodeStyle const& style)
{
  _nodeStyle = style;
}


void
NodeDataModel::
setToolTipText(const QString& toolTipText)
{
  _toolTipText = toolTipText;
  Q_EMIT setToolTipTextSignal(toolTipText);
}


void
NodeDataModel::
setMinimize(bool minimize)
{
    _minimize = minimize;
}


void
NodeDataModel::
setEnable(bool enable)
{
    _enable = enable;
}


void
NodeDataModel::
setDrawEntries(bool draw)
{
    _draw_entries = draw;
}

void
NodeDataModel::
setLockPosition(bool lock_position)
{
    _lock_position = lock_position;
}

void
NodeDataModel::
setDrawConnectionPoints(bool draw_connection_point)
{
    _draw_connection_point = draw_connection_point;
}
