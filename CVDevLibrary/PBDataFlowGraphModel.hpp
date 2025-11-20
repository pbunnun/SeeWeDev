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
 * @file PBDataFlowGraphModel.hpp
 * @brief Custom dataflow graph model for CVDev visual programming environment.
 *
 * This file defines the PBDataFlowGraphModel class, which extends QtNodes::DataFlowGraphModel
 * with custom features for node management, serialization, and connection validation in CVDev.
 *
 * **Key Features:**
 * - **Custom Serialization:** Save/load graph with node sizes and positions
 * - **Node Creation:** Override addNode to connect widget signals
 * - **Type Conversion:** Support automatic type converters in connections
 * - **Node Styling:** Per-node style instead of global style
 * - **Port Data Management:** Custom port data handling
 *
 * **Migration from v2 to v3:**
 * - Replaces v2's PBFlowScene (which inherited from QGraphicsScene)
 * - Now inherits from DataFlowGraphModel (model-view separation)
 * - Uses NodeId instead of Node* pointers
 * - Custom features (undo/redo, snap-to-grid) implemented separately
 *
 * **Common Use Cases:**
 * - Main graph model for dataflow editor
 * - Graph serialization (save/load .flow files)
 * - Node lifecycle management (creation, deletion)
 * - Connection validation and type checking
 *
 * **Integration Pattern:**
 * @code
 * // Setup graph model with registry
 * auto registry = std::make_shared<NodeDelegateModelRegistry>();
 * load_plugins(registry, pluginLoaders);  // Load node types
 * 
 * auto* model = new PBDataFlowGraphModel(registry);
 * auto* scene = new PBDataFlowGraphicsScene(*model);
 * auto* view = new PBFlowGraphicsView(scene);
 * 
 * // Add nodes
 * NodeId id = model->addNode("ImageLoader");
 * 
 * // Save/load
 * model->save_to_file("myproject.flow");
 * model->load_from_file("myproject.flow");
 * @endcode
 *
 * **Graph File Format (JSON):**
 * @code
 * {
 *   "nodes": [
 *     {
 *       "id": 1,
 *       "type": "ImageLoader",
 *       "position": {"x": 100, "y": 50},
 *       "size": {"width": 200, "height": 150},
 *       "properties": {...}
 *     }
 *   ],
 *   "connections": [
 *     {
 *       "out": {"nodeId": 1, "portIndex": 0},
 *       "in": {"nodeId": 2, "portIndex": 0}
 *     }
 *   ]
 * }
 * @endcode
 *
 * **Type Conversion Support:**
 * @code
 * // Automatic converter insertion between incompatible types
 * IntegerData → [Converter] → DoubleData
 * CVImageData → [Converter] → QImageData
 * 
 * // connectionPossible() checks if conversion available
 * @endcode
 *
 * @see QtNodes::DataFlowGraphModel for base class functionality
 * @see PBDataFlowGraphicsScene for scene integration
 * @see NodeDelegateModelRegistry for node type registry
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "PBNodeGroup.hpp"
#include <map>

namespace QtNodes {
    class NodeDelegateModelRegistry;
}

