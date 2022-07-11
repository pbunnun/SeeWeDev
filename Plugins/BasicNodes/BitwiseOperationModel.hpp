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

#ifndef BITWISEOPERATIONMODEL_HPP
#define BITWISEOPERATIONMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "BitwiseOperationEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

typedef struct BitwiseOperationParameters{
    int miBitwiseType;
    BitwiseOperationParameters()
        : miBitwiseType(0)
    {
    }
} BitwiseOperationParameters;

typedef struct BitwiseOperationProperties
{
    bool mbActiveMask;
    BitwiseOperationProperties()
        : mbActiveMask(false)
    {
    }
} BitwiseOperationProperties;

class BitwiseOperationModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    BitwiseOperationModel();

    virtual
    ~BitwiseOperationModel() override {}

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

private:
    BitwiseOperationParameters mParams;
    BitwiseOperationProperties mProps;
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mapCVImageInData[3] { {nullptr} };
    BitwiseOperationEmbeddedWidget* mpEmbeddedWidget {nullptr};
    QPixmap _minPixmap;

    void processData(const std::shared_ptr<CVImageData> (&in)[3], std::shared_ptr<CVImageData>& out, const BitwiseOperationParameters& params, BitwiseOperationProperties &props);

};

#endif // BITWISEOPERATIONMODEL_HPP
