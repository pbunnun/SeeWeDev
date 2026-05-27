//Copyright © 2025 - 2026, NECTEC, all rights reserved

#include "PythonEditorModel.hpp"
#include "PythonEditorEmbeddedWidget.hpp"
#include "CVImageData.hpp"
#include "DoubleData.hpp"
#include "IntegerData.hpp"
#include "StdStringData.hpp"
#include "CVPointData.hpp"
#include "CVRectData.hpp"
#include "CVSizeData.hpp"
#include "SyncData.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QDir>
#include <algorithm>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include "qtvariantproperty_p.h"

const QString PythonEditorModel::_category = QString("Python");
const QString PythonEditorModel::_model_name = QString("Python Editor");

PythonEditorModel::PythonEditorModel()
    : PBNodeDelegateModel(_model_name)
{
    mpEmbeddedWidget = new PythonEditorEmbeddedWidget();
    
    // Initialize input/output data vectors
    mvInputData.resize(miNumInputs);
    mvOutputData.resize(miNumOutputs);
    mvInputTypeIndices.resize(miNumInputs, PortTypeImage);
    mvOutputTypeIndices.resize(miNumOutputs, PortTypeImage);
    
    mpExecutionInfo = std::make_shared<InformationData>();
    mpExecutionInfo->set_information("Ready to execute Python code");

    // Create properties
    IntPropertyType intType;
    intType.miValue = miNumInputs;
    intType.miMin = 0;
    intType.miMax = 10;
    QString propId = "num_inputs";
    auto propNumInputs = std::make_shared<TypedProperty<IntPropertyType>>("Number of Inputs", propId, QMetaType::Int, intType, "Configuration");
    mvProperty.push_back(propNumInputs);
    mMapIdToProperty[propId] = propNumInputs;

    intType.miValue = miNumOutputs;
    propId = "num_outputs";
    auto propNumOutputs = std::make_shared<TypedProperty<IntPropertyType>>("Number of Outputs", propId, QMetaType::Int, intType, "Configuration");
    mvProperty.push_back(propNumOutputs);
    mMapIdToProperty[propId] = propNumOutputs;

    rebuildPortTypeProperties();

    propId = "python_code";
    auto propCode = std::make_shared<TypedProperty<QString>>("Python Code", propId, QMetaType::QString, msPythonCode, "Code");
    mvProperty.push_back(propCode);
    mMapIdToProperty[propId] = propCode;

    // Connect widget signals
        QObject::connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::executeClicked,
                 this, &PythonEditorModel::onExecutePython);
        QObject::connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::numInputsChanged,
                 this, &PythonEditorModel::onNumInputsChanged);
        QObject::connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::numOutputsChanged,
                 this, &PythonEditorModel::onNumOutputsChanged);
        QObject::connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::widgetClicked,
                 this, &PythonEditorModel::selection_request_signal);

        QObject::connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::editableWidgetSelectedChanged,
                 this, &PBNodeDelegateModel::editable_embedded_widget_selected_changed);
}

unsigned int PythonEditorModel::nPorts(PortType portType) const
{
    switch (portType)
    {
    case PortType::In:
        return miNumInputs;
    case PortType::Out:
        return miNumOutputs + 1; // +1 for execution info
    default:
        return 0;
    }
}

QStringList PythonEditorModel::availablePortTypeNames() const
{
    return QStringList({"Image", "Integer", "Double", "String", "Point", "Rect", "Size", "Sync"});
}

NodeDataType PythonEditorModel::nodeDataTypeFromIndex(int typeIndex) const
{
    switch (normalizePortTypeIndex(typeIndex))
    {
    case PortTypeInteger:
        return IntegerData().type();
    case PortTypeDouble:
        return DoubleData().type();
    case PortTypeString:
        return StdStringData().type();
    case PortTypePoint:
        return CVPointData().type();
    case PortTypeRect:
        return CVRectData().type();
    case PortTypeSize:
        return CVSizeData().type();
    case PortTypeSync:
        return SyncData().type();
    case PortTypeImage:
    default:
        return CVImageData().type();
    }
}

