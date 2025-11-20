//Copyright © 2025, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "PBNodeGroup.hpp"
#include <QJsonArray>
#include <QtNodes/DataFlowGraphModel>

bool PBNodeGroup::addNode(NodeId nodeId)
{
    auto result = mNodes.insert(nodeId);
    return result.second;  // True if inserted, false if already present
}

bool PBNodeGroup::removeNode(NodeId nodeId)
{
    return mNodes.erase(nodeId) > 0;
}

bool PBNodeGroup::contains(NodeId nodeId) const
{
    return mNodes.find(nodeId) != mNodes.end();
}

QJsonObject PBNodeGroup::save() const
{
    QJsonObject json;
    
    json["id"] = static_cast<qint64>(mId);
    json["name"] = mName;
    json["color"] = mColor.name(QColor::HexArgb);  // Save with alpha channel
    json["minimized"] = mMinimized;
    json["locked"] = mLocked;
    
    // Save node IDs as array
    QJsonArray nodesArray;
    for (NodeId nodeId : mNodes) {
        nodesArray.append(static_cast<qint64>(nodeId));
    }
    json["nodes"] = nodesArray;
    
    return json;
}

void PBNodeGroup::load(const QJsonObject& json)
{
    // Load ID
    if (json.contains("id") && json["id"].isDouble()) {
        mId = static_cast<GroupId>(json["id"].toInt());
    }
    
    // Load name
    if (json.contains("name") && json["name"].isString()) {
        mName = json["name"].toString();
    }
    
    // Load color
    if (json.contains("color") && json["color"].isString()) {
        mColor = QColor(json["color"].toString());
        if (!mColor.isValid()) {
            mColor = QColor(100, 150, 200, 80);  // Default if invalid
        }
    }
    
    // Load minimized state
    if (json.contains("minimized") && json["minimized"].isBool()) {
        mMinimized = json["minimized"].toBool();
    }
    
    // Load locked state
    if (json.contains("locked") && json["locked"].isBool()) {
        mLocked = json["locked"].toBool();
    }
    
    // Load node IDs
    mNodes.clear();
    if (json.contains("nodes") && json["nodes"].isArray()) {
        QJsonArray nodesArray = json["nodes"].toArray();
        for (const QJsonValue& value : nodesArray) {
            if (value.isDouble()) {
                NodeId nodeId = static_cast<NodeId>(value.toInt());
                mNodes.insert(nodeId);
            }
        }
    }
}

unsigned int PBNodeGroup::getTotalInputPorts(QtNodes::DataFlowGraphModel& graphModel) const
{
    unsigned int totalPorts = 0;
    
    for (NodeId nodeId : mNodes) {
        unsigned int inPortCount = graphModel.nodeData(nodeId, QtNodes::NodeRole::InPortCount).toUInt();
        totalPorts += inPortCount;
    }
    
    return totalPorts;
}

unsigned int PBNodeGroup::getTotalOutputPorts(QtNodes::DataFlowGraphModel& graphModel) const
{
    unsigned int totalPorts = 0;
    
    for (NodeId nodeId : mNodes) {
        unsigned int outPortCount = graphModel.nodeData(nodeId, QtNodes::NodeRole::OutPortCount).toUInt();
        totalPorts += outPortCount;
    }
    
    return totalPorts;
}

std::map<QtNodes::PortIndex, std::pair<NodeId, QtNodes::PortIndex>>
PBNodeGroup::getInputPortMapping(QtNodes::DataFlowGraphModel& graphModel) const
{
    std::map<QtNodes::PortIndex, std::pair<NodeId, QtNodes::PortIndex>> mapping;
    QtNodes::PortIndex groupPortIndex = 0;
    
    for (NodeId nodeId : mNodes) {
        unsigned int inPortCount = graphModel.nodeData(nodeId, QtNodes::NodeRole::InPortCount).toUInt();
        
        for (QtNodes::PortIndex nodePortIndex = 0; nodePortIndex < inPortCount; ++nodePortIndex) {
            mapping[groupPortIndex] = {nodeId, nodePortIndex};
            ++groupPortIndex;
        }
    }
    
    return mapping;
}

std::map<QtNodes::PortIndex, std::pair<NodeId, QtNodes::PortIndex>>
PBNodeGroup::getOutputPortMapping(QtNodes::DataFlowGraphModel& graphModel) const
{
    std::map<QtNodes::PortIndex, std::pair<NodeId, QtNodes::PortIndex>> mapping;
    QtNodes::PortIndex groupPortIndex = 0;
    
    for (NodeId nodeId : mNodes) {
        unsigned int outPortCount = graphModel.nodeData(nodeId, QtNodes::NodeRole::OutPortCount).toUInt();
        
        for (QtNodes::PortIndex nodePortIndex = 0; nodePortIndex < outPortCount; ++nodePortIndex) {
            mapping[groupPortIndex] = {nodeId, nodePortIndex};
            ++groupPortIndex;
        }
    }
    
    return mapping;
}
