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

#ifndef SOBELANDSCHARRMODEL_HPP
#define SOBELANDSCHARRMODEL_HPP
//Bug occurs when connecting empty Sobel output to the Gaussian Blur Node

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include <opencv2/imgproc.hpp>
#include "SobelAndScharrEmbeddedWidget.hpp"


using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct SobelAndScharrParameters{
    int miOrderX;
    int miOrderY;
    int miKernelSize;
    double mdScale;
    double mdDelta;
    int miBorderType;
    SobelAndScharrParameters()
        : miOrderX(1),
          miOrderY(1),
          miKernelSize(3),
          mdScale(1),
          mdDelta(0),
          miBorderType(cv::BORDER_DEFAULT)
    {
    }
} SobelAndScharrParameters;

class SobelAndScharrModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    SobelAndScharrModel();

    virtual
    ~SobelAndScharrModel() override {}

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

    void em_checkbox_checked(int);

private:
    SobelAndScharrParameters mParams;
    std::shared_ptr<CVImageData> mapCVImageData[3] {{ nullptr }};
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    SobelAndScharrEmbeddedWidget* mpEmbeddedWidget;
    QPixmap _minPixmap;

    void processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<CVImageData> (&out)[3],
                     const SobelAndScharrParameters &params);
};


#endif // SOBELANDSCHARRMODEL_HPP
