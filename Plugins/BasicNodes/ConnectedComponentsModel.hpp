#ifndef CONNECTEDCOMPONENTSMODEL_HPP
#define CONNECTEDCOMPONENTSMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <opencv2/imgproc.hpp>
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

typedef struct ConnectedComponentsParameters{
    int miConnectivity;
    int miImageType;
    int miAlgorithmType;
    bool mbVisualize;
    ConnectedComponentsParameters()
        : miConnectivity(4),
          miImageType(CV_32S),
          miAlgorithmType(cv::CCL_DEFAULT),
          mbVisualize(false)
    {
    }
} ConnectedComponentsParameters;

class ConnectedComponentsModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    ConnectedComponentsModel();

    virtual
    ~ConnectedComponentsModel() override {}

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
    ConnectedComponentsParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<IntegerData> mpIntegerData { nullptr };
    QPixmap _minPixmap;

    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & outImage,
                      std::shared_ptr<IntegerData> &outInt, const ConnectedComponentsParameters & params );
};

#endif // CONNECTEDCOMPONENTSMODEL_HPP
