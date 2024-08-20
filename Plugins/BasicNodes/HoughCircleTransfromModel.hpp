//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#ifndef HOUGHCIRCLETRANSFROMMODEL_HPP
#define HOUGHCIRCLETRANSFROMMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct HoughCircleTransformParameters{
    int miHoughMethod;
    double mdInverseRatio;
    double mdCenterDistance;
    double mdThresholdU;
    double mdThresholdL;
    int miRadiusMin;
    int miRadiusMax;
    bool mbDisplayPoint;
    int mucPointColor[3];
    int miPointSize;
    bool mbDisplayCircle;
    int mucCircleColor[3];
    int miCircleThickness;
    int miCircleType;

    bool mbEnableGradient;
    HoughCircleTransformParameters()
        : miHoughMethod(cv::HOUGH_GRADIENT),
          mdInverseRatio(1),
          mdCenterDistance(10),
          mdThresholdU(200),
          mdThresholdL(100),
          miRadiusMin(5),
          miRadiusMax(20),
          mbDisplayPoint(true),
          mucPointColor{0},
          miPointSize(3),
          mbDisplayCircle(true),
          mucCircleColor{0},
          miCircleThickness(3),
          miCircleType(cv::LINE_AA)
    {
    }
} HoughCircleTransformParameters;

class HoughCircleTransformModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    HoughCircleTransformModel();

    virtual
    ~HoughCircleTransformModel() override {}

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
    HoughCircleTransformParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<IntegerData> mpIntegerData { nullptr };
    QPixmap _minPixmap;

    static const std::string color[3];

    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & outImage,
                      std::shared_ptr<IntegerData> &outInt, const HoughCircleTransformParameters & params);
};

#endif // HOUGHCIRCLETRANSFROMMODEL_HPP