int PythonEditorModel::normalizePortTypeIndex(int typeIndex) const
{
    constexpr int minIndex = static_cast<int>(PortTypeImage);
    constexpr int maxIndex = static_cast<int>(PortTypeSync);
    if (typeIndex < minIndex || typeIndex > maxIndex)
        return PortTypeImage;
    return typeIndex;
}

QString PythonEditorModel::inputTypePropertyId(int index) const
{
    return QStringLiteral("input_type_") + QString::number(index);
}

QString PythonEditorModel::outputTypePropertyId(int index) const
{
    return QStringLiteral("output_type_") + QString::number(index);
}

void PythonEditorModel::rebuildPortTypeProperties()
{
    for (int i = static_cast<int>(mvProperty.size()) - 1; i >= 0; --i)
    {
        const QString propId = mvProperty[i]->getID();
        if (propId.startsWith("input_type_") || propId.startsWith("output_type_"))
            mvProperty.erase(mvProperty.begin() + i);
    }

    for (auto it = mMapIdToProperty.begin(); it != mMapIdToProperty.end(); )
    {
        if (it.key().startsWith("input_type_") || it.key().startsWith("output_type_"))
            it = mMapIdToProperty.erase(it);
        else
            ++it;
    }

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = availablePortTypeNames();

    for (int i = 0; i < miNumInputs; ++i)
    {
        enumPropertyType.miCurrentIndex = normalizePortTypeIndex(mvInputTypeIndices[i]);
        const QString propId = inputTypePropertyId(i);
        auto prop = std::make_shared<TypedProperty<EnumPropertyType>>(QStringLiteral("Input ") + QString::number(i) + QStringLiteral(" Type"),
                                                                       propId,
                                                                       QtVariantPropertyManager::enumTypeId(),
                                                                       enumPropertyType,
                                           QStringLiteral("Port Types"));
        mvProperty.push_back(prop);
        mMapIdToProperty[propId] = prop;
    }

    for (int i = 0; i < miNumOutputs; ++i)
    {
        enumPropertyType.miCurrentIndex = normalizePortTypeIndex(mvOutputTypeIndices[i]);
        const QString propId = outputTypePropertyId(i);
        auto prop = std::make_shared<TypedProperty<EnumPropertyType>>(QStringLiteral("Output ") + QString::number(i) + QStringLiteral(" Type"),
                                                                       propId,
                                                                       QtVariantPropertyManager::enumTypeId(),
                                                                       enumPropertyType,
                                           QStringLiteral("Port Types"));
        mvProperty.push_back(prop);
        mMapIdToProperty[propId] = prop;
    }

    Q_EMIT property_structure_changed_signal();
}

NodeDataType PythonEditorModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out)
    {
        if ( portIndex == static_cast<PortIndex>(miNumOutputs) )
            return InformationData().type();

        if (portIndex < static_cast<PortIndex>(mvOutputTypeIndices.size()))
            return nodeDataTypeFromIndex(mvOutputTypeIndices[portIndex]);

        return nodeDataTypeFromIndex(PortTypeImage);
    }
    else
    {
        if (portIndex < static_cast<PortIndex>(mvInputTypeIndices.size()))
            return nodeDataTypeFromIndex(mvInputTypeIndices[portIndex]);

        return nodeDataTypeFromIndex(PortTypeImage);
    }
}

std::shared_ptr<NodeData> PythonEditorModel::outData(PortIndex port)
{
    if (port == static_cast<PortIndex>(miNumOutputs))
        return mpExecutionInfo;
    
    if (port < static_cast<PortIndex>(mvOutputData.size()))
        return mvOutputData[port];
    
    return nullptr;
}

void PythonEditorModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex port)
{
    if (port >= static_cast<unsigned int>(mvInputData.size()))
        mvInputData.resize(port + 1);
    
    mvInputData[port] = nodeData;
    
    if( isEnable() )
        onExecutePython();
}

