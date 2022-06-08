#ifndef ERODEANDDILATEMODEL_HPP
#define ERODEANDDILATEMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include <opencv2/imgproc.hpp>
#include "ErodeAndDilateEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct ErodeAndDilateParameters{
    int miKernelShape;
    cv::Size mCVSizeKernel;
    cv::Point mCVPointAnchor;
    int miIterations;
    int miBorderType;
    ErodeAndDilateParameters()
        : miKernelShape(cv::MORPH_RECT),
          mCVSizeKernel(cv::Size(3,3)),
          mCVPointAnchor(cv::Point(-1,-1)),
          miIterations(1),
          miBorderType(cv::BORDER_DEFAULT)
    {
    }
} ErodeAndDilateParameters;


class ErodeAndDilateModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    ErodeAndDilateModel();

    virtual
    ~ErodeAndDilateModel() override {}

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

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:
    void em_radioButton_clicked();

private:
    ErodeAndDilateParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    ErodeAndDilateEmbeddedWidget* mpEmbeddedWidget;
    QPixmap _minPixmap;

    void processData(const std::shared_ptr<CVImageData>& in, std::shared_ptr<CVImageData>& out, const ErodeAndDilateParameters& params);
};

#endif // ERODEANDDILATEMODEL_HPP
