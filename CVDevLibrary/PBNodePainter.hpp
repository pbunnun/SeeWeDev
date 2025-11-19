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
 * @file PBNodePainter.hpp
 * @brief Custom node painter with enable/disable, lock, and minimize checkboxes.
 *
 * This file defines the PBNodePainter class, which extends QtNodes::AbstractNodePainter
 * to provide custom rendering of nodes with additional UI controls: enable/disable checkbox,
 * position lock checkbox, and minimize checkbox.
 *
 * **Key Features:**
 * - **Enable/Disable Checkbox:** Toggle node processing on/off
 * - **Lock Checkbox:** Prevent node movement in the graph
 * - **Minimize Checkbox:** Collapse node to compact view
 * - **Standard Node Elements:** Caption, ports, connections, validation
 * - **Custom Styling:** Consistent with QtNodes theming
 *
 * **Painting Order:**
 * 1. Node background rectangle
 * 2. Connection points (ports)
 * 3. Filled connection indicators
 * 4. Node caption/title
 * 5. Entry/exit port labels
 * 6. Resize handle (if resizable)
 * 7. Validation icon (if errors/warnings)
 * 8. Custom checkboxes (enable, lock, minimize)
 *
 * **Common Use Cases:**
 * - Custom node rendering in dataflow graphs
 * - Interactive node state controls
 * - Visual feedback for node state (enabled/disabled, locked, minimized)
 * - Consistent UI styling across all nodes
 *
 * **Integration Pattern:**
 * @code
 * // Register custom painter with QtNodes
 * auto* scene = new QtNodes::DataFlowGraphicsScene(model);
 * scene->setNodePainter(std::make_unique<PBNodePainter>());
 * 
 * // Nodes automatically rendered with custom checkboxes
 * @endcode
 *
 * **Checkbox Layout:**
 * @code
 * +-----------------------------+
 * | [✓] Enable  [🔒] Lock      |  <- Checkboxes in header
 * | [−] Minimize                |
 * +-----------------------------+
 * |   Node Caption              |
 * | ○ Input1    Output1 ○       |  <- Port labels
 * | ○ Input2    Output2 ○       |
 * +-----------------------------+
 * |          [⚠]                |  <- Validation icon
 * +-----------------------------+
 * @endcode
 *
 * **State Control:**
 * - **Enable Checkbox:** Controls whether node processes data (grayed out if disabled)
 * - **Lock Checkbox:** Prevents accidental node repositioning
 * - **Minimize Checkbox:** Collapses node to show only title and checkboxes
 *
 * @see QtNodes::AbstractNodePainter for base painter interface
 * @see QtNodes::NodeGraphicsObject for node graphics object
 * @see QtNodes::DefaultNodePainter for default rendering
 */

#pragma once

#include "CVDevLibrary.hpp"
#include <QtNodes/internal/AbstractNodePainter.hpp>

namespace QtNodes {
    class NodeGraphicsObject;
}