void PythonEditorModel::setModelProperty(QString& id, const QVariant& value)
{
    PBNodeDelegateModel::setModelProperty(id, value);

    if (id == "num_inputs")
    {
        miNumInputs = value.toInt();
        mpEmbeddedWidget->setNumInputs(miNumInputs);
        mvInputData.resize(miNumInputs);
        mvInputTypeIndices.resize(miNumInputs, PortTypeImage);
        rebuildPortTypeProperties();
        Q_EMIT embeddedWidgetSizeUpdated();
    }
    else if (id == "num_outputs")
    {
        miNumOutputs = value.toInt();
        mpEmbeddedWidget->setNumOutputs(miNumOutputs);
        mvOutputData.resize(miNumOutputs);
        mvOutputTypeIndices.resize(miNumOutputs, PortTypeImage);
        rebuildPortTypeProperties();
        Q_EMIT embeddedWidgetSizeUpdated();
    }
    else if (id.startsWith("input_type_"))
    {
        const int index = id.mid(QString("input_type_").size()).toInt();
        if (index >= 0 && index < static_cast<int>(mvInputTypeIndices.size()))
        {
            mvInputTypeIndices[index] = normalizePortTypeIndex(value.toInt());
            auto prop = mMapIdToProperty[id];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = mvInputTypeIndices[index];
            Q_EMIT property_changed_signal(prop);
            Q_EMIT embeddedWidgetSizeUpdated();
        }
    }
    else if (id.startsWith("output_type_"))
    {
        const int index = id.mid(QString("output_type_").size()).toInt();
        if (index >= 0 && index < static_cast<int>(mvOutputTypeIndices.size()))
        {
            mvOutputTypeIndices[index] = normalizePortTypeIndex(value.toInt());
            auto prop = mMapIdToProperty[id];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = mvOutputTypeIndices[index];
            Q_EMIT property_changed_signal(prop);
            Q_EMIT embeddedWidgetSizeUpdated();

            // Propagate only the changed output port so downstream links refresh immediately.
            Q_EMIT dataUpdated(static_cast<PortIndex>(index));
        }
    }
    else if (id == "python_code")
    {
        msPythonCode = value.toString();
        mpEmbeddedWidget->setPythonCode(msPythonCode);
    }
}

QJsonObject PythonEditorModel::save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    
    QJsonObject cParams;
    cParams["num_inputs"] = miNumInputs;
    cParams["num_outputs"] = miNumOutputs;
    cParams["python_code"] = mpEmbeddedWidget->getPythonCode();

    QJsonArray inputTypes;
    for (int typeIndex : mvInputTypeIndices)
        inputTypes.append(typeIndex);
    cParams["input_types"] = inputTypes;

    QJsonArray outputTypes;
    for (int typeIndex : mvOutputTypeIndices)
        outputTypes.append(typeIndex);
    cParams["output_types"] = outputTypes;
    
    modelJson["cParams"] = cParams;
    return modelJson;
}

