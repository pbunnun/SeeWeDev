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
 * @file PBNodeGeometry.hpp
 * @brief Custom node geometry calculator for CVDev dataflow graphs.
 *
 * This file defines the PBNodeGeometry class, which extends QtNodes'
 * DefaultHorizontalNodeGeometry to provide custom node size and widget
 * positioning calculations for CVDev's node-based interface.
 *
 * **Key Features:**
 * - **Size Calculation:** Custom node dimensions based on content
 * - **Widget Positioning:** Embedded QWidget placement within nodes
 * - **Port Layout:** Input/output port positioning
 * - **Checkbox Support:** Space for enable/lock/minimize controls
 *
 * **Geometry Components:**
 * @code
 * +--------------------------------+
 * | [✓][🔒][−] Node Caption      | <- Header with checkboxes
 * +--------------------------------+
 * | ○ Input1        Output1 ○     | <- Port labels and connection points
 * | ○ Input2        Output2 ○     |
 * +--------------------------------+
 * |   [Embedded Widget Area]       | <- Widget position calculated by widgetPosition()
 * +--------------------------------+
 * @endcode
 *
 * **Common Use Cases:**
 * - Calculate node bounding box for rendering
 * - Position embedded widgets (sliders, previews, displays)
 * - Layout ports and connection points
 * - Account for custom checkboxes in header
 *
 * **Integration Pattern:**
 * @code
 * // In scene setup
 * auto graphModel = std::make_shared<DataFlowGraphModel>();
 * auto geometry = std::make_unique<PBNodeGeometry>(*graphModel);
 * auto* scene = new DataFlowGraphicsScene(
 *     *graphModel,
 *     std::move(geometry)  // Custom geometry
 * );
 * @endcode
 *
 * **Size Calculation Flow:**
 * @code
 * 1. Measure caption text width
 * 2. Measure port labels (longest input + longest output)
 * 3. Account for checkbox width in header
 * 4. Add padding and margins
 * 5. Calculate embedded widget dimensions
 * 6. Combine into total bounding box
 * @endcode
 *
 * **Widget Embedding:**
 * @code
 * // Node with embedded slider
 * class SliderNode : public PBNodeDelegateModel {
 * public:
 *     QWidget* embeddedWidget() override {
 *         if (!_slider) {
 *             _slider = new QSlider(Qt::Horizontal);
 *             _slider->setRange(0, 100);
 *         }
 *         return _slider;
 *     }
 * };
 * 
 * // PBNodeGeometry positions slider within node bounds
 * QPointF pos = geometry->widgetPosition(nodeId);
 * embeddedWidget->move(pos.toPoint());
 * @endcode
 *
 * @see QtNodes::DefaultHorizontalNodeGeometry for base implementation
 * @see QtNodes::AbstractNodeGeometry for geometry interface
 * @see PBNodePainter for node rendering
 */

#pragma once

#include <QtNodes/internal/DefaultHorizontalNodeGeometry.hpp>

namespace QtNodes {
class AbstractGraphModel;
}