/**
 * @class PBNodePainter
 * @brief Custom node painter with enable/disable, lock, and minimize controls.
 *
 * Extends QtNodes::AbstractNodePainter to provide custom rendering of dataflow
 * graph nodes with additional interactive checkboxes for node state management.
 *
 * **Core Functionality:**
 * - **Standard Rendering:** Node rect, ports, labels, validation icons
 * - **Enable/Disable:** Checkbox to toggle node processing
 * - **Position Lock:** Checkbox to prevent node movement
 * - **Minimize:** Checkbox to collapse node to compact view
 * - **Consistent Styling:** Matches QtNodes theme system
 *
 * **Inheritance:**
 * @code
 * QtNodes::AbstractNodePainter
 *   └── PBNodePainter
 * @endcode
 *
 * **Typical Usage:**
 * @code
 * // Register painter with scene
 * auto painter = std::make_unique<PBNodePainter>();
 * scene->setNodePainter(std::move(painter));
 * 
 * // All nodes automatically use custom painter
 * // Checkboxes appear in node header
 * @endcode
 *
 * **Rendering Pipeline:**
 * @code
 * void PBNodePainter::paint(QPainter* painter, NodeGraphicsObject& ngo) const {
 *     // 1. Draw background and frame
 *     drawNodeRect(painter, ngo);
 *     
 *     // 2. Draw ports
 *     drawConnectionPoints(painter, ngo);
 *     drawFilledConnectionPoints(painter, ngo);
 *     
 *     // 3. Draw text elements
 *     drawNodeCaption(painter, ngo);
 *     drawEntryLabels(painter, ngo);
 *     
 *     // 4. Draw controls
 *     drawResizeRect(painter, ngo);
 *     drawValidationIcon(painter, ngo);
 *     
 *     // 5. Draw custom checkboxes
 *     drawEnableCheckbox(painter, ngo);
 *     drawLockCheckbox(painter, ngo);
 *     drawMinimizeCheckbox(painter, ngo);
 * }
 * @endcode
 *
 * **Checkbox Positions:**
 * @code
 * // Example layout (approximate coordinates)
 * Enable checkbox:   (5, 5) in node local coordinates
 * Lock checkbox:     (85, 5)
 * Minimize checkbox: (165, 5)
 * 
 * // Each checkbox: ~15x15 pixels
 * // Spacing: ~70 pixels between checkboxes
 * @endcode
 *
 * **Enable/Disable Functionality:**
 * @code
 * // When enable checkbox is unchecked:
 * // - Node is grayed out (reduced opacity)
 * // - Node doesn't process data
 * // - Connections remain visible
 * // - Can still be moved/edited
 * 
 * if (!nodeEnabled) {
 *     painter->setOpacity(0.5);  // Grayed out
 * }
 * @endcode
 *
 * **Lock Position Functionality:**
 * @code
 * // When lock checkbox is checked:
 * // - Node cannot be dragged
 * // - Connections can still be made/removed
 * // - Properties can still be edited
 * // - Lock icon displayed
 * 
 * if (nodeLocked) {
 *     ngo.setFlag(QGraphicsItem::ItemIsMovable, false);
 * }
 * @endcode
 *
 * **Minimize Functionality:**
 * @code
 * // When minimize checkbox is checked:
 * // - Node collapses to title + checkboxes only
 * // - Ports hidden (connections remain)
 * // - Reduced vertical size
 * // - Caption still visible
 * 
 * if (nodeMinimized) {
 *     // Draw only header region
 *     // Skip port labels and resize handle
 * }
 * @endcode
 *
 * **Drawing Methods:**
 * Each drawing method handles a specific visual component:
 * - drawNodeRect(): Background, border, shadow
 * - drawConnectionPoints(): Port circles
 * - drawFilledConnectionPoints(): Active connection indicators
 * - drawNodeCaption(): Title text
 * - drawEntryLabels(): Input/output port labels
 * - drawResizeRect(): Resize handle (bottom-right)
 * - drawValidationIcon(): Error/warning icons
 * - drawEnableCheckbox(): Enable/disable control
 * - drawLockCheckbox(): Position lock control
 * - drawMinimizeCheckbox(): Collapse/expand control
 *
 * **Checkbox Interaction:**
 * @code
 * // Mouse click handling (in NodeGraphicsObject)
 * void NodeGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event) {
 *     QPointF pos = event->pos();
 *     
 *     // Check if click is on enable checkbox
 *     if (enableCheckboxRect.contains(pos)) {
 *         toggleEnabled();
 *         update();  // Trigger repaint
 *     }
 *     
 *     // Similar for lock and minimize
 * }
 * @endcode
 *
 * **Style Customization:**
 * @code
 * // Checkboxes use QtNodes style system
 * // Colors from NodeStyle:
 * // - Checkbox border: NodeStyle::OutlineColor
 * // - Checkbox fill: NodeStyle::BackgroundColor
 * // - Checkmark: NodeStyle::ForegroundColor
 * // - Hover: NodeStyle::HighlightColor
 * @endcode
 *
 * @see QtNodes::AbstractNodePainter for base painter interface
 * @see QtNodes::NodeGraphicsObject for node graphics representation
 * @see QtNodes::DefaultNodePainter for standard rendering reference
 * @see QtNodes::NodeStyle for styling configuration
 */
