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

#include "PBDataFlowGraphModel.hpp"
#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"
#include "PBNodeGroup.hpp"
#include "TransportModeManager.hpp"
#include "ZenohBridge.hpp"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>
#include <QSize>
#include <QTimer>
#include <set>
#include <stack>

using QtNodes::NodeDelegateModelRegistry;
using QtNodes::NodeId;
using QtNodes::NodeRole;

PBDataFlowGraphModel::
PBDataFlowGraphModel(std::shared_ptr<NodeDelegateModelRegistry> registry, QObject *parent)
    : QtNodes::DataFlowGraphModel(registry)
{
    Q_UNUSED(parent);
    setTransportFlowFilename(QStringLiteral("Untitle"));
}

void
PBDataFlowGraphModel::
setTransportFlowFilename(const QString& filename)
{
    const QString baseName = QFileInfo(filename).completeBaseName().trimmed();
    msFlowFilename = baseName.isEmpty() ? QStringLiteral("Untitle") : baseName;
    updateAllNodeTransportContext();
}

NodeId
PBDataFlowGraphModel::
addNode(QString const nodeType)
{
    if (mbPresetMode)
        return QtNodes::InvalidNodeId;

    // Call parent's addNode to create the node
    NodeId newId = DataFlowGraphModel::addNode(nodeType);
    
    if (newId != QtNodes::InvalidNodeId) {
        // Get the delegate model
        auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(newId);
        
        if (delegateModel) {
            delegateModel->setRuntimeTransportContext(newId, msFlowFilename);
            // Connect embeddedWidgetSizeUpdated to trigger node visual update
            connect(delegateModel,
                    &QtNodes::NodeDelegateModel::embeddedWidgetSizeUpdated,
                    this,
                    [this, newId]() {
                        Q_EMIT nodeUpdated(newId);
                    });
            // Call late_constructor() here to perform any heavy initialization
            // (threads, hardware, etc.) only when the node is actually placed
            // into the scene. This centralizes the deferred initialization so
            // that registry/menu instantiations remain lightweight.
            delegateModel->late_constructor();

            if (auto w = nodeData(newId, NodeRole::Widget).value<QWidget *>()) {
                w->setEnabled(true);
            }
        }
    }
    
    return newId;
}

QJsonObject
PBDataFlowGraphModel::
save() const
{
    // Save current preset parameters if active
    if (mbPresetMode) {
        const_cast<PBDataFlowGraphModel*>(this)->snapshotActivePreset();
    }

    // Get base graph data (nodes, connections)
    QJsonObject jsonObj = DataFlowGraphModel::save();
    
    // Add groups
    if (!mGroups.empty()) {
        QJsonArray groupsArray;
        for (const auto& pair : mGroups) {
            groupsArray.append(pair.second.save());
        }
        jsonObj["groups"] = groupsArray;
    }
    
    jsonObj["readOnly"] = mbReadOnly;
    
    // Add presets
    if (mbPresetMode) {
        QJsonObject presetsObj;
        presetsObj["active"] = msActivePreset;
        
        QJsonArray orderArray;
        for (const QString& name : mPresetOrder) {
            orderArray.append(name);
        }
        presetsObj["order"] = orderArray;
        
        QJsonObject setsObj;
        for (auto it = mPresets.begin(); it != mPresets.end(); ++it) {
            QJsonObject setObj;
            for (auto nodeIt = it.value().begin(); nodeIt != it.value().end(); ++nodeIt) {
                setObj[nodeIt.key()] = nodeIt.value();
            }
            setsObj[it.key()] = setObj;
        }
        presetsObj["sets"] = setsObj;
        
        jsonObj["presets"] = presetsObj;
    }
    
    QJsonObject viewState;
    viewState["centerX"] = mViewportCenter.x();
    viewState["centerY"] = mViewportCenter.y();
    viewState["zoomScale"] = mZoomScale;
    viewState["hideCategory"] = mHideCategory;
    viewState["hideProperties"] = mHideProperties;
    viewState["hideWorkspace"] = mHideWorkspace;
    viewState["inFocusView"] = mInFocusView;
    viewState["inFullScreen"] = mInFullScreen;
    viewState["snapToGrid"] = mSnapToGrid;
    jsonObj["viewState"] = viewState;
    
    return jsonObj;
}