void PythonEditorModel::load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);
    
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty())
    {
        QJsonValue v;
        
        v = paramsObj["num_inputs"];
        if (!v.isNull())
        {
            miNumInputs = v.toInt();
            mpEmbeddedWidget->setNumInputs(miNumInputs);
            mvInputData.resize(miNumInputs);
            mvInputTypeIndices.resize(miNumInputs, PortTypeImage);
            
            auto prop = mMapIdToProperty["num_inputs"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = miNumInputs;
        }
        
        v = paramsObj["num_outputs"];
        if (!v.isNull())
        {
            miNumOutputs = v.toInt();
            mpEmbeddedWidget->setNumOutputs(miNumOutputs);
            mvOutputData.resize(miNumOutputs);
            mvOutputTypeIndices.resize(miNumOutputs, PortTypeImage);
            
            auto prop = mMapIdToProperty["num_outputs"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = miNumOutputs;
        }

        rebuildPortTypeProperties();

        v = paramsObj["input_types"];
        if (v.isArray())
        {
            QJsonArray inputTypes = v.toArray();
            const int count = std::min(miNumInputs, static_cast<int>(inputTypes.size()));
            for (int i = 0; i < count; ++i)
            {
                mvInputTypeIndices[i] = normalizePortTypeIndex(inputTypes[i].toInt(PortTypeImage));
                const QString propId = inputTypePropertyId(i);
                if (mMapIdToProperty.contains(propId))
                {
                    auto prop = mMapIdToProperty[propId];
                    auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
                    typedProp->getData().miCurrentIndex = mvInputTypeIndices[i];
                }
            }
        }

        v = paramsObj["output_types"];
        if (v.isArray())
        {
            QJsonArray outputTypes = v.toArray();
            const int count = std::min(miNumOutputs, static_cast<int>(outputTypes.size()));
            for (int i = 0; i < count; ++i)
            {
                mvOutputTypeIndices[i] = normalizePortTypeIndex(outputTypes[i].toInt(PortTypeImage));
                const QString propId = outputTypePropertyId(i);
                if (mMapIdToProperty.contains(propId))
                {
                    auto prop = mMapIdToProperty[propId];
                    auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
                    typedProp->getData().miCurrentIndex = mvOutputTypeIndices[i];
                }
            }
        }
        
        v = paramsObj["python_code"];
        if (!v.isNull())
        {
            msPythonCode = v.toString();
            mpEmbeddedWidget->setPythonCode(msPythonCode);
            
            auto prop = mMapIdToProperty["python_code"];
            auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
            typedProp->getData() = msPythonCode;
        }
    }
}

void PythonEditorModel::onNumInputsChanged()
{
    int newNum = mpEmbeddedWidget->getNumInputs();
    if (newNum != miNumInputs)
    {
        auto prop = mMapIdToProperty["num_inputs"];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);

        miNumInputs = newNum;
        mvInputData.resize(miNumInputs);
        mvInputTypeIndices.resize(miNumInputs, PortTypeImage);

        // Update property value first so the property browser rebuild sees the latest value.
        typedProp->getData().miValue = miNumInputs;

        rebuildPortTypeProperties();

        Q_EMIT property_changed_signal(prop);
        
        Q_EMIT embeddedWidgetSizeUpdated();
    }
}

void PythonEditorModel::onNumOutputsChanged()
{
    int newNum = mpEmbeddedWidget->getNumOutputs();
    if (newNum != miNumOutputs)
    {
        auto prop = mMapIdToProperty["num_outputs"];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);

        miNumOutputs = newNum;
        mvOutputData.resize(miNumOutputs);
        mvOutputTypeIndices.resize(miNumOutputs, PortTypeImage);

        // Update property value first so the property browser rebuild sees the latest value.
        typedProp->getData().miValue = miNumOutputs;

        rebuildPortTypeProperties();

        Q_EMIT property_changed_signal(prop);
        
        Q_EMIT embeddedWidgetSizeUpdated();
    }
}

void PythonEditorModel::onExecutePython()
{
    // Get code from widget
    msPythonCode = mpEmbeddedWidget->getPythonCode();
    
    // Update property
    auto prop = mMapIdToProperty["python_code"];
    auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedProp->getData() = msPythonCode;
    
    // Execute
    executePythonCode();
    
    // Emit data updates
    for (int i = 0; i <= miNumOutputs; ++i)
    {
        Q_EMIT dataUpdated( i );
    }
}

