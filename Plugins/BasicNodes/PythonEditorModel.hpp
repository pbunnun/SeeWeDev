//Copyright © 2025 - 2026, NECTEC, all rights reserved

#pragma once

#include <QtCore/QObject>
#include <QProcess>
#include <QTemporaryFile>
#include <QStringList>
#include <vector>
#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"
#include "PythonEditorEmbeddedWidget.hpp"

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
    virtual ~PythonEditorModel() override {};

    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    QWidget *embeddedWidget() override { return mpEmbeddedWidget; }

    void setModelProperty(QString &, const QVariant &) override;

    QPixmap minPixmap() const override { return QPixmap(":/basic_nodes/python.png"); }

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;

    static const QString _category;
    static const QString _model_name;

private Q_SLOTS:
    void onExecutePython();
    void onNumInputsChanged();
    void onNumOutputsChanged();

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
        PortTypeSync
    };

    QStringList availablePortTypeNames() const;
    NodeDataType nodeDataTypeFromIndex(int typeIndex) const;
    int normalizePortTypeIndex(int typeIndex) const;
    QString inputTypePropertyId(int index) const;
    QString outputTypePropertyId(int index) const;
    void rebuildPortTypeProperties();

    void executePythonCode();
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
};
