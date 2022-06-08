#ifndef NODEDATATIMERMODEL_HPP
#define NODEDATATIMERMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "NodeDataTimerEmbeddedWidget.hpp"


using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.


class NodeDataTimerModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    NodeDataTimerModel();

    virtual
    ~NodeDataTimerModel() override {}

    QJsonObject
    save() const override;

    void
    restore(const QJsonObject &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    QPixmap
    minPixmap() const override { return _minPixmap; }


    static const QString _category;

    static const QString _model_name;

private Q_SLOTS :

    void em_timeout();

private:

    std::shared_ptr<NodeData> mpNodeData { nullptr };
    NodeDataTimerEmbeddedWidget* mpEmbeddedWidget;

    QPixmap _minPixmap;

};

#endif // NODEDATATIMERMODEL_HPP
