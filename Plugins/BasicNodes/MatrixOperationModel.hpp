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

#ifndef MATRIXOPERATIONMODEL_HPP
#define MATRIXOPERATIONMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

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
///

struct MatOps
{
    enum Operators
    {
        PLUS = 0,
        MINUS = 1,
        GREATER_THAN = 2,
        GREATER_THAN_OR_EQUAL = 3,
        LESSER_THAN = 4,
        LESSER_THAN_OR_EQUAL = 5,
        MULTIPLY = 6,
        DIVIDE = 7,
        MAXIMUM = 8,
        MINIMUM = 9
    };
};

typedef struct MatrixOperationParameters{
    int miOperator;
    MatrixOperationParameters()
        : miOperator(MatOps::PLUS)
    {
    }
} MatrixOperationParameters;

class MatrixOperationModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    MatrixOperationModel();

    virtual
    ~MatrixOperationModel() override {}

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
    MatrixOperationParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mapCVImageInData[2] {{ nullptr }};
    QPixmap _minPixmap;

    void processData( const std::shared_ptr<CVImageData> (&in)[2], std::shared_ptr< CVImageData > & out,
                      const MatrixOperationParameters & params );
};

#endif // MATRIXOPERATIONMODEL_HPP
