#ifndef CONVERTDEPTHMODEL_HPP
#define CONVERTDEPTHMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <opencv2/core.hpp>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct ConvertDepthParameters{
    int miImageDepth;
    double mdAlpha;
    double mdBeta;
    ConvertDepthParameters()
        : miImageDepth(CV_8U),
          mdAlpha(1),
          mdBeta(0)
    {
    }
} ConvertDepthParameters;

class ConvertDepthModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    ConvertDepthModel();

    virtual
    ~ConvertDepthModel() override {}

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
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:
    ConvertDepthParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<IntegerData> mpIntegerInData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    QPixmap _minPixmap;

    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr<CVImageData> & out,
                      const ConvertDepthParameters & params );

    void overwrite(std::shared_ptr<IntegerData> &in, ConvertDepthParameters &params );
};

#endif // CONVERTDEPTHMODEL_HPP