/**
 * @class PBDataFlowGraphModel
 * @brief Custom dataflow graph model with enhanced serialization and node management.
 *
 * Extends QtNodes::DataFlowGraphModel to provide CVDev-specific features including
 * custom file I/O, node size persistence, per-node styling, and connection validation
 * with automatic type conversion support.
 *
 * **Core Functionality:**
 * - **Node Management:** Create, configure, and track nodes
 * - **Serialization:** Save/load graphs with full state preservation
 * - **Connection Validation:** Check type compatibility with converter support
 * - **Styling:** Per-node style overrides
 * - **Port Management:** Custom port data handling
 *
 * **Inheritance:**
 * @code
 * QObject
 *   └── AbstractGraphModel
 *         └── DataFlowGraphModel
 *               └── PBDataFlowGraphModel
 * @endcode
 *
 * **Migration Notes (NodeEditor v2 → v3):**
 * This replaces v2's PBFlowScene with key architectural changes:
 * - Model-view separation: No longer a QGraphicsScene
 * - NodeId-based: Uses NodeId instead of Node* pointers
 * - Separate concerns: Undo/redo moved to QUndoStack, snap-to-grid to scene
 *
 * **Typical Usage:**
 * @code
 * // Create model with registry
 * auto registry = std::make_shared<NodeDelegateModelRegistry>();
 * auto* model = new PBDataFlowGraphModel(registry, parent);
 * 
 * // Add nodes programmatically
 * NodeId loaderId = model->addNode("ImageLoader");
 * NodeId blurId = model->addNode("GaussianBlur");
 * 
 * // Create connection
 * ConnectionId connId = model->addConnection({loaderId, 0}, {blurId, 0});
 * 
 * // Save to file
 * if (model->save_to_file("myworkflow.flow")) {
 *     qDebug() << "Saved successfully";
 * }
 * 
 * // Load from file
 * if (model->load_from_file("myworkflow.flow")) {
 *     qDebug() << "Loaded successfully";
 * }
 * @endcode
 *
 * **Node Creation with Widget Signals:**
 * @code
 * // addNode() automatically connects widget resize signals
 * NodeId id = model->addNode("ImageDisplay");
 * // embeddedWidgetSizeUpdated signal → geometry recomputation
 * @endcode
 *
 * **Custom Serialization:**
 * @code
 * // saveNode() includes node size in JSON
 * {
 *   "id": 1,
 *   "type": "ImageLoader",
 *   "position": {"x": 100, "y": 50},
 *   "size": {"width": 200, "height": 150},  // Custom: preserved
 *   "properties": {...}
 * }
 * 
 * // loadNode() restores size from JSON
 * @endcode
 *
 * @see DataFlowGraphModel for base dataflow functionality
 * @see PBDataFlowGraphicsScene for graphics rendering
 * @see NodeDelegateModelRegistry for node type management
 */
/*
 * Node lifecycle note (short):
 * - Interactive creation: `addNode()` -> delegate created -> `nodeCreated(nodeId)` ->
 *   `PBDataFlowGraphModel::addNode()` connects widget signals and calls `late_constructor()`.
 * - Load from file: `DataFlowGraphModel::loadNode()` creates delegate and emits
 *   `nodeCreated(restoredNodeId)` then calls `delegate->load(...)`. After the base
 *   load returns, `PBDataFlowGraphModel::loadNode()` connects widget signals,
 *   calls `late_constructor()`, and restores embedded-widget size (with a
 *   single-shot re-apply to handle immediate resizes).
 *
 * Rationale: keep heavy initialization out of registry/menu time and centralize
 * ordering to avoid races between widget restoration and background activity.
 */

