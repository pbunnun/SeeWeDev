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

#ifndef TIMERMODEL_HPP
#define TIMERMODEL_HPP

#pragma once

#include <QTimer>
#include "PBNodeDataModel.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class TimerModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    TimerModel();

    virtual
    ~TimerModel() override
    {
        if( mpTimer )
        {
            mpTimer->stop();
            delete mpTimer;
        }
    }

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
    setInData(std::shared_ptr<NodeData>, int) override { }

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:
    void
    enable_changed( bool ) override;

    void timeout_function();

private:
    QTimer * mpTimer;
    std::shared_ptr<SyncData> mpSyncData { nullptr };

    int miMillisecondInterval { 1000 };
};

#endif // TIMERMODEL_HPP