QString PythonEditorModel::serializeInputData(int index)
{
    if (index >= static_cast<int>(mvInputData.size()) || !mvInputData[index])
        return "null";
    
    auto data = mvInputData[index];
    QJsonObject json;
    
    // Check data type and serialize appropriately
    if (auto imgData = std::dynamic_pointer_cast<CVImageData>(data))
    {
        cv::Mat img = imgData->data();
        if (img.empty())
            return "null";
        
        // Encode image as base64 PNG
        std::vector<uchar> buf;
        cv::imencode(".png", img, buf);
        QByteArray byteArray(reinterpret_cast<const char*>(buf.data()), buf.size());
        
        json["type"] = "image";
        json["data"] = QString::fromLatin1(byteArray.toBase64());
        json["rows"] = img.rows;
        json["cols"] = img.cols;
        json["channels"] = img.channels();
        json["dtype"] = img.type();
    }
    else if (auto doubleData = std::dynamic_pointer_cast<DoubleData>(data))
    {
        json["type"] = "double";
        json["data"] = doubleData->data();
    }
    else if (auto intData = std::dynamic_pointer_cast<IntegerData>(data))
    {
        json["type"] = "int";
        json["data"] = intData->data();
    }
    else if (auto strData = std::dynamic_pointer_cast<StdStringData>(data))
    {
        json["type"] = "string";
        json["data"] = QString::fromStdString(strData->data());
    }
    else if (auto pointData = std::dynamic_pointer_cast<CVPointData>(data))
    {
        cv::Point pt = pointData->data();
        json["type"] = "point";
        json["x"] = pt.x;
        json["y"] = pt.y;
    }
    else if (auto rectData = std::dynamic_pointer_cast<CVRectData>(data))
    {
        cv::Rect rect = rectData->data();
        json["type"] = "rect";
        json["x"] = rect.x;
        json["y"] = rect.y;
        json["width"] = rect.width;
        json["height"] = rect.height;
    }
    else if (auto sizeData = std::dynamic_pointer_cast<CVSizeData>(data))
    {
        cv::Size size = sizeData->data();
        json["type"] = "size";
        json["width"] = size.width;
        json["height"] = size.height;
    }
    else if (auto syncData = std::dynamic_pointer_cast<SyncData>(data))
    {
        json["type"] = "sync";
        json["data"] = syncData->data();
    }
    else
    {
        return "null";
    }
    
    QJsonDocument doc(json);
    return doc.toJson(QJsonDocument::Compact);
}

bool PythonEditorModel::deserializeOutputData(int index, const QString& jsonStr, QString& errorMessage)
{
    if (jsonStr.isEmpty() || jsonStr == "null")
    {
        errorMessage = QStringLiteral("Output%1 is empty or null").arg(index);
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isObject())
    {
        errorMessage = QStringLiteral("Output%1 is not a valid JSON object").arg(index);
        return false;
    }
    
    QJsonObject json = doc.object();
    const QString type = json["type"].toString();
    if (type.isEmpty())
    {
        errorMessage = QStringLiteral("Output%1 has no type field").arg(index);
        return false;
    }
    
    if (type == "image")
    {
        // Decode base64 PNG
        QByteArray byteArray = QByteArray::fromBase64(json["data"].toString().toLatin1());
        std::vector<uchar> buf(byteArray.begin(), byteArray.end());
        cv::Mat img = cv::imdecode(buf, cv::IMREAD_UNCHANGED);
        
        if (!img.empty())
        {
            auto imgData = std::make_shared<CVImageData>();
            imgData->set_image(img);
            mvOutputData[index] = imgData;
            return true;
        }

        errorMessage = QStringLiteral("Output%1 image decode failed").arg(index);
        return false;
    }
    else if (type == "double")
    {
        auto doubleData = std::make_shared<DoubleData>();
        doubleData->data() = json["data"].toDouble();
        mvOutputData[index] = doubleData;
        return true;
    }
    else if (type == "int")
    {
        auto intData = std::make_shared<IntegerData>();
        intData->data() = json["data"].toInt();
        mvOutputData[index] = intData;
        return true;
    }
    else if (type == "string")
    {
        auto strData = std::make_shared<StdStringData>();
        strData->data() = json["data"].toString().toStdString();
        mvOutputData[index] = strData;
        return true;
    }
    else if (type == "point")
    {
        auto pointData = std::make_shared<CVPointData>();
        pointData->data() = cv::Point(json["x"].toInt(), json["y"].toInt());
        mvOutputData[index] = pointData;
        return true;
    }
    else if (type == "rect")
    {
        auto rectData = std::make_shared<CVRectData>();
        rectData->data() = cv::Rect(json["x"].toInt(), json["y"].toInt(),
                                     json["width"].toInt(), json["height"].toInt());
        mvOutputData[index] = rectData;
        return true;
    }
    else if (type == "size")
    {
        auto sizeData = std::make_shared<CVSizeData>();
        sizeData->data() = cv::Size(json["width"].toInt(), json["height"].toInt());
        mvOutputData[index] = sizeData;
        return true;
    }
    else if (type == "sync")
    {
        auto syncData = std::make_shared<SyncData>();
        syncData->set_data(json["data"].toBool());
        mvOutputData[index] = syncData;
        return true;
    }

    errorMessage = QStringLiteral("Output%1 has unsupported type '%2'").arg(index).arg(type);
    return false;
}

