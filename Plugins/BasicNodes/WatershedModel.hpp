#ifndef WATERSHEDMODEL_HPP
#define WATERSHEDMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class WatershedModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    WatershedModel();

    virtual
    ~WatershedModel() override {}

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

    std::shared_ptr< CVImageData > mapCVImageInData[2];
    std::shared_ptr< CVImageData > mpCVImageData;
    std::shared_ptr<SyncData> mpSyncData;

    QPixmap _minPixmap;
    void processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr< CVImageData > & out );
};

#endif // WATERSHEDMODEL_HPP
