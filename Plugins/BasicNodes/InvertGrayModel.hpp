#ifndef INVERTGRAYMODEL_HPP
#define INVERTGRAYMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class InvertGrayModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    InvertGrayModel();

    virtual
    ~InvertGrayModel() override {}

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr< NodeData >
    outData( PortIndex port ) override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:
    std::shared_ptr< CVImageData > mpCVImageData;

    QPixmap _minPixmap;
    void processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr< CVImageData > & out );
};

#endif // INVERTGRAYMODEL_HPP
