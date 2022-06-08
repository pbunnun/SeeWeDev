#ifndef IMAGEROIMODEL_HPP
#define IMAGEROIMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "ImageROIEmbeddedWidget.hpp"
#include <opencv2/highgui.hpp>
#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;


typedef struct ImageROIParameters
{
    cv::Point mCVPointRect1;
    cv::Point mCVPointRect2;
    int mucLineColor[3];
    int miLineThickness;
    bool mbDisplayLines;
    bool mbLockOutputROI;
    ImageROIParameters()
        : mCVPointRect1(cv::Point(0,0)),
          mCVPointRect2(cv::Point(0,0)),
          mucLineColor{0},
          miLineThickness(2),
          mbDisplayLines(true),
          mbLockOutputROI(false)
    {
    }
} ImageROIParameters;

typedef struct ImageROIProperties
{
    bool mbReset;
    bool mbApply;
    bool mbNewMat;
    ImageROIProperties()
        : mbReset(false),
          mbApply(false),
          mbNewMat(true)
    {
    }
} ImageROIProperties;

class ImageROIModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    ImageROIModel();

    virtual
    ~ImageROIModel() override {}

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
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS :

    void em_button_clicked( int button );


private:

    void processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr<CVImageData> (&out)[2],
                     const ImageROIParameters & params, ImageROIProperties &props );

    void overwrite(const std::shared_ptr<CVImageData>& in, ImageROIParameters& params);

    static const std::string color[3];

    ImageROIParameters mParams;

    ImageROIProperties mProps;

    ImageROIEmbeddedWidget* mpEmbeddedWidget;

    std::shared_ptr<CVImageData> mapCVImageInData[2] {{nullptr}};

    std::shared_ptr<CVImageData> mapCVImageData[2] {{nullptr}};

    QPixmap _minPixmap;
};

#endif // IMAGEROIMODEL_HPP
