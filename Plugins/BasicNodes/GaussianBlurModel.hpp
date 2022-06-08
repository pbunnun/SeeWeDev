#ifndef GAUSSIANBLURMODEL_HPP
#define GAUSSIANBLURMODEL_HPP

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

typedef struct GaussianBlurParameters{
    cv::Size mCVSizeKernel;
    double mdSigmaX;
    double mdSigmaY;
    int miBorderType;
    GaussianBlurParameters()
    {
        mCVSizeKernel = cv::Size(5,5);
        mdSigmaX = 0;
        mdSigmaY = 0;
        miBorderType = cv::BORDER_DEFAULT;
    }
} GaussianBlurParameters;

class GaussianBlurModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    GaussianBlurModel();

    virtual
    ~GaussianBlurModel() override {}

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
    GaussianBlurParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    QPixmap _minPixmap;

    void processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<CVImageData>& out, const GaussianBlurParameters& params);

};
#endif