void
PBDataFlowGraphModel::
load(const QJsonObject& json)
{
    mbRestoringGraph = true;

    if (json.contains("viewState")) {
        QJsonObject viewState = json["viewState"].toObject();
        mViewportCenter = QPointF(viewState["centerX"].toDouble(), viewState["centerY"].toDouble());
        mZoomScale = viewState["zoomScale"].toDouble();
        mHideCategory = viewState["hideCategory"].toBool();
        mHideProperties = viewState["hideProperties"].toBool();
        mHideWorkspace = viewState["hideWorkspace"].toBool();
        mInFocusView = viewState["inFocusView"].toBool();
        mInFullScreen = viewState["inFullScreen"].toBool();
        mSnapToGrid = viewState["snapToGrid"].toBool(false);
    } else {
        mViewportCenter = QPointF(0.0, 0.0);
        mZoomScale = 1.0;
        mHideCategory = false;
        mHideProperties = false;
        mHideWorkspace = false;
        mInFocusView = false;
        mInFullScreen = false;
        mSnapToGrid = false;
    }

    // Load base graph data (nodes, connections)
    DataFlowGraphModel::load(json);

    mbReadOnly = json.value("readOnly").toBool(false);

    // Load groups
    mGroups.clear();
    mNextGroupId = 1;
    
    if (json.contains("groups") && json["groups"].isArray()) {
        QJsonArray groupsArray = json["groups"].toArray();
        for (const QJsonValue& value : groupsArray) {
            if (value.isObject()) {
                PBNodeGroup group;
                group.load(value.toObject());
                
                if (group.id() != InvalidGroupId) {
                    mGroups[group.id()] = group;
                    
                    // Update next group ID
                    if (group.id() >= mNextGroupId) {
                        mNextGroupId = group.id() + 1;
                    }
                    
                    // Emit signal for each loaded group
                    Q_EMIT groupCreated(group.id());
                }
            }
        }
    }

    // Load presets
    mbPresetMode = false;
    msActivePreset.clear();
    mPresetOrder.clear();
    mPresets.clear();
    
    if (json.contains("presets") && json["presets"].isObject()) {
        QJsonObject presetsObj = json["presets"].toObject();
        mbPresetMode = true;
        msActivePreset = presetsObj["active"].toString();
        
        QJsonArray orderArray = presetsObj["order"].toArray();
        for (const QJsonValue& val : orderArray) {
            mPresetOrder << val.toString();
        }
        
        QJsonObject setsObj = presetsObj["sets"].toObject();
        for (auto it = setsObj.begin(); it != setsObj.end(); ++it) {
            QJsonObject setObj = it.value().toObject();
            QMap<QString, QJsonObject> nodeMap;
            for (auto nodeIt = setObj.begin(); nodeIt != setObj.end(); ++nodeIt) {
                nodeMap[nodeIt.key()] = nodeIt.value().toObject();
            }
            mPresets[it.key()] = nodeMap;
        }
        
        // Apply active preset parameters to the newly loaded nodes
        if (!msActivePreset.isEmpty() && mPresets.contains(msActivePreset)) {
            const auto& presetData = mPresets[msActivePreset];
            for (NodeId nodeId : allNodeIds()) {
                auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(nodeId);
                if (delegateModel) {
                    QString idStr = QString::number(nodeId);
                    if (presetData.contains(idStr)) {
                        QJsonObject fullJson = delegateModel->save();
                        fullJson["cParams"] = presetData[idStr];
                        delegateModel->load(fullJson);
                    }
                }
            }
        }
    }

    mbRestoringGraph = false;

    triggerInitialPropagation();

    if (mbPresetMode) {
        Q_EMIT presetModeChanged(true);
        Q_EMIT presetsChanged();
    }
}

bool
PBDataFlowGraphModel::
save_to_file(QString const & sFilename) const
{
    if( !sFilename.isEmpty() )
    {
        QFile file(sFilename);
        if( file.open(QIODevice::WriteOnly) )
        {
            QJsonObject jsonObj = save();  // Call PBDataFlowGraphModel::save() to include groups
            QJsonDocument jsonDoc(jsonObj);
            file.write(jsonDoc.toJson());
            return true;
        }
        else
            return false;
    }
    return false;
}