void PythonEditorModel::executePythonCode()
{
    if (msPythonCode.trimmed().isEmpty())
    {
        mpExecutionInfo->set_information("Error: No code to execute");
        mbExecutionSuccess = false;
        return;
    }

    // Create Python script that handles data exchange
    QString pythonScript = R"(
import sys
import json
import base64
import numpy as np
try:
    import cv2
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False

# Parse input data
inputs_json = sys.argv[1] if len(sys.argv) > 1 else '{}'
num_outputs = int(sys.argv[2]) if len(sys.argv) > 2 else 0

inputs = json.loads(inputs_json)

# Deserialize inputs
for key, value in inputs.items():
    if value is None or value == 'null':
        continue
    
    data = json.loads(value)
    data_type = data.get('type')
    
    if data_type == 'image' and HAS_CV2:
        # Decode image
        img_data = base64.b64decode(data['data'])
        nparr = np.frombuffer(img_data, np.uint8)
        img = cv2.imdecode(nparr, cv2.IMREAD_UNCHANGED)
        globals()[key] = img
    elif data_type == 'double':
        globals()[key] = float(data['data'])
    elif data_type == 'int':
        globals()[key] = int(data['data'])
    elif data_type == 'string':
        globals()[key] = str(data['data'])
    elif data_type == 'point':
        globals()[key] = {'x': data['x'], 'y': data['y']}
    elif data_type == 'rect':
        globals()[key] = {'x': data['x'], 'y': data['y'], 'width': data['width'], 'height': data['height']}
    elif data_type == 'size':
        globals()[key] = {'width': data['width'], 'height': data['height']}
    elif data_type == 'sync':
        globals()[key] = bool(data['data'])

# User code execution
try:
)";

    // Add user code (indented)
    QStringList lines = msPythonCode.split('\n');
    for (const QString& line : lines)
    {
        pythonScript += "    " + line + "\n";
    }

    pythonScript += R"(
except Exception as e:
    print(f"ERROR: {type(e).__name__}: {e}", file=sys.stderr)
    sys.exit(1)

# Serialize outputs
outputs = {}
try:
    for i in range(num_outputs):
        output_name = f'output{i}'
        if output_name in globals():
            value = globals()[output_name]
            
            if HAS_CV2 and isinstance(value, np.ndarray):
                # Encode image
                _, buffer = cv2.imencode('.png', value)
                img_base64 = base64.b64encode(buffer).decode('utf-8')
                outputs[output_name] = json.dumps({
                    'type': 'image',
                    'data': img_base64,
                    'rows': value.shape[0],
                    'cols': value.shape[1],
                    'channels': value.shape[2] if len(value.shape) > 2 else 1,
                    'dtype': int(value.dtype.num)
                })
            elif isinstance(value, (bool, np.bool_)):
                outputs[output_name] = json.dumps({'type': 'sync', 'data': bool(value)})
            elif isinstance(value, (int, np.integer)):
                outputs[output_name] = json.dumps({'type': 'int', 'data': int(value)})
            elif isinstance(value, (float, np.floating)):
                outputs[output_name] = json.dumps({'type': 'double', 'data': float(value)})
            elif isinstance(value, str):
                outputs[output_name] = json.dumps({'type': 'string', 'data': value})
            elif isinstance(value, dict):
                if 'x' in value and 'y' in value and 'width' not in value:
                    outputs[output_name] = json.dumps({'type': 'point', 'x': value['x'], 'y': value['y']})
                elif 'x' in value and 'y' in value and 'width' in value and 'height' in value:
                    outputs[output_name] = json.dumps({'type': 'rect', 'x': value['x'], 'y': value['y'], 'width': value['width'], 'height': value['height']})
                elif 'width' in value and 'height' in value:
                    outputs[output_name] = json.dumps({'type': 'size', 'width': value['width'], 'height': value['height']})
                else:
                    raise ValueError(f"{output_name} dict format is not supported")
            else:
                raise TypeError(f"{output_name} has unsupported type: {type(value).__name__}")