/**
 * @class PBNodeGeometry
 * @brief Custom geometry calculator for CVDev node layout and sizing.
 *
 * Extends DefaultHorizontalNodeGeometry to provide customized node dimensions
 * and embedded widget positioning for the CVDev visual programming environment.
 * Accounts for custom checkboxes, port layouts, and embedded QWidgets.
 *
 * **Core Functionality:**
 * - **Size Calculation:** Compute node bounding box from content
 * - **Widget Positioning:** Calculate embedded widget coordinates
 * - **Port Layout:** Position input/output connection points
 * - **Custom Elements:** Account for enable/lock/minimize checkboxes
 *
 * **Inheritance:**
 * @code
 * QtNodes::AbstractNodeGeometry (interface)
 *   └── QtNodes::DefaultHorizontalNodeGeometry (base implementation)
 *         └── PBNodeGeometry (CVDev customization)
 * @endcode
 *
 * **Typical Usage:**
 * @code
 * // Create custom geometry for scene
 * auto model = std::make_shared<DataFlowGraphModel>();
 * auto geometry = std::make_unique<PBNodeGeometry>(*model);
 * 
 * // Use in scene
 * auto* scene = new DataFlowGraphicsScene(
 *     *model,
 *     std::move(geometry)
 * );
 * 
 * // Geometry automatically calculates sizes and positions
 * QSizeF nodeSize = geometry->size(nodeId);
 * QPointF widgetPos = geometry->widgetPosition(nodeId);
 * @endcode
 *
 * **Node Layout Structure:**
 * @code
 * +----------------------------------+
 * | [✓][🔒][−] Caption             | <- Header (30px height)
 * +----------------------------------+
 * | ○ Input1          Output1 ○    | <- Port row 1 (20px)
 * | ○ Input2          Output2 ○    | <- Port row 2 (20px)
 * +----------------------------------+
 * |                                  |
 * |    Embedded Widget Area          | <- Widget (variable height)
 * |                                  |
 * +----------------------------------+
 * 
 * Width = max(
 *   caption_width + checkbox_width,
 *   input_labels_width + output_labels_width + port_spacing,
 *   widget_width
 * ) + padding
 * 
 * Height = header_height + 
 *          (num_ports * port_height) + 
 *          widget_height + 
 *          padding
 * @endcode
 *
 * **Size Calculation Components:**
 * - Caption width: QFontMetrics::width(caption)
 * - Checkbox width: 3 checkboxes × 20px + spacing
 * - Port label width: Longest input label + longest output label
 * - Port spacing: Fixed separation between input/output sides
 * - Widget dimensions: From QWidget::sizeHint() or minimum
 * - Padding: Border margins, inter-element spacing
 *
 * **Widget Positioning Example:**
 * @code
 * // Node with slider widget
 * class ThresholdNode : public PBNodeDelegateModel {
 * public:
 *     QWidget* embeddedWidget() override {
 *         auto* slider = new QSlider(Qt::Horizontal);
 *         slider->setFixedWidth(200);
 *         return slider;
 *     }
 * };
 * 
 * // Geometry positions slider below ports
 * // Position calculated as:
 * // x = horizontal_center - (widget_width / 2)
 * // y = header_height + (num_ports * port_height) + vertical_margin
 * @endcode
 *
 * @see QtNodes::DefaultHorizontalNodeGeometry for base calculations
 * @see QtNodes::AbstractNodeGeometry for geometry interface
 * @see PBNodePainter for rendering using geometry data
 */
class PBNodeGeometry : public QtNodes::DefaultHorizontalNodeGeometry
{
public:
    /**
     * @brief Constructs custom geometry calculator with graph model reference.
     *
     * Initializes the geometry system with access to the graph model for
     * querying node properties, port configurations, and widget information.
     *
     * @param graphModel Reference to the AbstractGraphModel containing node data
     *
     * **Example:**
     * @code
     * auto model = std::make_shared<DataFlowGraphModel>();
     * 
     * // Create custom geometry
     * auto geometry = std::make_unique<PBNodeGeometry>(*model);
     * 
     * // Use in scene construction
     * auto* scene = new DataFlowGraphicsScene(
     *     *model,
     *     std::move(geometry),
     *     nullptr  // Use default node painter
     * );
     * @endcode
     *
     * @note Stores reference to graphModel - model must outlive geometry object
     */
    PBNodeGeometry(QtNodes::AbstractGraphModel &graphModel);
    
    /**
     * @brief Calculates the position of a port connection point.
     *
     * Determines where connection lines attach to the node. Ports are centered
     * vertically on the left (input) or right (output) side of the node.
     *
     * @param nodeId Unique identifier of the node
     * @param portType Port type (PortType::In or PortType::Out)
     * @param index Port index (0-based)
     * @return QPointF Position in node's local coordinates
     */
    QPointF portPosition(QtNodes::NodeId const nodeId,
                        QtNodes::PortType const portType,
                        QtNodes::PortIndex const index) const override;