bool
PBDataFlowGraphModel::
load_from_file(QString const & sFilename)
{
    if( !QFileInfo::exists(sFilename) )
        return false;

    // Update flow filename scope before loading nodes so loaded delegates get the right key context.
    setTransportFlowFilename(sFilename);

    QFile file(sFilename);
    if( !file.open(QIODevice::ReadOnly) )
        return false;

    QByteArray wholeFile = file.readAll();
    
    QJsonDocument jsonDoc = QJsonDocument::fromJson(wholeFile);
    if( !jsonDoc.isObject() )
        return false;
    
    // Clear the list of load errors before loading
    mLoadErrors.clear();
    
    load(jsonDoc.object());

    // Show dialog if any nodes had errors during loading
    if (!mLoadErrors.isEmpty()) {
        // Remove duplicates
        QStringList uniqueErrors = mLoadErrors;
        uniqueErrors.removeDuplicates();
        
        QString message = "The following errors occurred while loading nodes:\n\n";
        message += uniqueErrors.join("\n");
        message += "\n\nThese nodes have been skipped.";
        
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Node Loading Warning");
        msgBox.setText("Some nodes could not be loaded");
        msgBox.setInformativeText(message);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();

        return false;
    }
    return true;
}

QJsonObject
PBDataFlowGraphModel::
saveNode(NodeId const nodeId) const
{
    // Get the base node data from parent class
    QJsonObject nodeJson = DataFlowGraphModel::saveNode(nodeId);
    
    // Save the embedded widget's size (not the node's size)
    // The node size includes margins, captions, ports, etc. and is recalculated from the widget
    if (auto w = nodeData(nodeId, NodeRole::Widget).value<QWidget *>()) {
        auto *delegateModel = const_cast<PBDataFlowGraphModel*>(this)->delegateModel<PBNodeDelegateModel>(nodeId);
        QSize widgetSize;
        if (delegateModel && delegateModel->isMinimize()) {
            widgetSize = delegateModel->savedWidgetSize();
        } else {
            widgetSize = w->size();
        }

        if (widgetSize.isValid() && widgetSize.width() > 0 && widgetSize.height() > 0) {
            QJsonObject widgetSizeJson;
            widgetSizeJson["width"] = widgetSize.width();
            widgetSizeJson["height"] = widgetSize.height();
            nodeJson["widget-size"] = widgetSizeJson;
        }
    }
    
    return nodeJson;
}

