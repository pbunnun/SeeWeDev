#ifndef INFORMATIONDISPLAYMODEL_HPP
#define INFORMATIONDISPLAYMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPlainTextEdit>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "InformationData.hpp"
#include "InformationDisplayEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class InformationDisplayModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    InformationDisplayModel();

    virtual
    ~InformationDisplayModel() override {}

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    bool
    resizable() const override { return true; }

    static const QString _category;

    static const QString _model_name;

private:

    InformationDisplayEmbeddedWidget * mpEmbeddedWidget;

    std::shared_ptr< InformationData > mpInformationData { nullptr };
};

#endif // INFORMATIONDISPLAYMODEL_HPP
