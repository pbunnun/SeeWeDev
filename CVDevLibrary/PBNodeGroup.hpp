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

/**
 * @file PBNodeGroup.hpp
 * @brief Data structure for node grouping in visual graph editor
 * 
 * Defines PBNodeGroup class for organizing nodes into named, colored groups
 * with serialization support for save/load functionality.
 */

#pragma once

#include "CVDevLibrary.hpp"
#include <QtNodes/Definitions>
#include <QString>
#include <QColor>
#include <QRectF>
#include <QJsonObject>
#include <set>

using QtNodes::NodeId;

// Forward declaration
namespace QtNodes {
    class DataFlowGraphModel;
}

/**
 * @typedef GroupId
 * @brief Unique identifier for node groups
 * 
 * Uses unsigned int to match NodeId convention in QtNodes
 */
using GroupId = unsigned int;

/**
 * @brief Invalid group identifier constant
 */
static constexpr GroupId InvalidGroupId = static_cast<GroupId>(-1);

/**
 * @class PBNodeGroup
 * @brief Container for grouped nodes with visual properties
 * 
 * PBNodeGroup represents a collection of nodes that are logically grouped together.
 * Groups have a name, color, and maintain a set of member node IDs. The bounding
 * rectangle is calculated dynamically from member node positions.
 * 
 * **Features:**
 * - Named groups for organization
 * - Custom color per group
 * - Membership management (add/remove nodes)
 * - JSON serialization for persistence
 * - Dynamic bounding box calculation
 * 
 * **Usage Example:**
 * @code
 * PBNodeGroup group;
 * group.setId(1);
 * group.setName("Image Processing");
 * group.setColor(QColor(100, 150, 200, 100));  // Semi-transparent blue
 * group.addNode(nodeId1);
 * group.addNode(nodeId2);
 * 
 * // Save to JSON
 * QJsonObject json = group.save();
 * 
 * // Load from JSON
 * PBNodeGroup loaded;
 * loaded.load(json);
 * @endcode
 */
class CVDEVSHAREDLIB_EXPORT PBNodeGroup
{
public:
    /**
     * @brief Default constructor
     */
    PBNodeGroup() = default;

    /**
     * @brief Gets the group identifier
     * @return GroupId Unique identifier for this group
     */
    GroupId id() const { return mId; }

    /**
     * @brief Sets the group identifier
     * @param id Unique identifier to assign
     */
    void setId(GroupId id) { mId = id; }

    /**
     * @brief Gets the group name
     * @return QString Display name for this group
     */
    QString name() const { return mName; }

    /**
     * @brief Sets the group name
     * @param name Display name to assign
     */
    void setName(const QString& name) { mName = name; }

    /**
     * @brief Gets the group color
     * @return QColor Color used for group background
     */
    QColor color() const { return mColor; }

    /**
     * @brief Sets the group color
     * @param color Color to use for group background
     */
    void setColor(const QColor& color) { mColor = color; }

    /**
     * @brief Gets the set of member node IDs
     * @return const std::set<NodeId>& Reference to member nodes
     */
    const std::set<NodeId>& nodes() const { return mNodes; }

    /**
     * @brief Gets the minimized state
     * @return bool True if group is minimized
     */
    bool isMinimized() const { return mMinimized; }

    /**
     * @brief Sets the minimized state
     * @param minimized True to minimize, false to expand
     */
    void setMinimized(bool minimized) { mMinimized = minimized; }

    /**
     * @brief Adds a node to the group
     * @param nodeId ID of the node to add
     * @return bool True if node was added, false if already present
     */
    bool addNode(NodeId nodeId);

    /**
     * @brief Removes a node from the group
     * @param nodeId ID of the node to remove
     * @return bool True if node was removed, false if not present
     */
    bool removeNode(NodeId nodeId);

    /**
     * @brief Checks if a node is in the group
     * @param nodeId ID of the node to check
     * @return bool True if node is a member
     */
    bool contains(NodeId nodeId) const;

    /**
     * @brief Checks if the group is empty
     * @return bool True if no nodes in group
     */
    bool isEmpty() const { return mNodes.empty(); }

    /**
     * @brief Gets the number of nodes in the group
     * @return size_t Number of member nodes
     */
    size_t size() const { return mNodes.size(); }

    /**
     * @brief Clears all nodes from the group
     */
    void clear() { mNodes.clear(); }

    /**
     * @brief Calculates total number of input ports from all grouped nodes
     * @param graphModel Reference to the graph model for accessing node port counts
     * @return unsigned int Total number of input ports exposed by the group
     */
    unsigned int getTotalInputPorts(QtNodes::DataFlowGraphModel& graphModel) const;

    /**
     * @brief Calculates total number of output ports from all grouped nodes
     * @param graphModel Reference to the graph model for accessing node port counts
     * @return unsigned int Total number of output ports exposed by the group
     */
    unsigned int getTotalOutputPorts(QtNodes::DataFlowGraphModel& graphModel) const;

    /**
     * @brief Gets the port mapping for input ports
     * 
     * Maps group input port indices to their source node and port.
     * Format: group_port_index -> (node_id, node_port_index)
     * 
     * @param graphModel Reference to the graph model
     * @return std::map<QtNodes::PortIndex, std::pair<NodeId, QtNodes::PortIndex>>
     */
    std::map<QtNodes::PortIndex, std::pair<NodeId, QtNodes::PortIndex>>
    getInputPortMapping(QtNodes::DataFlowGraphModel& graphModel) const;

    /**
     * @brief Gets the port mapping for output ports
     * 
     * Maps group output port indices to their source node and port.
     * Format: group_port_index -> (node_id, node_port_index)
     * 
     * @param graphModel Reference to the graph model
     * @return std::map<QtNodes::PortIndex, std::pair<NodeId, QtNodes::PortIndex>>
     */
    std::map<QtNodes::PortIndex, std::pair<NodeId, QtNodes::PortIndex>>
    getOutputPortMapping(QtNodes::DataFlowGraphModel& graphModel) const;

    /**
     * @brief Serializes the group to JSON
     * @return QJsonObject JSON representation
     * 
     * **JSON Format:**
     * @code
     * {
     *   "id": 1,
     *   "name": "Image Processing",
     *   "color": "#6496C8",
     *   "nodes": [1, 2, 3, 5]
     * }
     * @endcode
     */
    QJsonObject save() const;

    /**
     * @brief Deserializes the group from JSON
     * @param json JSON object to load from
     * 
     * Restores group ID, name, color, and member nodes from JSON.
     */
    void load(const QJsonObject& json);

private:
    GroupId mId{InvalidGroupId};        ///< Unique identifier
    QString mName{"Group"};              ///< Display name
    QColor mColor{100, 150, 200, 80};   ///< Background color (default: semi-transparent blue)
    std::set<NodeId> mNodes;             ///< Member node IDs
    bool mMinimized{false};              ///< Minimized state
};