except Exception as e:
    print(f"ERROR: {type(e).__name__}: {e}", file=sys.stderr)
    sys.exit(1)

print(json.dumps(outputs))
)";

    // Write script to temporary file
    QTemporaryFile scriptFile;
    scriptFile.setAutoRemove(true);
    scriptFile.setFileTemplate(QDir::tempPath() + "/cvdev_python_XXXXXX.py");
    
    if (!scriptFile.open())
    {
        mpExecutionInfo->set_information("Error: Cannot create temporary script file");
        mbExecutionSuccess = false;
        return;
    }
    
    scriptFile.write(pythonScript.toUtf8());
    scriptFile.flush();
    QString scriptPath = scriptFile.fileName();

    // Prepare input data JSON
    QJsonObject inputsJson;
    for (int i = 0; i < miNumInputs; ++i)
    {
        QString key = QStringLiteral("input") + QString::number(i);
        inputsJson[key] = serializeInputData(i);
    }
    QJsonDocument inputsDoc(inputsJson);
    QString inputsStr = inputsDoc.toJson(QJsonDocument::Compact);

    // Execute Python script
    QProcess process;
    QStringList args;
    args << scriptPath << inputsStr << QString::number(miNumOutputs);
    
    process.start("python3", args);
    
    if (!process.waitForStarted(3000))
    {
        mpExecutionInfo->set_information("Error: Cannot start Python interpreter");
        mbExecutionSuccess = false;
        return;
    }
    
    if (!process.waitForFinished(30000)) // 30 second timeout
    {
        process.kill();
        mpExecutionInfo->set_information("Error: Execution timeout (30s)");
        mbExecutionSuccess = false;
        return;
    }

    // Check for errors
    QString stderr_output = process.readAllStandardError();
    if (process.exitCode() != 0 || !stderr_output.isEmpty())
    {
        QString errorMsg = "Execution failed:\n" + stderr_output;
        mpExecutionInfo->set_information(errorMsg.left(500));
        mbExecutionSuccess = false;
        qWarning() << "Python execution error:" << stderr_output;
        return;
    }

    // Parse outputs
    QString stdout_output = process.readAllStandardOutput();
    QJsonDocument outputsDoc = QJsonDocument::fromJson(stdout_output.toUtf8());
    
    if (!outputsDoc.isObject())
    {
        mpExecutionInfo->set_information("Error: Invalid output format");
        mbExecutionSuccess = false;
        return;
    }

    QJsonObject outputsJson = outputsDoc.object();
    for (int i = 0; i < miNumOutputs; ++i)
    {
        QString key = QStringLiteral("output") + QString::number(i);
        if (outputsJson.contains(key))
        {
            if (!outputsJson[key].isString())
            {
                mpExecutionInfo->set_information(QStringLiteral("Error: output%1 payload must be a JSON string").arg(i));
                mbExecutionSuccess = false;
                return;
            }

            QString errorMessage;
            if (!deserializeOutputData(i, outputsJson[key].toString(), errorMessage))
            {
                mpExecutionInfo->set_information(QStringLiteral("Error: %1").arg(errorMessage.left(450)));
                mbExecutionSuccess = false;
                return;
            }
        }
    }

    mpExecutionInfo->set_information("Execution successful ✓");
    mbExecutionSuccess = true;
    
    scriptFile.close(); // Auto-removed
}
