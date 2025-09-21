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

#ifndef COMBINESYNCMODEL_HPP
#define COMBINESYNCMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QComboBox>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

enum CombineCondition
{
    CombineCondition_AND = 0,
    CombineCondition_OR
};

class CombineSyncModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CombineSyncModel();

    virtual
    ~CombineSyncModel() override {}

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
    embeddedWidget() override { return mpEmbeddedWidget; }

    void
    setModelProperty( QString &, const QVariant & ) override;

//    bool
//    resizable() const override { return true; }

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:
    void
    combine_operation_changed(const QString &);

private:

    QComboBox * mpEmbeddedWidget;
    CombineCondition mCombineCondition{ CombineCondition_AND };
    bool mbReady_1 {false};
    bool mbReady_2 {false};
    std::shared_ptr< SyncData > mpSyncData_1;
    std::shared_ptr< SyncData > mpSyncData_2;
    std::shared_ptr< SyncData > mpSyncData;
};

#endif // COMBINESYNCMODEL_HPP
