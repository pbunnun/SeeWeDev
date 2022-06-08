#ifndef FLOODFILLMODEL_HPP
#define FLOODFILLMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "FloodFillEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct FloodFillParameters{
    cv::Point mCVPointSeed;
    int mucFillColor[4]; //{B,G,R,Grayscale}
    int mucLowerDiff[4];
    int mucUpperDiff[4];
    bool mbDefineBoundaries;
    cv::Point mCVPointRect1;
    cv::Point mCVPointRect2;
    int miFlags;
    int miMaskColor;
    FloodFillParameters()
        : mCVPointSeed(cv::Point(0,0)),
          mucFillColor{0},
          mucLowerDiff{0},
          mucUpperDiff{0},
          mbDefineBoundaries(false),
          mCVPointRect1(cv::Point(0,0)),
          mCVPointRect2(cv::Point(0,0)),
          miFlags(4),
          miMaskColor(255)
    {
    }
} FloodFillParameters;

typedef struct FloodFillProperties
{
    bool mbActiveMask;
    FloodFillProperties()
        : mbActiveMask(false)
    {
    }
} FloodFillProperties;


class FloodFillModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    FloodFillModel();

    virtual
    ~FloodFillModel() override {}

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

    void em_spinbox_clicked( int spinbox, int value );

private:
    FloodFillParameters mParams;
    FloodFillProperties mProps;
    std::shared_ptr<CVImageData> mapCVImageData[2] { {nullptr} };
    std::shared_ptr<CVImageData> mapCVImageInData[2] { {nullptr} };
    FloodFillEmbeddedWidget* mpEmbeddedWidget;
    QPixmap _minPixmap;

    static const std::string color[4];

    void processData( const std::shared_ptr<CVImageData> (&in)[2], std::shared_ptr< CVImageData > (&out)[2],
                      const FloodFillParameters & params, FloodFillProperties &props);

};

#endif // FLOODFILLMODEL_HPP