    /**
     * @brief Recalculates and caches node size based on current content.
     *
     * Called when node content changes (caption, ports, widget) to update
     * the cached size. Considers all size-affecting components:
     * - Caption text width
     * - Number and labels of input/output ports
     * - Embedded widget dimensions
     * - Custom checkbox area in header
     * - Padding and margins
     *
     * @param nodeId Unique identifier of the node to recalculate
     *
     * **Recalculation Triggers:**
     * @code
     * // Size recalculated when:
     * // 1. Node created
     * geometry->recomputeSize(newNodeId);
     * 
     * // 2. Caption changed
     * node->setCaption("New Name");
     * geometry->recomputeSize(nodeId);
     * 
     * // 3. Ports added/removed
     * model->addPort(nodeId, PortType::In, PortIndex(2));
     * geometry->recomputeSize(nodeId);
     * 
     * // 4. Widget size hint changed
     * embeddedWidget->updateGeometry();
     * geometry->recomputeSize(nodeId);
     * @endcode
     *
     * **Size Components:**
     * @code
     * // Width calculation
     * int captionWidth = QFontMetrics(font).width(caption);
     * int checkboxWidth = 3 * 20 + 2 * 5;  // 3 checkboxes + spacing
     * int headerWidth = captionWidth + checkboxWidth;
     * 
     * int maxInputLabelWidth = getMaxPortLabelWidth(PortType::In);
     * int maxOutputLabelWidth = getMaxPortLabelWidth(PortType::Out);
     * int portWidth = maxInputLabelWidth + maxOutputLabelWidth + portSpacing;
     * 
     * QSize widgetSize = embeddedWidget ? embeddedWidget->sizeHint() : QSize(0, 0);
     * 
     * int nodeWidth = std::max({headerWidth, portWidth, widgetSize.width()}) + padding;
     * 
     * // Height calculation
     * int headerHeight = 30;
     * int portHeight = std::max(numInputs, numOutputs) * 20;
     * int widgetHeight = widgetSize.height();
     * 
     * int nodeHeight = headerHeight + portHeight + widgetHeight + padding;
     * @endcode
     *
     * @note Results are cached - call this when content changes to update cache
     * @note Override this to customize size calculation logic
     *
     * @see size() to retrieve the cached size
     * @see widgetPosition() for widget placement after resize
     */
    void recomputeSize(QtNodes::NodeId const nodeId) const override;

    /**
     * @brief Calculates the position for the embedded widget within the node.
     *
     * Determines the coordinates where an embedded QWidget should be positioned
     * within the node's local coordinate system, typically below the port rows.
     *
     * @param nodeId Unique identifier of the node
     * @return QPointF Position in node's local coordinates (top-left corner)
     *
     * **Position Calculation:**
     * @code
     * // Typical widget positioning
     * // X: Centered horizontally
     * float nodeWidth = size(nodeId).width();
     * float widgetWidth = embeddedWidget->width();
     * float x = (nodeWidth - widgetWidth) / 2.0f;
     * 
     * // Y: Below header and ports
     * float headerHeight = 30;
     * float portSectionHeight = numPorts * 20;
     * float verticalMargin = 5;
     * float y = headerHeight + portSectionHeight + verticalMargin;
     * 
     * return QPointF(x, y);
     * @endcode
     *
     * **Usage Example:**
     * @code
     * // In NodeGraphicsObject::updateEmbeddedWidget()
     * QWidget* widget = delegateModel->embeddedWidget();
     * if (widget) {
     *     QPointF pos = geometry->widgetPosition(nodeId);
     *     widget->move(pos.toPoint());
     *     widget->show();
     * }
     * @endcode
     *
     * **Widget Types and Positioning:**
     * @code
     * // Image preview widget (large)
     * QLabel* preview = new QLabel();
     * preview->setPixmap(image);
     * preview->setFixedSize(320, 240);
     * // Positioned: Centered, below ports
     * 
     * // Slider widget (horizontal)
     * QSlider* slider = new QSlider(Qt::Horizontal);
     * slider->setFixedWidth(200);
     * // Positioned: Centered, below ports
     * 
     * // Button widget
     * QPushButton* button = new QPushButton("Execute");
     * // Positioned: Centered, below ports
     * @endcode
     *
     * **Coordinate System:**
     * @code
     * // Node local coordinates (origin at top-left)
     * (0,0) +------------------------+
     *       |  Header                |
     *       +------------------------+
     *       |  Ports                 |
     *       +------------------------+
     *       |                        |
     *       | (widgetX, widgetY) +---+--- Widget
     *       |                    |   |
     *       +--------------------+---+
     * @endcode
     *
     * @note Returns QPointF(0, 0) if node has no embedded widget
     * @note Widget must set its own size (via setFixedSize or sizeHint)
     *
     * @see recomputeSize() for overall node size calculation
     * @see PBNodeDelegateModel::embeddedWidget() for widget access
     */
    QPointF widgetPosition(QtNodes::NodeId const nodeId) const override;
};