void
PBDataFlowGraphModel::
loadNode(QJsonObject const &nodeJson)
{
    /*
     * Node lifecycle note (brief):
     * --------------------------------
     * This function is the PBDataFlowGraphModel's load-time hook that wraps
     * the NodeEditor base behavior. Key ordering to understand:
     *
     * 1) DataFlowGraphModel::loadNode(nodeJson) will create the delegate via
     *    the registry, store it under the restored id and emit nodeCreated(id).
     *    It will then call the delegate's load(internalDataJson) so the model
     *    can restore its internal state.
     *
     * 2) After the base loadNode returns, PBDataFlowGraphModel performs
     *    post-load steps: it connects embedded-widget resize signals, invokes
     *    delegateModel->late_constructor() to perform deferred/heavy init, and
     *    restores any saved embedded-widget size (with a single-shot re-apply
     *    to handle immediate resizes the model may perform during its load).
     *
     * Rationale: centralizing late_constructor() here (and in addNode()) keeps
     * heavy initialization out of registry/menu instantiation and provides a
     * single place to reason about ordering and race conditions.
     */
    // Validate required fields exist
    if (!nodeJson.contains("id") || !nodeJson.contains("internal-data")) {
        mLoadErrors.append("Missing required fields in JSON (id or internal-data)");
        return;
    }
    
    // Get the internal data object
    QJsonObject internalData = nodeJson["internal-data"].toObject();
    if (!internalData.contains("model-name")) {
        mLoadErrors.append("Missing model-name in internal-data");
        return;
    }
    
    // Get the node type (model-name) to validate it's registered
    QString nodeType = internalData["model-name"].toString();
    
    // Validate that the node type is registered
    auto registry = dataModelRegistry();
    if (!registry) {
        mLoadErrors.append("Registry not available");
        return;
    }
    
    const auto& registeredModels = registry->registeredModelsCategoryAssociation();
    if (registeredModels.find(nodeType) == registeredModels.end()) {
        mLoadErrors.append(QString("Node type not registered: %1").arg(nodeType));
        return;
    }
    
    // Get the node ID that will be restored (before calling parent's loadNode)
    NodeId restoredNodeId = nodeJson["id"].toInt();
    
    // Call parent's loadNode to create the node
    DataFlowGraphModel::loadNode(nodeJson);
    
    // Verify the node was actually created
    if (!nodeExists(restoredNodeId)) {
        return;
    }
    
    // Connect embeddedWidgetSizeUpdated signal for the loaded node
    auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(restoredNodeId);
    if (delegateModel) {
        delegateModel->setRuntimeTransportContext(restoredNodeId, msFlowFilename);
        connect(delegateModel,
                &QtNodes::NodeDelegateModel::embeddedWidgetSizeUpdated,
                this,
                [this, restoredNodeId]() {
                    Q_EMIT nodeUpdated(restoredNodeId);
                });
        // Perform deferred/late initialization for the delegate model here so
        // that any heavy work (threads, hardware, etc.) only runs when the
        // node is actually present in the scene during load. This mirrors the
        // behavior used when nodes are created via addNode().
        delegateModel->late_constructor();

        if (auto w = nodeData(restoredNodeId, NodeRole::Widget).value<QWidget *>()) {
            w->setEnabled(true);
        }

        if (delegateModel->isMinimize()) {
            QTimer::singleShot(0, this, [this, restoredNodeId]() {
                Q_EMIT nodeUpdated(restoredNodeId);
            });
        }
    }
    
    // Now restore the embedded widget's size if it was saved
    if (nodeJson.contains("widget-size"))
    {
        QJsonObject widgetSizeJson = nodeJson["widget-size"].toObject();
        int width = widgetSizeJson["width"].toInt();
        int height = widgetSizeJson["height"].toInt();
        
        if (width > 0 && height > 0)
        {
            QSize widgetSize(width, height);
            
            // Resize the embedded widget to the saved size. Some embedded widgets
            // can perform additional synchronous resizing during their load/late
            // constructor (or as a side-effect of starting worker threads). In
            // that case the restored size may be overridden immediately after
            // this call. To make the saved size stick, re-assert it once more
            // after the current event loop turn using a single-shot timer.
            if (auto w = nodeData(restoredNodeId, NodeRole::Widget).value<QWidget *>()) {
                w->resize(widgetSize);
                if (delegateModel) {
                    delegateModel->setSavedWidgetSize(widgetSize);
                }

                // Trigger node update to recalculate node geometry based on new widget size
                Q_EMIT nodeUpdated(restoredNodeId);

                // Re-apply after a short delay to catch any immediate resizes, ex: CV Video Loader
                // RUT : TODO Should not have this hack to fix the problem.
                QTimer::singleShot(0, this, [this, restoredNodeId, widgetSize, delegateModel]() {
                    if (auto w2 = nodeData(restoredNodeId, NodeRole::Widget).value<QWidget *>()) {
                        w2->resize(widgetSize);
                        if (delegateModel) {
                            delegateModel->setSavedWidgetSize(widgetSize);
                        }
                        Q_EMIT nodeUpdated(restoredNodeId);
                    }
                });
            }
        }
    }
}

void
PBDataFlowGraphModel::
updateAllNodeTransportContext()
{
    for (const auto nodeId : allNodeIds()) {
        auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(nodeId);
        if (delegateModel) {
            delegateModel->setRuntimeTransportContext(nodeId, msFlowFilename);
        }
    }
}

QVariant
PBDataFlowGraphModel::
nodeData(NodeId nodeId, NodeRole role) const
{
    // For Style role, return the delegate model's style instead of global style
    if (role == NodeRole::Style) {
        auto *model = const_cast<PBDataFlowGraphModel*>(this)->delegateModel<PBNodeDelegateModel>(nodeId);
        if (model) {
            // Return the node's custom style as a QVariant
            auto style = model->nodeStyle();
            return style.toJson().toVariantMap();
        }
    }
    
    // For all other roles, use the parent implementation
    return DataFlowGraphModel::nodeData(nodeId, role);
}