class CVDEVSHAREDLIB_EXPORT PBDataFlowGraphModel : public QtNodes::DataFlowGraphModel
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a custom dataflow graph model.
     *
     * Initializes the graph model with a node type registry for creating
     * and managing nodes in the dataflow graph.
     *
     * @param registry Shared pointer to NodeDelegateModelRegistry with registered node types
     * @param parent Optional parent QObject for Qt ownership (default: nullptr)
     *
     * **Example:**
     * @code
     * // Setup registry with plugins
     * auto registry = std::make_shared<NodeDelegateModelRegistry>();
     * add_type_converters(registry);
     * load_plugins(registry, loaders);
     * 
     * // Create model
     * auto* model = new PBDataFlowGraphModel(registry, mainWindow);
     * 
     * // Use with scene and view
     * auto* scene = new PBDataFlowGraphicsScene(*model);
     * auto* view = new PBFlowGraphicsView(scene);
     * @endcode
     */
    explicit PBDataFlowGraphModel(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry,
                                  QObject *parent = nullptr);

    /**
     * @brief Saves the graph model to JSON including groups
     * @return QJsonObject Complete graph state
     * 
     * Overrides base save() to include group data alongside nodes and connections.
     */
    QJsonObject save() const override;
    
    /**
     * @brief Restores a previously dissolved group
     * @param group Group to restore
     * @return bool True if the group was restored successfully
     */
    bool restoreGroup(const PBNodeGroup &group);
    
    /**
     * @brief Loads the graph model from JSON including groups
     * @param json Complete graph state
     * 
     * Overrides base load() to restore group data alongside nodes and connections.
     */
    void load(const QJsonObject& json) override;

    /**
     * @brief Creates a new node and connects widget signals.
     *
     * Overrides base addNode to automatically connect the node's embedded widget
     * size change signals to trigger geometry recalculation.
     *
     * @param nodeType QString identifier of the node type to create (e.g., "ImageLoader")
     * @return NodeId Unique identifier for the created node
     *
     * **Example:**
     * @code
     * // Create nodes
     * NodeId loaderId = model->addNode("ImageLoader");
     * NodeId blurId = model->addNode("GaussianBlur");
     * NodeId displayId = model->addNode("ImageDisplay");
     * 
     * // Position nodes
     * model->setNodeData(loaderId, NodeRole::Position, QPointF(100, 100));
     * model->setNodeData(blurId, NodeRole::Position, QPointF(400, 100));
     * model->setNodeData(displayId, NodeRole::Position, QPointF(700, 100));
     * @endcode
     *
     * **Widget Signal Connection:**
     * @code
     * // For nodes with embedded widgets:
     * // embeddedWidgetSizeUpdated signal → geometry->recomputeSize()
     * // Ensures node resizes when widget size changes
     * @endcode
     *
     * @note Returns InvalidNodeId if node type not found in registry
     * @see NodeDelegateModelRegistry::create() for node instantiation
     */
    QtNodes::NodeId addNode(QString const nodeType = QString()) override;
    
    /**
     * @brief Deletes a node and ensures group membership is updated.
     *
     * Overrides base deleteNode to remove the node from any group before
     * the node is erased from the underlying model. This keeps group
     * invariants consistent and prevents later attempts to access deleted
     * node entries.
     */
    bool deleteNode(QtNodes::NodeId const nodeId) override;
    /**
     * @brief Saves the graph to a JSON file.
     *
     * Serializes the entire graph (nodes, connections, properties) to a JSON
     * file for persistent storage. Includes custom node sizes and positions.
     *
     * @param sFilename Path to the output file (e.g., "myproject.flow")
     * @return bool True if save succeeded, false on error
     *
     * **Example:**
     * @code
     * // Save current graph
     * QString filename = QFileDialog::getSaveFileName(
     *     this, "Save Graph", "", "Flow Files (*.flow)"
     * );
     * 
     * if (!filename.isEmpty()) {
     *     if (model->save_to_file(filename)) {
     *         statusBar()->showMessage("Saved: " + filename);
     *     } else {
     *         QMessageBox::warning(this, "Error", "Failed to save file");
     *     }
     * }
     * @endcode
     *
     * **File Format:**
     * @code
     * {
     *   "nodes": [...],
     *   "connections": [...]
     * }
     * @endcode
     *
     * @note Creates parent directories if they don't exist
     * @note Overwrites existing files without warning
     * @see load_from_file() for loading saved graphs
     */
    bool save_to_file(QString const & sFilename) const;

    /**
     * @brief Loads a graph from a JSON file.
     *
     * Deserializes a graph from a JSON file, creating all nodes and connections.
     * Restores node positions, sizes, and property values.
     *
     * @param sFilename Path to the input file (e.g., "myproject.flow")
     * @return bool True if load succeeded, false on error
     *
     * **Example:**
     * @code
     * // Load existing graph
     * QString filename = QFileDialog::getOpenFileName(
     *     this, "Open Graph", "", "Flow Files (*.flow)"
     * );
     * 
     * if (!filename.isEmpty()) {
     *     // Clear current graph first
     *     model->clearScene();
     *     
     *     if (model->load_from_file(filename)) {
     *         statusBar()->showMessage("Loaded: " + filename);
     *         view->center_on(QPointF(0, 0));  // Center view on graph
     *     } else {
     *         QMessageBox::warning(this, "Error", "Failed to load file");
     *     }
     * }
     * @endcode
     *
     * **Loading Process:**
     * 1. Parse JSON file
     * 2. Create nodes with loadNode()
     * 3. Restore node positions and sizes
     * 4. Create connections
     * 5. Restore property values
     *
     * @note Fails gracefully if file doesn't exist or is invalid JSON
     * @note Does not clear existing graph - call clearScene() first if needed
     * @see save_to_file() for saving graphs
     */
    bool load_from_file(QString const & sFilename);

    /**
     * @brief Serializes a node to JSON with custom size information.
     *
     * Overrides base saveNode to include node size in the JSON output,
     * ensuring size is preserved across save/load cycles.
     *
     * @param nodeId Unique identifier of the node to serialize
     * @return QJsonObject JSON representation of the node
     *
     * **JSON Output:**
     * @code
     * {
     *   "id": 1,
     *   "type": "ImageLoader",
     *   "position": {"x": 100, "y": 50},
     *   "size": {"width": 200, "height": 150},  // Custom addition
     *   "properties": {
     *     "file_path": "/path/to/image.png"
     *   }
     * }
     * @endcode
     *
     * @note Called internally by save_to_file()
     * @see loadNode() for deserialization
     */
    QJsonObject saveNode(QtNodes::NodeId const nodeId) const override;

    /**
     * @brief Deserializes a node from JSON and restores size.
     *
     * Overrides base loadNode to restore custom node size from JSON,
     * ensuring nodes appear with correct dimensions after loading.
     *
     * @param nodeJson JSON object containing node data
     *
     * **Expected JSON:**
     * @code
     * {
     *   "id": 1,
     *   "type": "GaussianBlur",
     *   "position": {"x": 300, "y": 100},
     *   "size": {"width": 220, "height": 180},
     *   "properties": {
     *     "kernel_size": 5
     *   }
     * }
     * @endcode
     *
     * @note Called internally by load_from_file()
     * @see saveNode() for serialization
     */
    void loadNode(QJsonObject const &nodeJson) override;

    /**
     * @brief Returns node data with per-node style support.
     *
     * Overrides base nodeData to return the delegate model's custom style
     * instead of the global style, enabling per-node visual customization.
     *
     * @param nodeId Unique identifier of the node
     * @param role NodeRole indicating requested data type
     * @return QVariant Requested node data (style, position, etc.)
     *
     * **Per-Node Styling:**
     * @code
     * // Each node can have custom colors/fonts
     * class CustomNode : public PBNodeDelegateModel {
     * public:
     *     NodeStyle nodeStyle() const override {
     *         NodeStyle style;
     *         style.GradientColor0 = QColor(100, 150, 200);
     *         style.GradientColor1 = QColor(80, 120, 180);
     *         return style;
     *     }
     * };
     * @endcode
     *
     * @see NodeRole for available data types
     * @see NodeDelegateModel::nodeStyle() for custom styling
     */
    QVariant nodeData(QtNodes::NodeId nodeId, QtNodes::NodeRole role) const override;

    /**
     * @brief Sets port data with custom handling.
     *
     * Overrides base setPortData to provide custom port data management,
     * enabling specialized behavior for port value updates.
     *
     * @param nodeId Unique identifier of the node
     * @param portType Input or output port type
     * @param portIndex Zero-based port index
     * @param value New value for the port
     * @param role Port data role (default: Data)
     * @return bool True if data was set successfully
     *
     * **Example:**
     * @code
     * // Manually set port data
     * auto imageData = std::make_shared<CVImageData>(image);
     * model->setPortData(
     *     nodeId, 
     *     PortType::Out, 
     *     PortIndex(0),
     *     QVariant::fromValue(imageData)
     * );
     * @endcode
     *
     * @note Typically called internally during node computation
     */
    bool setPortData(QtNodes::NodeId nodeId,
                     QtNodes::PortType portType,
                     QtNodes::PortIndex portIndex,
                     QVariant const &value,
                     QtNodes::PortRole role = QtNodes::PortRole::Data) override;

    /**
     * @brief Checks if a connection is possible with type conversion support.
     *
     * Overrides base connectionPossible to enable automatic type converter
     * insertion when connecting incompatible port types.
     *
     * @param connectionId Connection identifier specifying source and sink
     * @return bool True if connection possible (direct or via converter)
     *
     * **Type Conversion Examples:**
     * @code
     * // Direct connection (types match)
     * ImageLoader[CVImageData] → GaussianBlur[CVImageData] ✓
     * 
     * // Converter inserted automatically
     * ImageLoader[CVImageData] → [Converter] → QImageDisplay[QImageData] ✓
     * 
     * // No converter available
     * ImageLoader[CVImageData] → TextDisplay[StringData] ✗
     * @endcode
     *
     * **Converter Registration:**
     * @code
     * // In add_type_converters()
     * registry->registerTypeConverter(
     *     "CVImageData", "QImageData",
     *     []() { return std::make_unique<CVImageToQImage>(); }
     * );
     * @endcode
     *
     * @note Checks registry for available type converters
     * @see add_type_converters() for converter registration
     * @see NodeDelegateModelRegistry::registerTypeConverter()
     */
    bool connectionPossible(QtNodes::ConnectionId const connectionId) const override;

    // ========== Node Grouping API ==========
    
    /**
     * @brief Creates a new group from selected nodes
     * @param name Group display name
     * @param nodeIds Set of node IDs to include in group
     * @return GroupId Unique identifier for the created group
     * 
     * **Example:**
     * @code
     * std::set<NodeId> nodes = {1, 2, 3};
     * GroupId gid = model->createGroup("Preprocessing", nodes);
     * model->setGroupColor(gid, QColor(150, 200, 150, 80));
     * @endcode
     */
    GroupId createGroup(const QString& name, const std::set<NodeId>& nodeIds);
    
    /**
     * @brief Dissolves a group (removes grouping but keeps nodes)
     * @param groupId Group identifier to dissolve
     * @return bool True if group was dissolved
     */
    bool dissolveGroup(GroupId groupId);
    
    /**
     * @brief Adds nodes to an existing group
     * @param groupId Group to modify
     * @param nodeIds Nodes to add
     * @return bool True if any nodes were added
     */
    bool addNodesToGroup(GroupId groupId, const std::set<NodeId>& nodeIds);
    
    /**
     * @brief Removes nodes from a group
     * @param groupId Group to modify
     * @param nodeIds Nodes to remove
     * @return bool True if any nodes were removed
     */
    bool removeNodesFromGroup(GroupId groupId, const std::set<NodeId>& nodeIds);
    
    /**
     * @brief Gets the group containing a node
     * @param nodeId Node to query
     * @return GroupId Group ID or InvalidGroupId if ungrouped
     */
    GroupId getPBNodeGroup(NodeId nodeId) const;
    
    /**
     * @brief Gets all groups in the model
     * @return const std::map<GroupId, PBNodeGroup>& Map of groups
     */
    const std::map<GroupId, PBNodeGroup>& groups() const { return mGroups; }
    
    /**
     * @brief Gets a specific group
     * @param groupId Group identifier
     * @return const PBNodeGroup* Pointer to group or nullptr if not found
     */
    const PBNodeGroup* getGroup(GroupId groupId) const;
    
    /**
     * @brief Sets the color of a group
     * @param groupId Group to modify
     * @param color New color for the group
     * @return bool True if group exists and was updated
     */
    bool setGroupColor(GroupId groupId, const QColor& color);
    
    /**
     * @brief Renames a group
     * @param groupId Group to modify
     * @param name New name for the group
     * @return bool True if group exists and was updated
     */
    bool setGroupName(GroupId groupId, const QString& name);

    /**
     * @brief Sets the minimized state of a group
     * @param groupId ID of the group to modify
     * @param minimized True to minimize, false to expand
     * @return bool True if group exists and was updated
     */
    bool setGroupMinimized(GroupId groupId, bool minimized);

    /**
     * @brief Toggles the minimized state of a group
     * @param groupId ID of the group to toggle
     * @return bool True if group exists and was toggled
     */
    bool toggleGroupMinimized(GroupId groupId);

    /**
     * @brief Sets the locked state of a group
     * @param groupId ID of the group to modify
     * @param locked True to lock position, false to unlock
     * @return bool True if group exists and was updated
     */
    bool setGroupLocked(GroupId groupId, bool locked);

Q_SIGNALS:
    /**
     * @brief Emitted when a group is created
     * @param groupId ID of the created group
     */
    void groupCreated(GroupId groupId);
    
    /**
     * @brief Emitted when a group is dissolved
     * @param groupId ID of the dissolved group
     */
    void groupDissolved(GroupId groupId);
    
    /**
     * @brief Emitted when a group's properties change
     * @param groupId ID of the modified group
     */
    void groupUpdated(GroupId groupId);

private:
    // Track error messages for nodes that couldn't be loaded
    QStringList mLoadErrors;
    
    // Node grouping
    std::map<GroupId, PBNodeGroup> mGroups;  ///< All groups in the model
    GroupId mNextGroupId{1};                ///< Next available group ID

};
