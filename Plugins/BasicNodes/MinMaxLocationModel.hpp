#ifndef MINMAXLOCATIONMODEL_HPP
#define MINMAXLOCATIONMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "CVPointData.hpp"
#include "DoubleData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

class MinMaxLocationModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    MinMaxLocationModel();

    virtual
    ~MinMaxLocationModel() override {}

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:

    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<CVPointData> mapCVPointData[2] {{nullptr}};
    std::shared_ptr<DoubleData> mapDoubleData[2] {{nullptr}};
    QPixmap _minPixmap;

    void processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVPointData> (&outPoint)[2], std::shared_ptr<DoubleData>(&outDouble)[2]);

};

#endif // MINMAXLOCATIONMODEL_HPP
