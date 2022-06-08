#ifndef DRAWCONTOURMODEL_HPP
#define DRAWCONTOURMODEL_HPP

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

typedef struct DrawContourParameters{
    int miContourMode;
    int miContourMethod;
    int mucBValue;
    int mucGValue;
    int mucRValue;
    int miLineThickness;
    int miLineType;
    DrawContourParameters()
        : miContourMode(1),
          miContourMethod(1),
          mucBValue(0),
          mucGValue(255),
          mucRValue(0),
          miLineThickness(2),
          miLineType(0)
    {
    }
} DrawContourParameters;

class DrawContourModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    DrawContourModel();

    virtual
    ~DrawContourModel() override {}

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    QWidget *
    embeddedWidget() override {return nullptr;}

    /*
     * Recieve signals back from QtPropertyBrowser and use this function to
     * set parameters/variables accordingly.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;


private:
    DrawContourParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<IntegerData> mpIntegerData {nullptr};
    QPixmap _minPixmap;

    void processData(const std::shared_ptr<CVImageData>& in, std::shared_ptr<CVImageData>& outImage,
                     std::shared_ptr<IntegerData> &outInt, const DrawContourParameters& params);
};

#endif // DRAWCONTOURMODEL_HPP
