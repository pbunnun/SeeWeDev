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
#include <QMetaObject>
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

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msPythonExecutable;
    filePathPropertyType.msFilter = "*";
    filePathPropertyType.msMode = "open";

    propId = "python_executable";
    auto propExe = std::make_shared<TypedProperty<FilePathPropertyType>>("Python Executable", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType, "Code");
    mvProperty.push_back(propExe);
    mMapIdToProperty[propId] = propExe;

    // Connect widget signals
    connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::executeClicked,
            this, &PythonEditorModel::onExecutePython);
    connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::numInputsChanged,
            this, &PythonEditorModel::onNumInputsChanged);
    connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::numOutputsChanged,
            this, &PythonEditorModel::onNumOutputsChanged);
    connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::widgetClicked,
            this, &PythonEditorModel::selection_request_signal);

    connect(mpEmbeddedWidget, &PythonEditorEmbeddedWidget::editableWidgetSelectedChanged,
            this, &PBNodeDelegateModel::editable_embedded_widget_selected_changed);
}

PythonEditorModel::~PythonEditorModel()
{
    if( mpSessionWorker && mpWorkerThread && mpWorkerThread->isRunning() )
    {
        QMetaObject::invokeMethod( mpSessionWorker, "stopSession", Qt::BlockingQueuedConnection );
    }

    if( mpWorkerThread )
    {
        mpWorkerThread->quit();
        mpWorkerThread->wait( 3000 );
    }
}

void PythonEditorModel::late_constructor()
{
    if( !start_late_constructor() )
        return;

    mpWorkerThread = new QThread( this );
    mpSessionWorker = new PythonSessionWorker();
    mpSessionWorker->moveToThread( mpWorkerThread );

    QObject::connect( mpWorkerThread,
                      &QThread::finished,
                      mpSessionWorker,
                      &QObject::deleteLater );

    QObject::connect( mpSessionWorker,
                      &PythonSessionWorker::sessionStarted,
                      this,
                      &PythonEditorModel::onSessionStarted );

    QObject::connect( mpSessionWorker,
                      &PythonSessionWorker::sessionFailed,
                      this,
                      &PythonEditorModel::onSessionFailed );

    QObject::connect( mpSessionWorker,
                      &PythonSessionWorker::resultReady,
                      this,
                      &PythonEditorModel::onResultReady );

    QObject::connect( mpSessionWorker,
                      &PythonSessionWorker::executionError,
                      this,
                      &PythonEditorModel::onExecutionError );

    mpWorkerThread->start();
    QMetaObject::invokeMethod( mpSessionWorker,
                               "setExecutablePath",
                               Qt::QueuedConnection,
                               Q_ARG( QString, msPythonExecutable ) );
    QMetaObject::invokeMethod( mpSessionWorker, "startSession", Qt::QueuedConnection );
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
    return QStringList({"Image", "Integer", "Double", "String", "Point", "Rect", "Size", "Sync", "Info"});
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
    case PortTypeInfo:
        return InformationData().type();
    case PortTypeImage:
    default:
        return CVImageData().type();
    }
}

int PythonEditorModel::normalizePortTypeIndex(int typeIndex) const
{
    constexpr int minIndex = static_cast<int>(PortTypeImage);
    constexpr int maxIndex = static_cast<int>(PortTypeInfo);
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
            emitOutputPort(static_cast<PortIndex>(index));
        }
    }
    else if (id == "python_code")
    {
        msPythonCode = value.toString();
        mpEmbeddedWidget->setPythonCode(msPythonCode);
    }
    else if (id == "python_executable")
    {
        msPythonExecutable = value.toString().trimmed().isEmpty() ? QStringLiteral("python3") : value.toString().trimmed();
        auto prop = mMapIdToProperty["python_executable"];
        auto typedProp = std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(prop);
        typedProp->getData().msFilename = msPythonExecutable;
    }
}

QJsonObject PythonEditorModel::save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    
    QJsonObject cParams;
    cParams["num_inputs"] = miNumInputs;
    cParams["num_outputs"] = miNumOutputs;
    cParams["python_code"] = mpEmbeddedWidget->getPythonCode();
    cParams["python_executable"] = msPythonExecutable;

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

        v = paramsObj["python_executable"];
        if (!v.isNull() && !v.toString().trimmed().isEmpty())
        {
            msPythonExecutable = v.toString().trimmed();
            auto prop = mMapIdToProperty["python_executable"];
            auto typedProp = std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(prop);
            typedProp->getData().msFilename = msPythonExecutable;
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

    // Force execution for explicit button clicks even when no new input arrived.
    mbPendingExecution = true;
    executePythonCode();
}

void PythonEditorModel::onSessionStarted()
{
    mbSessionRunning = true;

    if( mbPendingExecution && !mbBusy )
        executePythonCode();
}

void PythonEditorModel::onSessionFailed( const QString& errorMessage )
{
    mbSessionRunning = false;
    mbBusy = false;

    mpExecutionInfo->set_information( QStringLiteral( "Session failed: %1" ).arg( errorMessage.left( 420 ) ) );
    mbExecutionSuccess = false;
    emitOutputPort( static_cast<PortIndex>( miNumOutputs ) );
}

