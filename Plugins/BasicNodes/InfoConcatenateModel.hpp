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

#ifndef INFOCONCATENATEMODEL_HPP
#define INFOCONCATENATEMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPlainTextEdit>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "InformationData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class InfoConcatenateModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    InfoConcatenateModel();

    virtual
    ~InfoConcatenateModel() override {}

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p) override;

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

private Q_SLOTS:
    void
    inputConnectionCreated(QtNodes::Connection const&) override;

    void
    inputConnectionDeleted(QtNodes::Connection const&) override;

private:

    std::shared_ptr< InformationData > mpInformationData_1;
    std::shared_ptr< InformationData > mpInformationData_2;
    std::shared_ptr< InformationData > mpInformationData;

    bool mbUseSyncSignal{false};
};

#endif // INFOCONCATENATEMODEL_HPP
