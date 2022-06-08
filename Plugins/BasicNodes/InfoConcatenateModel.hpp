#ifndef INFOCONCATENATEMODEL_HPP
#define INFOCONCATENATEMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPlainTextEdit>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "InformationData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class InfoConcatenateModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    InfoConcatenateModel();

    virtual
    ~InfoConcatenateModel() override {}

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

//    bool
//    resizable() const override { return true; }

    static const QString _category;

    static const QString _model_name;

private:

    std::shared_ptr< InformationData > mpInformationData_1;
    std::shared_ptr< InformationData > mpInformationData_2;
    std::shared_ptr< InformationData > mpInformationData;
};

#endif // INFOCONCATENATEMODEL_HPP