class CVDEVSHAREDLIB_EXPORT PBNodePainter : public QtNodes::AbstractNodePainter
{
public:
    /**
     * @brief Main paint method for rendering nodes with custom controls.
     *
     * Implements the AbstractNodePainter interface to draw all node visual elements
     * including standard components (background, ports, labels) and custom checkboxes.
     *
     * @param painter QPainter for rendering operations
     * @param ngo NodeGraphicsObject representing the node to paint
     *
     * **Rendering Order:**
     * 1. Node background rectangle and border
     * 2. Connection points (input/output ports)
     * 3. Filled connection indicators (active connections)
     * 4. Node caption text
     * 5. Entry/exit port labels
     * 6. Resize handle (if node is resizable)
     * 7. Validation icon (errors/warnings)
     * 8. Enable/disable checkbox
     * 9. Lock position checkbox
     * 10. Minimize/expand checkbox
     *
     * **Example Implementation:**
     * @code
     * void PBNodePainter::paint(QPainter* painter, NodeGraphicsObject& ngo) const {
     *     painter->save();
     *     
     *     // Standard elements
     *     drawNodeRect(painter, ngo);
     *     drawConnectionPoints(painter, ngo);
     *     drawFilledConnectionPoints(painter, ngo);
     *     drawNodeCaption(painter, ngo);
     *     drawEntryLabels(painter, ngo);
     *     
     *     // Optional elements
     *     if (ngo.nodeGeometry().resizable()) {
     *         drawResizeRect(painter, ngo);
     *     }
     *     
     *     if (ngo.nodeState().hasErrors()) {
     *         drawValidationIcon(painter, ngo);
     *     }
     *     
     *     // Custom checkboxes
     *     drawEnableCheckbox(painter, ngo);
     *     drawLockCheckbox(painter, ngo);
     *     drawMinimizeCheckbox(painter, ngo);
     *     
     *     painter->restore();
     * }
     * @endcode
     *
     * @note Painter state is saved/restored to prevent side effects.
     * @note All coordinates are in node local space.
     *
     * @see AbstractNodePainter::paint() for interface definition
     */
    void paint(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const override;

private:
    /**
     * @brief Draws the node's background rectangle and border.
     *
     * Renders the main rectangular background with rounded corners, border,
     * and optional shadow effects. Uses NodeStyle colors and geometry.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing geometry and style
     *
     * **Visual Elements:**
     * - Rounded rectangle background (NodeStyle::BackgroundColor)
     * - Border outline (NodeStyle::OutlineColor)
     * - Drop shadow (if enabled in style)
     * - Gradient fill (optional, based on style)
     *
     * @note Coordinates from ngo.nodeGeometry().boundingRect()
     * @see NodeGeometry for size calculations
     * @see NodeStyle for color configuration
     */
    void drawNodeRect(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;

    /**
     * @brief Draws connection point circles for input/output ports.
     *
     * Renders small circles at port locations to indicate where connections
     * can be made. Uses port geometry from NodeGeometry.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing port positions
     *
     * **Port Rendering:**
     * - Circle radius: ~5-7 pixels
     * - Border: NodeStyle::OutlineColor
     * - Fill: NodeStyle::BackgroundColor
     * - Hover state: NodeStyle::HighlightColor
     *
     * @note Left side = input ports, right side = output ports
     * @see drawFilledConnectionPoints() for active connection indicators
     */
    void drawConnectionPoints(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;

    /**
     * @brief Draws filled indicators for ports with active connections.
     *
     * Highlights connection points that have established connections by
     * filling the port circles with a distinct color.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing connection state
     *
     * **Active Connection Indicator:**
     * - Filled circle at port location
     * - Color: NodeStyle::ConnectionColor
     * - Only drawn if port has connection
     *
     * @see drawConnectionPoints() for base port rendering
     */
    void drawFilledConnectionPoints(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;

    /**
     * @brief Draws the node's caption (title) text.
     *
     * Renders the node's display name at the top of the node, typically
     * centered or left-aligned in the header area.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing caption text
     *
     * **Caption Rendering:**
     * - Font: NodeStyle::CaptionFont
     * - Color: NodeStyle::ForegroundColor
     * - Position: Top center or left of node
     * - Truncation: Ellipsis if too long
     *
     * @note Caption text comes from NodeDelegateModel::caption()
     */
    void drawNodeCaption(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;

    /**
     * @brief Draws labels for input and output ports.
     *
     * Renders text labels next to each port showing the port name/type,
     * helping users identify connection purposes.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing port label information
     *
     * **Label Layout:**
     * - Input labels: Right-aligned, left of port circles
     * - Output labels: Left-aligned, right of port circles
     * - Font: NodeStyle::EntryFont
     * - Color: NodeStyle::ForegroundColor
     *
     * **Example:**
     * @code
     * Image ○              ○ Result
     * Kernel ○             ○ Status
     * @endcode
     *
     * @note Labels come from PortData::name in the node delegate
     */
    void drawEntryLabels(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;

    /**
     * @brief Draws the resize handle for resizable nodes.
     *
     * Renders a small drag handle in the bottom-right corner for nodes
     * that support dynamic resizing.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing resize capability info
     *
     * **Resize Handle:**
     * - Position: Bottom-right corner
     * - Icon: Grip lines or corner symbol
     * - Color: NodeStyle::OutlineColor
     * - Size: ~10x10 pixels
     *
     * @note Only drawn if NodeGeometry::resizable() returns true
     * @note Interaction handled by NodeGraphicsObject mouse events
     */
    void drawResizeRect(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;

    /**
     * @brief Draws validation icons for errors or warnings.
     *
     * Renders status icons (error, warning, info) when the node has
     * validation issues or informational messages.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing validation state
     *
     * **Validation Icons:**
     * - Error: Red ⚠ or ✕ symbol
     * - Warning: Yellow ⚠ symbol
     * - Info: Blue ℹ symbol
     * - Position: Top-right or bottom-center
     *
     * **Example:**
     * @code
     * +--------------------+
     * | Node Name      [⚠]|  <- Error icon
     * | ...               |
     * +--------------------+
     * @endcode
     *
     * @note Icons only shown if NodeState has errors/warnings
     * @see NodeState::errors() for validation messages
     */
    void drawValidationIcon(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;
    
    /**
     * @brief Draws the enable/disable checkbox control.
     *
     * Renders an interactive checkbox that allows users to enable or disable
     * node processing. Disabled nodes are grayed out and skip data processing.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing enable state
     *
     * **Checkbox Rendering:**
     * - Position: Top-left header area (~5, 5)
     * - Size: ~15x15 pixels
     * - Border: NodeStyle::OutlineColor
     * - Fill: NodeStyle::BackgroundColor (checked), transparent (unchecked)
     * - Checkmark: NodeStyle::ForegroundColor
     *
     * **States:**
     * - Checked: Node processes data normally
     * - Unchecked: Node skips processing, outputs null/default values
     *
     * **Visual Feedback:**
     * @code
     * // Enabled
     * [✓] Enable
     * 
     * // Disabled (node grayed out)
     * [ ] Enable
     * @endcode
     *
     * @note State stored in node's custom data or properties
     * @note Click handling in NodeGraphicsObject::mousePressEvent()
     */
    void drawEnableCheckbox(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;
    
    /**
     * @brief Draws the lock position checkbox control.
     *
     * Renders an interactive checkbox that prevents node movement when checked,
     * useful for fixed-position nodes in the graph layout.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing lock state
     *
     * **Checkbox Rendering:**
     * - Position: Top-center header area (~85, 5)
     * - Size: ~15x15 pixels
     * - Icon: Lock symbol 🔒 when checked
     * - Border/fill: Same style as enable checkbox
     *
     * **States:**
     * - Checked: Node cannot be dragged/moved
     * - Unchecked: Node can be freely positioned
     *
     * **Behavior:**
     * @code
     * if (lockCheckboxChecked) {
     *     ngo.setFlag(QGraphicsItem::ItemIsMovable, false);
     * } else {
     *     ngo.setFlag(QGraphicsItem::ItemIsMovable, true);
     * }
     * @endcode
     *
     * @note Locked nodes can still be connected and edited
     * @note Lock state persists in saved graph files
     */
    void drawLockCheckbox(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;
    
    /**
     * @brief Draws the minimize/expand checkbox control.
     *
     * Renders an interactive checkbox that collapses the node to a compact
     * view showing only the header and checkboxes, hiding ports and labels.
     *
     * @param painter QPainter for drawing operations
     * @param ngo NodeGraphicsObject providing minimize state
     *
     * **Checkbox Rendering:**
     * - Position: Top-right header area (~165, 5)
     * - Size: ~15x15 pixels
     * - Icon: Minimize symbol (−) or expand symbol (+)
     * - Border/fill: Same style as other checkboxes
     *
     * **States:**
     * - Checked (minimized): Node shows only header + checkboxes
     * - Unchecked (expanded): Node shows full content
     *
     * **Minimized View:**
     * @code
     * // Expanded
     * +--------------------+
     * | [✓][🔒][−] Caption|
     * | Input ○    ○ Out  |
     * +--------------------+
     * 
     * // Minimized
     * +--------------------+
     * | [✓][🔒][+] Caption|
     * +--------------------+
     * @endcode
     *
     * @note Connections remain visible even when minimized
     * @note Minimize state affects NodeGeometry height calculation
     */
    void drawMinimizeCheckbox(QPainter *painter, QtNodes::NodeGraphicsObject &ngo) const;
};
