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

#ifndef TEMPLATETHREADMODEL_HPP
#define TEMPLATETHREADMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class TemplateThread : public QThread
{
    Q_OBJECT
public:
    explicit
    TemplateThread( QObject *parent = nullptr );

    ~TemplateThread() override;

    void
    start_thread( );

    void
    stop_thread();

Q_SIGNALS:
    void
    error_signal(int);

protected:
    void
    run() override;

private:
    int miThreadStatus {0};
    bool mbThreadReady {false};
    QSemaphore mWaitingSemaphore;
    bool mbAbort {false};
};

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class TemplateThreadModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    TemplateThreadModel();

    virtual
    ~TemplateThreadModel() override
    {
        if( mpTemplateThread )
            delete mpTemplateThread;
    }

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    void
    setModelProperty( QString &, const QVariant & ) override;

    void
    late_constructor() override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:
    void
    thread_error_occured(int);

private:
    TemplateThread * mpTemplateThread { nullptr };

    void processData(const std::shared_ptr< NodeData > & in );
};
#endif