bool
PBDataFlowGraphModel::
setPortData(NodeId nodeId,
            QtNodes::PortType portType,
            QtNodes::PortIndex portIndex,
            QVariant const &value,
            QtNodes::PortRole role)
{
    if (mbRestoringGraph) {
        return true;
    }

    if (role == QtNodes::PortRole::Data && portType == QtNodes::PortType::In) {
        auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(nodeId);
        if (delegateModel && !delegateModel->isEnable()) {
            return true;
        }
    }

    // For input ports with data role, check if type conversion is needed
    if (role == QtNodes::PortRole::Data && portType == QtNodes::PortType::In) {
        auto incomingData = value.value<std::shared_ptr<QtNodes::NodeData>>();
        
        if (incomingData) {
            // Get the expected input type for this port
            auto expectedType = portData(nodeId, portType, portIndex, QtNodes::PortRole::DataType)
                                    .value<QtNodes::NodeDataType>();
            
            // Get the actual incoming type
            auto incomingType = incomingData->type();
            
            // If types don't match, check if we need to convert
            if (expectedType.id != incomingType.id) {
                // Check if target expects InformationData
                if (expectedType.id == "Information") {
                    // Try to cast incoming data to InformationData
                    // Since CVImageData and other types inherit from InformationData,
                    // we can use dynamic_pointer_cast
                    auto convertedData = std::dynamic_pointer_cast<InformationData>(incomingData);
                    
                    if (convertedData) {
                        // Successfully converted - we need to pass it as std::shared_ptr<NodeData>
                        // Cast it back to NodeData to maintain the proper type for setInData
                        std::shared_ptr<QtNodes::NodeData> nodeDataPtr = 
                            std::static_pointer_cast<QtNodes::NodeData>(convertedData);
                        
                        QVariant convertedValue;
                        convertedValue.setValue(nodeDataPtr);
                        return DataFlowGraphModel::setPortData(nodeId, portType, portIndex, 
                                                               convertedValue, role);
                    }
                }
            }
        }
    }
    
    // For all other cases, use the parent implementation
    return DataFlowGraphModel::setPortData(nodeId, portType, portIndex, value, role);
}

bool
PBDataFlowGraphModel::
connectionPossible(QtNodes::ConnectionId const connectionId) const
{
    if (mbPresetMode)
        return false;

    // First check if the basic parent class validation passes
    // This handles exact type matching, port bounds, port vacancy, and loop detection
    if (DataFlowGraphModel::connectionPossible(connectionId)) {
        return true;
    }
    
    // If exact type matching failed, check if we can convert between types
    // Check if nodes exist
    if (!nodeExists(connectionId.outNodeId) || !nodeExists(connectionId.inNodeId)) {
        return false;
    }
    
    // Get the data types for source and target ports
    auto getDataType = [&](QtNodes::PortType const portType) {
        return portData(getNodeId(portType, connectionId),
                        portType,
                        getPortIndex(portType, connectionId),
                        QtNodes::PortRole::DataType)
            .value<QtNodes::NodeDataType>();
    };
    
    auto outType = getDataType(QtNodes::PortType::Out);
    auto inType = getDataType(QtNodes::PortType::In);
    
    // Check if target type is InformationData
    // In our architecture, all data types inherit from InformationData,
    // so we can accept any data type when the target expects InformationData
    if (inType.id == "Information") {
        // Check port bounds
        auto checkPortBounds = [&](QtNodes::PortType const portType) {
            NodeId const nodeId = getNodeId(portType, connectionId);
            auto portCountRole = (portType == QtNodes::PortType::Out) ? QtNodes::NodeRole::OutPortCount
                                                             : QtNodes::NodeRole::InPortCount;

            std::size_t const portCount = nodeData(nodeId, portCountRole).toUInt();

            return getPortIndex(portType, connectionId) < portCount;
        };
        
        // Check if ports are vacant
        auto portVacant = [&](QtNodes::PortType const portType) {
            NodeId const nodeId = getNodeId(portType, connectionId);
            QtNodes::PortIndex const portIndex = getPortIndex(portType, connectionId);
            auto const connected = connections(nodeId, portType, portIndex);

            auto policy = portData(nodeId, portType, portIndex, QtNodes::PortRole::ConnectionPolicyRole)
                              .value<QtNodes::ConnectionPolicy>();

            return connected.empty() || (policy == QtNodes::ConnectionPolicy::Many);
        };
        
        // Allow connection if basic checks pass
        // Note: We skip the loop check here because we already did it in the parent call
        // and it failed only due to type mismatch
        bool const basicChecks = portVacant(QtNodes::PortType::Out) && portVacant(QtNodes::PortType::In)
                                && checkPortBounds(QtNodes::PortType::Out) && checkPortBounds(QtNodes::PortType::In);
        
        if (!basicChecks) {
            return false;
        }
        
        // Also need to check for loops when allowing type conversion
        // We perform depth-first graph traversal starting from the "Input" port.
        // We should never encounter the starting "Out" node.
        std::stack<NodeId> filo;
        filo.push(connectionId.inNodeId);

        while (!filo.empty()) {
            auto id = filo.top();
            filo.pop();

            // Check all output ports and their connections
            std::size_t const outPortCount = nodeData(id, QtNodes::NodeRole::OutPortCount).toUInt();
            for (QtNodes::PortIndex portIndex = 0; portIndex < outPortCount; ++portIndex) {
                auto connected = connections(id, QtNodes::PortType::Out, portIndex);
                for (auto const &cn : connected) {
                    // If we reached the starting output node, we have a loop
                    if (cn.inNodeId == connectionId.outNodeId) {
                        return false;
                    }
                    filo.push(cn.inNodeId);
                }
            }
        }
        
        return true;
    }
    
    // For other type combinations, don't allow conversion
    return false;
}

