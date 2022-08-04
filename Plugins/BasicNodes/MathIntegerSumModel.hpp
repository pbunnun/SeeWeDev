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

#ifndef MATHINTEGERSUMMODEL_HPP
#define MATHINTEGERSUMMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPlainTextEdit>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "IntegerData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class MathIntegerSumModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    MathIntegerSumModel();

    virtual
    ~MathIntegerSumModel() override {}

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

//    bool
//    resizable() const override { return true; }

    static const QString _category;

    static const QString _model_name;

private:

    std::shared_ptr< IntegerData > mpIntegerData_1;
    std::shared_ptr< IntegerData > mpIntegerData_2;
    std::shared_ptr< IntegerData > mpIntegerData;
};

#endif // MATHINTEGERSUMMODEL_HPP
