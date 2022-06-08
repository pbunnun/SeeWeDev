#ifndef BLENDIMAGESMODEL_HPP
#define BLENDIMAGESMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "BlendImagesEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct BlendImagesParameters{
    double mdAlpha;
    double mdBeta;
    double mdGamma;
    bool mbSizeFromPort0;
    BlendImagesParameters()
        : mdAlpha(0.5),
          mdBeta(0.5),
          mdGamma(0),
          mbSizeFromPort0(false)
    {
    }
} BlendImagesParameters;


class BlendImagesModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    BlendImagesModel();

    virtual
    ~BlendImagesModel() override {}

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
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;

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
    BlendImagesParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mapCVImageInData[2] { {nullptr} };
    BlendImagesEmbeddedWidget* mpEmbeddedWidget;
    QPixmap _minPixmap;

    void processData( const std::shared_ptr< CVImageData> (&in)[2], std::shared_ptr< CVImageData > & out,
                      const BlendImagesParameters & params);
    bool allports_are_active(const std::shared_ptr<CVImageData> (&ap)[2] ) const;
};

#endif // BLENDIMAGESMODEL_HPP