bool
PBDataFlowGraphModel::
deleteConnection(QtNodes::ConnectionId const connectionId)
{
    if (mbPresetMode)
        return false;
    return DataFlowGraphModel::deleteConnection(connectionId);
}

// ========== Node Grouping Implementation ==========

GroupId
PBDataFlowGraphModel::
createGroup(const QString& name, const std::set<NodeId>& nodeIds)
{
    if (nodeIds.empty()) {
        return InvalidGroupId;
    }
    
    // Check if any nodes are already in a group
    for (NodeId nodeId : nodeIds) {
        if (getPBNodeGroup(nodeId) != InvalidGroupId) {
            // Remove node from its current group first
            GroupId currentGroup = getPBNodeGroup(nodeId);
            std::set<NodeId> singleNode = {nodeId};
            removeNodesFromGroup(currentGroup, singleNode);
        }
    }
    
    // Create new group
    GroupId newGroupId = mNextGroupId++;
    PBNodeGroup group;
    group.setId(newGroupId);
    group.setName(name);
    
    for (NodeId nodeId : nodeIds) {
        group.addNode(nodeId);
    }
    
    mGroups[newGroupId] = group;
    
    Q_EMIT groupCreated(newGroupId);
    
    return newGroupId;
}

bool
PBDataFlowGraphModel::
dissolveGroup(GroupId groupId)
{
    auto it = mGroups.find(groupId);
    if (it == mGroups.end()) {
        return false;
    }
    
    mGroups.erase(it);
    Q_EMIT groupDissolved(groupId);
    
    return true;
}

bool
PBDataFlowGraphModel::
addNodesToGroup(GroupId groupId, const std::set<NodeId>& nodeIds)
{
    auto it = mGroups.find(groupId);
    if (it == mGroups.end()) {
        return false;
    }
    
    bool anyAdded = false;
    for (NodeId nodeId : nodeIds) {
        // Remove from current group if any
        GroupId currentGroup = getPBNodeGroup(nodeId);
        if (currentGroup != InvalidGroupId && currentGroup != groupId) {
            std::set<NodeId> singleNode = {nodeId};
            removeNodesFromGroup(currentGroup, singleNode);
        }
        
        if (it->second.addNode(nodeId)) {
            anyAdded = true;
        }
    }
    
    if (anyAdded) {
        Q_EMIT groupUpdated(groupId);
    }
    
    return anyAdded;
}

bool
PBDataFlowGraphModel::
removeNodesFromGroup(GroupId groupId, const std::set<NodeId>& nodeIds)
{
    auto it = mGroups.find(groupId);
    if (it == mGroups.end()) {
        return false;
    }
    
    bool anyRemoved = false;
    for (NodeId nodeId : nodeIds) {
        if (it->second.removeNode(nodeId)) {
            anyRemoved = true;
        }
    }
    
    // If group is now empty, dissolve it
    if (it->second.isEmpty()) {
        dissolveGroup(groupId);
        return true;
    }
    
    if (anyRemoved) {
        Q_EMIT groupUpdated(groupId);
    }
    
    return anyRemoved;
}

GroupId
PBDataFlowGraphModel::
getPBNodeGroup(NodeId nodeId) const
{
    for (const auto& pair : mGroups) {
        if (pair.second.contains(nodeId)) {
            return pair.first;
        }
    }
    return InvalidGroupId;
}

const PBNodeGroup*
PBDataFlowGraphModel::
getGroup(GroupId groupId) const
{
    auto it = mGroups.find(groupId);
    if (it != mGroups.end()) {
        return &it->second;
    }
    return nullptr;
}

bool
PBDataFlowGraphModel::
setGroupColor(GroupId groupId, const QColor& color)
{
    auto it = mGroups.find(groupId);
    if (it == mGroups.end()) {
        return false;
    }
    
    it->second.setColor(color);
    Q_EMIT groupUpdated(groupId);
    
    return true;
}

