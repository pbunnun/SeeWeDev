#ifndef CVIMAGEDISPLAYMODEL_HPP
#define CVIMAGEDISPLAYMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "SyncData.hpp"
#include "PBImageDisplayWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class CVImageDisplayModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CVImageDisplayModel();

    virtual
    ~CVImageDisplayModel() override {}

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    bool
    resizable() const override { return true; }

    bool
    eventFilter(QObject *object, QEvent *event) override;

    static const QString _category;

    static const QString _model_name;

private:
    void display_image( );

    PBImageDisplayWidget * mpEmbeddedWidget;

    std::shared_ptr< NodeData > mpNodeData;
    std::shared_ptr< SyncData > mpSyncData;

    QPixmap _minPixmap;
};

#endif
