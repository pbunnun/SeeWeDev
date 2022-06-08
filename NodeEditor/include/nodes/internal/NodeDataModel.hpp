#pragma once


#include <QtWidgets/QWidget>

#include "PortType.hpp"
#include "NodeData.hpp"
#include "Serializable.hpp"
#include "NodeGeometry.hpp"
#include "NodeStyle.hpp"
#include "NodePainterDelegate.hpp"
#include "Export.hpp"
#include "memory.hpp"

namespace QtNodes
{

enum class NodeValidationState
{
  Valid,
  Warning,
  Error
};

class Connection;

class StyleCollection;

class NODE_EDITOR_PUBLIC NodeDataModel
  : public QObject
  , public Serializable
{
  Q_OBJECT

public:

  NodeDataModel();

  virtual
  ~NodeDataModel() = default;

  /// Caption is used in GUI
  virtual QString
  caption() const = 0;

  /// It is possible to hide caption in GUI
  virtual bool
  captionVisible() const { return true; }

  /// Port caption is used in GUI to label individual ports
  virtual QString
  portCaption(PortType, PortIndex) const { return QString(); }

  /// It is possible to hide port caption in GUI
  virtual bool
  portCaptionVisible(PortType, PortIndex) const { return false; }

  /// Name makes this model unique
  virtual QString
  name() const = 0;

  void
  setToolTipText(const QString& toolTopText);

public:

  QJsonObject
  save() const override;

  void
  restore(QJsonObject const &p) override;

public:

  virtual
  unsigned int nPorts(PortType portType) const = 0;

  virtual
  NodeDataType dataType(PortType portType, PortIndex portIndex) const = 0;

public:

  enum class ConnectionPolicy
  {
    One,
    Many,
  };

  virtual
  ConnectionPolicy
  portOutConnectionPolicy(PortIndex) const
  {
    return ConnectionPolicy::Many;
  }

  virtual
  ConnectionPolicy
  portInConnectionPolicy(PortIndex) const
  {
    return ConnectionPolicy::One;
  }

  NodeStyle const&
  nodeStyle() const;

  void
  setNodeStyle(NodeStyle const& style);

  virtual
  void
  setMinimize(bool minimize);

  bool
  isMinimize() const { return _minimize; };

  virtual 
  void
  setEnable(bool enable);

  bool
  isEnable() const { return _enable; };

  virtual
  void
  setDrawConnectionPoints(bool);

  bool
  isDrawConnectionPoints() const { return _draw_connection_point; };

  virtual
  void
  setDrawEntries(bool draw);

  bool
  isDrawEntries() const { return _draw_entries; };

  virtual
  void
  setLockPosition(bool lock_position);

  bool
  isLockPosition() const { return _lock_position; };
public:

  /// Triggers the algorithm
  virtual
  void
  setInData(std::shared_ptr<NodeData> nodeData,
            PortIndex port) = 0;

  // Use this if portInConnectionPolicy returns ConnectionPolicy::Many
  virtual
  void
  setInData(std::shared_ptr<NodeData> nodeData,
            PortIndex port,
            const QUuid& connectionId)
  {
    Q_UNUSED(connectionId);
    setInData(nodeData, port);
  }

  virtual
  std::shared_ptr<NodeData>
  outData(PortIndex port) = 0;

  virtual
  QWidget *
  embeddedWidget() = 0;

  virtual
  QPixmap
  minPixmap() const { return _minPixmap; }

  /// Call this function when a node want to initilise somethings, eg. hardware interface, after it was added to the scene.
  virtual
  void
  late_constructor() {}

  virtual
  bool
  resizable() const { return false; }

  virtual
  NodeValidationState
  validationState() const { return NodeValidationState::Valid; }

  virtual
  QString
  validationMessage() const { return QString(""); }

  virtual
  NodePainterDelegate* painterDelegate() const { return nullptr; }

  QString
  toolTipText() const { return _toolTipText; }

public Q_SLOTS:

  virtual void
  inputConnectionCreated(Connection const&)
  {
  }

  virtual void
  inputConnectionDeleted(Connection const&)
  {
  }

  virtual void
  outputConnectionCreated(Connection const&)
  {
  }

  virtual void
  outputConnectionDeleted(Connection const&)
  {
  }

Q_SIGNALS:

  void
  dataUpdated(PortIndex index);

  void
  dataInvalidated(PortIndex index);

  void
  computingStarted();

  void
  computingFinished();

  void
  embeddedWidgetSizeUpdated();

  void
  embeddedWidgetStatusUpdated();

  void
  setToolTipTextSignal(const QString& text);

private:
  bool _minimize;

  bool _enable;

  bool _draw_entries;

  bool _lock_position;

  bool _draw_connection_point;

  NodeStyle _nodeStyle;

  QPixmap _minPixmap;

  QString _toolTipText;
};
}
