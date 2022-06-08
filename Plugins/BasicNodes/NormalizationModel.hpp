#ifndef NORMALIZATIONMODEL_HPP
#define NORMALIZATIONMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "DoubleData.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct NormalizationParameters{
    double mdRangeMax;
    double mdRangeMin;
    int miNormType;
    NormalizationParameters()
        : mdRangeMax(255),
          mdRangeMin(0),
          miNormType(cv::NORM_MINMAX)
    {
    }
} NormalizationParameters;

class NormalizationModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    NormalizationModel();

    virtual
    ~NormalizationModel() override {}

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
    NormalizationParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<DoubleData> mapDoubleInData[2] {{nullptr}};
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    QPixmap _minPixmap;

    void processData( const std::shared_ptr< CVImageData> &in, std::shared_ptr<CVImageData> &out,
                      const NormalizationParameters & params);

    void overwrite(std::shared_ptr<DoubleData> (&in)[2], NormalizationParameters &params);

};

#endif // NORMALIZATIONMODEL_HPP