bool
PBDataFlowGraphModel::
setGroupName(GroupId groupId, const QString& name)
{
    auto it = mGroups.find(groupId);
    if (it == mGroups.end()) {
        return false;
    }
    
    it->second.setName(name);
    Q_EMIT groupUpdated(groupId);
    
    return true;
}

bool
PBDataFlowGraphModel::
setGroupMinimized(GroupId groupId, bool minimized)
{
    auto it = mGroups.find(groupId);
    if (it == mGroups.end()) {
        return false;
    }
    
    it->second.setMinimized(minimized);
    Q_EMIT groupUpdated(groupId);
    
    return true;
}

bool
PBDataFlowGraphModel::
toggleGroupMinimized(GroupId groupId)
{
    auto it = mGroups.find(groupId);
    if (it == mGroups.end()) {
        return false;
    }
    
    it->second.setMinimized(!it->second.isMinimized());
    Q_EMIT groupUpdated(groupId);
    
    return true;
}

bool
PBDataFlowGraphModel::
setGroupLocked(GroupId groupId, bool locked)
{
    auto it = mGroups.find(groupId);
    if (it == mGroups.end()) {
        return false;
    }

    it->second.setLocked(locked);
    Q_EMIT groupUpdated(groupId);

    return true;
}

bool
PBDataFlowGraphModel::
restoreGroup(const PBNodeGroup &group)
{
    if (group.id() == InvalidGroupId)
        return false;

    if (group.nodes().empty())
        return false;

    PBNodeGroup restoredGroup = group;
    restoredGroup.clear();
    for (NodeId nodeId : group.nodes()) {
        if (!nodeExists(nodeId))
            continue;
        restoredGroup.addNode(nodeId);
    }

    if (restoredGroup.nodes().empty())
        return false;

    for (NodeId nodeId : restoredGroup.nodes()) {
        GroupId currentGroup = getPBNodeGroup(nodeId);
        if (currentGroup != InvalidGroupId && currentGroup != restoredGroup.id()) {
            std::set<NodeId> singleNode = {nodeId};
            removeNodesFromGroup(currentGroup, singleNode);
        }
    }

    bool alreadyExists = (mGroups.find(restoredGroup.id()) != mGroups.end());
    mGroups[restoredGroup.id()] = restoredGroup;
    if (restoredGroup.id() >= mNextGroupId)
        mNextGroupId = restoredGroup.id() + 1;

    if (alreadyExists) {
        Q_EMIT groupUpdated(restoredGroup.id());
    } else {
        Q_EMIT groupCreated(restoredGroup.id());
    }
    return true;
}

bool
PBDataFlowGraphModel::
deleteNode(NodeId const nodeId)
{
    if (mbPresetMode)
        return false;

    // If the node belongs to a group, remove it from that group first so
    // that group data structures never reference a node that no longer
    // exists in the underlying model. removeNodesFromGroup will dissolve
    // the group if it becomes empty, and will emit the appropriate signals.
    GroupId gid = getPBNodeGroup(nodeId);
    if (gid != InvalidGroupId) {
        std::set<NodeId> single = {nodeId};
        // ignore return value; proceed to delete node regardless
        removeNodesFromGroup(gid, single);
    }

    // Delegate to the base implementation to erase node and its connections
    return DataFlowGraphModel::deleteNode(nodeId);
}

void
PBDataFlowGraphModel::
triggerInitialPropagation()
{
    // Find all source-like nodes: nodes that have no incoming connection links
    std::vector<NodeId> sourceNodes;
    auto allIds = allNodeIds();
    for (NodeId nodeId : allIds) {
        auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(nodeId);
        if (!delegateModel) {
            continue;
        }

        bool hasIncomingConnections = false;
        unsigned int nInputPorts = delegateModel->nPorts(QtNodes::PortType::In);
        for (QtNodes::PortIndex port = 0; port < nInputPorts; ++port) {
            if (!connections(nodeId, QtNodes::PortType::In, port).empty()) {
                hasIncomingConnections = true;
                break;
            }
        }
        if (!hasIncomingConnections) {
            sourceNodes.push_back(nodeId);
        }
    }

    // Propagate output data for each source-like node
    for (NodeId nodeId : sourceNodes) {
        auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(nodeId);
        if (!delegateModel || !delegateModel->isEnable()) {
            continue;
        }

        unsigned int nOutputPorts = delegateModel->nPorts(QtNodes::PortType::Out);
        for (QtNodes::PortIndex port = 0; port < nOutputPorts; ++port) {
            Q_EMIT delegateModel->dataUpdated(port);
        }
    }
}

