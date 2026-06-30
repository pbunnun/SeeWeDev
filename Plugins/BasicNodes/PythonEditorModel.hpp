//Copyright © 2025 - 2026, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#pragma once

#include <QtCore/QObject>
#include <QThread>
#include <QStringList>
#include <vector>
#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"
#include "PythonEditorEmbeddedWidget.hpp"
#include "PythonSessionWorker.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @brief Python Editor Node - Execute Python code with input/output data exchange
 * 
 * Features:
 * - Multi-line Python code editor
 * - Configurable number of input/output ports
 * - Automatic data type conversion (images, numbers, strings, points, etc.)
 * - Real-time execution with error reporting
 * - Variable persistence across executions
 * - Access to input data via input0, input1, ... variables
 * - Return output via output0, output1, ... variables
 */
class PythonEditorModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    PythonEditorModel();
    virtual ~PythonEditorModel() override;

    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    QWidget *embeddedWidget() override { return mpEmbeddedWidget; }

    void setModelProperty(QString &, const QVariant &) override;

    QPixmap minPixmap() const override { return QPixmap(":/basic_nodes/python.png"); }

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;
    void late_constructor() override;

    static const QString _category;
    static const QString _model_name;

private Q_SLOTS:
    void onExecutePython();
    void onNumInputsChanged();
    void onNumOutputsChanged();
    void onSessionStarted();
    void onSessionFailed( const QString& errorMessage );
    void onResultReady( const QString& outputsJson );
    void onExecutionError( const QString& errorMessage );

private:
    enum PortDataTypeIndex
    {
        PortTypeImage = 0,
        PortTypeInteger,
        PortTypeDouble,
        PortTypeString,
        PortTypePoint,
        PortTypeRect,
        PortTypeSize,
        PortTypeSync,
        PortTypeInfo
    };

    QStringList availablePortTypeNames() const;
    NodeDataType nodeDataTypeFromIndex(int typeIndex) const;
    int normalizePortTypeIndex(int typeIndex) const;
    QString inputTypePropertyId(int index) const;
    QString outputTypePropertyId(int index) const;
    void rebuildPortTypeProperties();

    void executePythonCode();
    QString buildInputsJson();
    QString serializeInputData(int index);
    bool deserializeOutputData(int index, const QString& jsonStr, QString& errorMessage);
    
    PythonEditorEmbeddedWidget* mpEmbeddedWidget {nullptr};
    
    // Python code
    QString msPythonCode;
    
    // Number of ports
    int miNumInputs{1};
    int miNumOutputs{1};

    // Per-port selectable data type index
    std::vector<int> mvInputTypeIndices;
    std::vector<int> mvOutputTypeIndices;
    
    // Input data storage
    std::vector<std::shared_ptr<NodeData>> mvInputData;
    
    // Output data storage
    std::vector<std::shared_ptr<NodeData>> mvOutputData;
    
    // Execution status
    std::shared_ptr<InformationData> mpExecutionInfo;
    bool mbExecutionSuccess{true};

    QThread* mpWorkerThread { nullptr };
    PythonSessionWorker* mpSessionWorker { nullptr };
    bool mbSessionRunning { false };
    bool mbBusy { false };
    bool mbPendingExecution { false };
    QString msLastExecutedCode;
    QString msPythonExecutable { "python3" };
    QString msLastSessionExecutable;
};
