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

#ifndef SYNCGATEMODEL_HPP
#define SYNCGATEMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "SyncData.hpp"
#include "BoolData.h"

#include "SyncGateEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;


typedef struct LogicGate
{
    enum LogicType
    {
        EQUAL = 0,
        AND = 1,
        OR = 2,
        XOR = 3,
        NOR = 4,
        NAND = 5,
        DIRECT = 6,
        DIRECT_NOT = 7
    };
} LogicGate;


typedef struct SyncGateParameters
{
    int miOperation;
    SyncGateParameters()
        : miOperation(LogicGate::AND)
    {
    }
} SyncGateParameters;


class SyncGateModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    SyncGateModel();

    virtual
    ~SyncGateModel() override {}

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

private Q_SLOTS :

    void em_checkbox_checked();

private:
    SyncGateParameters mParams;
    SyncGateEmbeddedWidget* mpEmbeddedWidget;
    std::shared_ptr<SyncData> mapSyncInData[2] { {nullptr} };
    std::shared_ptr<BoolData> mapBoolInData[2] { {nullptr} };
    std::shared_ptr<SyncData> mapSyncData[2] { {nullptr} };
    std::shared_ptr<BoolData> mapBoolData[2] { {nullptr} };
    QPixmap _minPixmap;

    void processData(const std::shared_ptr<SyncData> (&inSync)[2], const std::shared_ptr<BoolData> (&inBool)[2],
                     std::shared_ptr<SyncData> (&outSync)[2], std::shared_ptr<BoolData> (&outBool)[2],
                     const SyncGateParameters & params);
};

#endif // SYNCGATEMODEL_HPP