void PythonEditorModel::onResultReady( const QString& outputsJson )
{
    mbBusy = false;

    QJsonDocument outputsDoc = QJsonDocument::fromJson( outputsJson.toUtf8() );
    if( !outputsDoc.isObject() )
    {
        mpExecutionInfo->set_information( "Error: Invalid output format" );
        mbExecutionSuccess = false;
        emitOutputPort( static_cast<PortIndex>( miNumOutputs ) );
        return;
    }

    const QJsonObject outputsObj = outputsDoc.object();
    for( int i = 0; i < miNumOutputs; ++i )
    {
        const QString key = QStringLiteral( "output" ) + QString::number( i );
        if( !outputsObj.contains( key ) )
            continue;

        if( !outputsObj[key].isString() )
        {
            mpExecutionInfo->set_information( QStringLiteral( "Error: output%1 payload must be a JSON string" ).arg( i ) );
            mbExecutionSuccess = false;
            emitOutputPort( static_cast<PortIndex>( miNumOutputs ) );
            return;
        }

        QString errorMessage;
        if( !deserializeOutputData( i, outputsObj[key].toString(), errorMessage ) )
        {
            mpExecutionInfo->set_information( QStringLiteral( "Error: %1" ).arg( errorMessage.left( 450 ) ) );
            mbExecutionSuccess = false;
            emitOutputPort( static_cast<PortIndex>( miNumOutputs ) );
            return;
        }
    }

    mpExecutionInfo->set_information( "Execution successful ✓" );
    mbExecutionSuccess = true;

    for( int i = 0; i <= miNumOutputs; ++i )
        emitOutputPort( static_cast<PortIndex>( i ) );

    if( mbPendingExecution )
        executePythonCode();
}

void PythonEditorModel::onExecutionError( const QString& errorMessage )
{
    mbBusy = false;

    mpExecutionInfo->set_information( QStringLiteral( "Execution failed:\n%1" ).arg( errorMessage.left( 460 ) ) );
    mbExecutionSuccess = false;
    emitOutputPort( static_cast<PortIndex>( miNumOutputs ) );

    if( mbPendingExecution )
        executePythonCode();
}

QString PythonEditorModel::buildInputsJson()
{
    QJsonObject inputsJson;
    for( int i = 0; i < miNumInputs; ++i )
    {
        const QString key = QStringLiteral( "input" ) + QString::number( i );
        inputsJson[key] = serializeInputData( i );
    }

    return QString::fromUtf8( QJsonDocument( inputsJson ).toJson( QJsonDocument::Compact ) );
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
        if (auto infoData = std::dynamic_pointer_cast<InformationData>(data))
        {
        json["type"] = "info";
        json["data"] = infoData->info();
        json["timestamp"] = static_cast<qint64>(infoData->timestamp());
        }
        else
        {
        json["type"] = "string";
        json["data"] = QString::fromStdString(strData->data());
        }
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
        const int expectedTypeIndex = (index >= 0 && index < static_cast<int>(mvOutputTypeIndices.size()))
                                      ? normalizePortTypeIndex(mvOutputTypeIndices[index])
                                      : PortTypeImage;
        if( expectedTypeIndex == PortTypeInfo )
        {
            auto infoData = std::make_shared<InformationData>();
            infoData->set_information(json["data"].toString());
            mvOutputData[index] = infoData;
            if (json.contains("timestamp"))
                infoData->set_timestamp(static_cast<long int>(json["timestamp"].toVariant().toLongLong()));
            mvOutputData[index] = infoData;
            return true;
        }
        else if( expectedTypeIndex == PortTypeString )
        {
            auto strData = std::make_shared<StdStringData>();
            strData->data() = json["data"].toString().toStdString();
            mvOutputData[index] = strData;
            return true;
        }
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

    if( !mpSessionWorker )
    {
        mpExecutionInfo->set_information( "Error: Python session is not initialized" );
        mbExecutionSuccess = false;
        return;
    }

    const bool codeChanged = ( msPythonCode != msLastExecutedCode );
    const bool executableChanged = ( msPythonExecutable != msLastSessionExecutable );
    if( codeChanged || executableChanged )
    {
        msLastExecutedCode = msPythonCode;
        msLastSessionExecutable = msPythonExecutable;
        mbSessionRunning = false;
        mbBusy = false;
        mbPendingExecution = true;

        QMetaObject::invokeMethod( mpSessionWorker,
                                   "setExecutablePath",
                                   Qt::QueuedConnection,
                                   Q_ARG( QString, msPythonExecutable ) );
        QMetaObject::invokeMethod( mpSessionWorker, "stopSession", Qt::QueuedConnection );
        QMetaObject::invokeMethod( mpSessionWorker, "startSession", Qt::QueuedConnection );
        return;
    }

    if( !mbSessionRunning )
    {
        mbPendingExecution = true;
        return;
    }

    if( mbBusy )
    {
        mbPendingExecution = true;
        return;
    }

    const QString inputsJson = buildInputsJson();
    mbBusy = true;
    mbPendingExecution = false;

    QMetaObject::invokeMethod( mpSessionWorker,
                               "executeFrame",
                               Qt::QueuedConnection,
                               Q_ARG( QString, msPythonCode ),
                               Q_ARG( QString, inputsJson ),
                               Q_ARG( int, miNumOutputs ) );
}

QString
PythonEditorModel::
portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In)
    {
        return QString("Input %1: Generic Python input payload.").arg(portIndex);
    }
    else if (portType == QtNodes::PortType::Out)
    {
        if (portIndex == static_cast<QtNodes::PortIndex>(miNumOutputs))
            return "Execution Info: Python stdout and tracebacks.";
        return QString("Output %1: Generic Python output payload.").arg(portIndex);
    }
    return PBNodeDelegateModel::portToolTip(portType, portIndex);
}
