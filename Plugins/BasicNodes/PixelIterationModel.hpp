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

#ifndef PIXELITERATIONMODEL_HPP
#define PIXELITERATIONMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"
#include "CVScalarData.h"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct PixIter
{
    enum IterKey {
        COUNT = 0,
        REPLACE = 1,
        LINEAR = 2,
        INVERSE = 3
    };

    int miIterKey;

    explicit PixIter(const int key)
        : miIterKey(key)
    {
    }

    void Iterate
    (cv::Mat& image, const cv::Scalar& inColors = 0, const cv::Scalar& outColors = 0, int* const number = 0, const double alpha = 0, const double beta = 0) const;

} PixIter;

typedef struct PixelIterationParameters{
    int miOperation;
    int mucColorInput[3];
    int mucColorOutput[3];
    double mdAlpha;
    double mdBeta;
    PixelIterationParameters()
        : miOperation(0),
          mucColorInput{0},
          mucColorOutput{0},
          mdAlpha(1),
          mdBeta(0)
    {
    }
} PixelIterationParameters;

class PixelIterationModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    PixelIterationModel();

    virtual
    ~PixelIterationModel() override {}

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
    PixelIterationParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<CVScalarData> mpCVScalarInData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<IntegerData> mpIntegerData {nullptr};
    QPixmap _minPixmap;

    static const std::string color[3];

    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & outImage,
                      std::shared_ptr<IntegerData> &outInt, const PixelIterationParameters & params );

    void overwrite( std::shared_ptr<CVScalarData> &in, PixelIterationParameters &params);
};

#endif // PIXELITERATIONMODEL_HPP
