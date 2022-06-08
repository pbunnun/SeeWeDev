#ifndef FILTER2DMODEL_HPP
#define FILTER2DMODEL_HPP

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
///


typedef struct MatKernel
{
    enum KernelType {
        KERNEL_NULL = 0,
        KERNEL_LAPLACIAN = 1,
        KERNEL_AVERAGE = 2
    };

    int miKernelType;
    int miKernelSize;

    explicit MatKernel(const KernelType kernelType, const int size)
        : miKernelType(kernelType),
          miKernelSize(size)
    {
    }

    const cv::Mat image() const;

} MatKernel;

typedef struct Filter2DParameters{
    int miImageDepth;
    MatKernel mMKKernel;
    double mdDelta;
    int miBorderType;
    Filter2DParameters()
        : miImageDepth(CV_8U),
          mMKKernel(MatKernel(MatKernel::KERNEL_NULL, 3)),
          mdDelta(0),
          miBorderType(cv::BORDER_DEFAULT)
    {
    }
} Filter2DParameters;

class Filter2DModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    Filter2DModel();

    virtual
    ~Filter2DModel() override {}

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
    Filter2DParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    QPixmap _minPixmap;

    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & out,
                      const Filter2DParameters & params );
};

#endif //FILTER2DMODEL_H