void
PBDataFlowGraphModel::
enablePresetMode(const QString& initialPresetName)
{
    if (mbPresetMode)
        return;
    
    mbPresetMode = true;
    msActivePreset = initialPresetName;
    mPresetOrder.clear();
    mPresetOrder << initialPresetName;
    mPresets.clear();
    
    snapshotActivePreset();
    
    Q_EMIT presetModeChanged(true);
    Q_EMIT presetsChanged();
}

void
PBDataFlowGraphModel::
disablePresetMode()
{
    if (!mbPresetMode)
        return;
    
    mbPresetMode = false;
    msActivePreset.clear();
    mPresetOrder.clear();
    mPresets.clear();
    
    Q_EMIT presetModeChanged(false);
    Q_EMIT presetsChanged();
}
void
PBDataFlowGraphModel::
snapshotActivePreset()
{
    if (!mbPresetMode || msActivePreset.isEmpty())
        return;
        
    QMap<QString, QJsonObject> presetData;
    for (NodeId nodeId : allNodeIds()) {
        auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(nodeId);
        if (delegateModel) {
            QJsonObject fullJson = delegateModel->save();
            QJsonObject customParams = fullJson["cParams"].toObject();
            if (!customParams.isEmpty()) {
                presetData[QString::number(nodeId)] = customParams;
            }
        }
    }
    mPresets[msActivePreset] = presetData;
}

void
PBDataFlowGraphModel::
applyPreset(const QString& presetName)
{
    if (!mbPresetMode || !mPresetOrder.contains(presetName))
        return;
        
    // Save current parameters first before switching
    snapshotActivePreset();
    
    msActivePreset = presetName;
    
    const auto& presetData = mPresets[presetName];
    for (NodeId nodeId : allNodeIds()) {
        auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(nodeId);
        if (delegateModel) {
            QString idStr = QString::number(nodeId);
            if (presetData.contains(idStr)) {
                QJsonObject fullJson = delegateModel->save();
                fullJson["cParams"] = presetData[idStr];
                delegateModel->load(fullJson);
                delegateModel->updateAllOutputPorts();
            }
        }
    }
    
    Q_EMIT presetsChanged();
}


void
PBDataFlowGraphModel::
addPreset(const QString& name)
{
    if (!mbPresetMode || name.isEmpty() || mPresetOrder.contains(name))
        return;
        
    snapshotActivePreset();
    
    // Copy the active preset's parameters to the new preset
    if (!msActivePreset.isEmpty() && mPresets.contains(msActivePreset)) {
        mPresets[name] = mPresets[msActivePreset];
    } else {
        mPresets[name] = QMap<QString, QJsonObject>();
    }
    
    mPresetOrder << name;
    msActivePreset = name;
    
    Q_EMIT presetsChanged();
}

void
PBDataFlowGraphModel::
renamePreset(const QString& oldName, const QString& newName)
{
    if (!mbPresetMode || oldName.isEmpty() || newName.isEmpty() || !mPresetOrder.contains(oldName) || mPresetOrder.contains(newName))
        return;
        
    snapshotActivePreset();
    
    // Rename key in map
    mPresets[newName] = mPresets.take(oldName);
    
    // Update list order
    int idx = mPresetOrder.indexOf(oldName);
    if (idx != -1) {
        mPresetOrder[idx] = newName;
    }
    
    if (msActivePreset == oldName) {
        msActivePreset = newName;
    }
    
    Q_EMIT presetsChanged();
}

void
PBDataFlowGraphModel::
deletePreset(const QString& name)
{
    if (!mbPresetMode || !mPresetOrder.contains(name))
        return;
        
    // Prevent deleting the last preset
    if (mPresetOrder.size() <= 1)
        return;
        
    mPresets.remove(name);
    mPresetOrder.removeAll(name);
    
    if (msActivePreset == name) {
        msActivePreset = mPresetOrder.first();
        // Load the new active preset's parameters
        const auto& presetData = mPresets[msActivePreset];
        for (NodeId nodeId : allNodeIds()) {
            auto *delegateModel = this->delegateModel<PBNodeDelegateModel>(nodeId);
            if (delegateModel) {
                QString idStr = QString::number(nodeId);
                if (presetData.contains(idStr)) {
                    QJsonObject fullJson = delegateModel->save();
                    fullJson["cParams"] = presetData[idStr];
                    delegateModel->load(fullJson);
                    delegateModel->updateAllOutputPorts();
                }
            }
        }
    }
    
    Q_EMIT presetsChanged();
}

