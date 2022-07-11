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

#ifndef TEMPLATEMATCHINGMODEL_HPP
#define TEMPLATEMATCHINGMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <opencv2/imgproc.hpp>

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

typedef struct TemplateMatchingParameters{
    int miMatchingMethod;
    int mucLineColor[3];
    int miLineThickness;
    int miLineType;
    TemplateMatchingParameters()
        : miMatchingMethod(cv::TM_SQDIFF),
          mucLineColor{0},
          miLineThickness(3),
          miLineType(cv::LINE_8)
    {
    }
} TemplateMatchingParameters;

class TemplateMatchingModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    TemplateMatchingModel();

    virtual
    ~TemplateMatchingModel() override {}

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
    TemplateMatchingParameters mParams;
    std::shared_ptr<CVImageData> mapCVImageInData[2] {{ nullptr }};
    std::shared_ptr<CVImageData> mapCVImageData[2] {{ nullptr }};
    QPixmap _minPixmap;

    static const std::string color[3];

    void processData( const std::shared_ptr< CVImageData> (&in)[2], std::shared_ptr< CVImageData > (&out)[2],
                      const TemplateMatchingParameters & params );
};

#endif // TEMPLATEMATCHINGMODEL_HPP
