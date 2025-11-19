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
 * @file PBConnectionPainter.hpp
 * @brief Custom connection painter with group-aware routing for cross-group connections.
 *
 * This file defines the PBConnectionPainter class, which extends QtNodes::DefaultConnectionPainter
 * to provide custom connection routing for nodes in different groups.
 *
 * **Key Features:**
 * - **Intra-group Connections:** Nodes in same group use normal bezier routing
 * - **Cross-group Connections:** Split into two sections:
 *   - **Section 1 (Inside source group):** Horizontal line from port to group boundary
 *   - **Section 2 (Outside):** Normal bezier from group edge to destination
 * - **Smart Boundary Routing:** Output ports route to right border, input ports to left border
 * - **Consistent Styling:** Inherits all styling from default connection painter
 *
 * **Visual Behavior:**
 * @code
 * // Nodes in same group - normal routing (no special handling)
 * Node1 ○ ╭─────────┐
 *         ╰─────────╯ Node2 ○
 * 
 * // Cross-group connection - section routing
 * ┌─────────────────────┐        
 * │ Group A             │        
 * │  Node1 ○ ──→ RIGHT  │        
 * │        (horiz.line) │            ┌──────────────────┐
 * │            ●────────┤──┐         │ Group B          │
 * └─────────────────────┘  │         │  Node2 ○         │
 *                          └─────────│─● (receives from │
 *                    (cubic bezier)  │   LEFT)          │   
 * @endcode                           └──────────────────┘
 *
 * **Connection Port Types:**
 * - **Output Port (source):** Routes horizontally to RIGHT edge of group
 * - **Input Port (destination):** Routes horizontally from LEFT edge of group
 */

#pragma once

#include "CVDevLibrary.hpp"
#include <QtNodes/internal/DefaultConnectionPainter.hpp>

namespace QtNodes {
    class ConnectionGraphicsObject;
}

class PBDataFlowGraphModel;

/**
 * @class PBConnectionPainter
 * @brief Custom connection painter for cross-group connection routing.
 *
 * Extends QtNodes::DefaultConnectionPainter to modify connection paths when
 * endpoints are in different groups. For intra-group connections, uses normal
 * routing. For cross-group connections, routes with horizontal sections inside
 * groups and normal bezier outside.
 *
 * **Inheritance:**
 * @code
 * QtNodes::DefaultConnectionPainter
 *   └── PBConnectionPainter
 * @endcode
 *
 * **Routing Strategy:**
 * 1. Check if output and input nodes are in the same group
 * 2. If same group: Use normal cubic bezier path
 * 3. If different groups:
 *    - Find group boundaries (left/right edges)
 *    - Route from output port horizontally to right edge of source group
 *    - Route from left edge of destination group horizontally to input port
 *    - Connect the two sections with normal cubic bezier at group edges
 *
 * **Usage:**
 * @code
 * // Register in PBDataFlowGraphicsScene
 * auto painter = std::make_unique<PBConnectionPainter>(pbGraphModel);
 * scene->setConnectionPainter(std::move(painter));
 * @endcode
 */
class CVDEVSHAREDLIB_EXPORT PBConnectionPainter : public QtNodes::DefaultConnectionPainter
{
public:
    /**
     * @brief Constructor taking reference to graph model for group queries.
     *
     * @param graphModel PBDataFlowGraphModel to query for node group membership
     *
     * @note Stores reference to model for checking group associations
     */
    explicit PBConnectionPainter(PBDataFlowGraphModel &graphModel);

    /**
     * @brief Custom paint implementation with group-aware routing.
     *
     * Overrides the default paint method to implement custom connection routing
     * for nodes within the same group. Delegates to the base class painting
     * logic but uses our custom path generation.
     *
     * @param painter QPainter for drawing operations
     * @param cgo ConnectionGraphicsObject to paint
     */
    void paint(QPainter *painter, QtNodes::ConnectionGraphicsObject const &cgo) const override;

    /**
     * @brief Generates a cubic bezier path with cross-group routing.
     *
     * Implements different routing strategies based on group membership:
     * - **Intra-group:** Use standard cubic bezier (normal behavior)
     * - **Cross-group:** Split into horizontal sections at group edges + bezier between edges
     *
     * **For Cross-Group Connections:**
     * The path is constructed as:
     * 1. Get source node's group (if any) and destination node's group (if any)
     * 2. If nodes in different groups:
     *    - Find group boundary rectangles
     *    - Create horizontal line from source port to right edge of source group
     *    - Create cubic bezier from right edge to left edge
     *    - Create horizontal line from left edge of dest group to destination port
     * 3. If same group or no groups: Use standard cubic bezier
     *
     * **Algorithm Details:**
     * - Group edges are determined from the group's bounding rectangle
     * - Output ports (source) route to right edge (x = group.right())
     * - Input ports (destination) route to left edge (x = group.left())
     * - Vertical position preserved at group edges
     * - Smooth transitions using cubic bezier segments
     *
     * @param connection ConnectionGraphicsObject containing connection endpoints
     * @return QPainterPath Path with group-aware routing sections
     *
     * @see QtNodes::DefaultConnectionPainter::cubicPath() for default behavior
     * @see getGroupBounds() for group boundary calculation
     */
    QPainterPath cubicPath(QtNodes::ConnectionGraphicsObject const &connection) const;

    /**
     * @brief Provide a painter stroke that matches our custom path.
     *
     * DefaultConnectionPainter::getPainterStroke() builds a stroke from its
     * own cubicPath implementation which does not include the horizontal
     * group-boundary segments introduced here. Override it so bounding boxes
     * and hit testing include the full painted path.
     */
    QPainterPath getPainterStroke(QtNodes::ConnectionGraphicsObject const &cgo) const override;

private:
    /**
     * @brief Checks if both nodes are in the same group.
     *
     * Queries the graph model to determine if the source and destination nodes
     * of a connection belong to the same node group.
     *
     * @param outNodeId NodeId of the output (source) node
     * @param inNodeId NodeId of the input (destination) node
     * @return bool True if both nodes are in the same group, false otherwise
     *
     * @note Returns false if either node has no group assignment
     * @note Groups are compared by GroupId
     */
    bool areNodesInSameGroup(QtNodes::NodeId outNodeId, QtNodes::NodeId inNodeId) const;

    /**
     * @brief Returns true when either endpoint belongs to a group that is minimized.
     *
     * When a group is minimized we prefer to hide connection lines that target
     * nodes inside that group. This helper centralizes that test.
     */
    bool isEitherEndpointInMinimizedGroup(QtNodes::NodeId outNodeId, QtNodes::NodeId inNodeId) const;

    /**
     * @brief Generates a standard cubic bezier path between two points.
     *
     * Used as fallback when group-aware routing cannot be applied.
     *
     * @param out Source point
     * @param in Destination point
     * @return QPainterPath Standard cubic bezier path
     */
    QPainterPath cubicPathNormal(QPointF const &out, QPointF const &in) const;

private:
    /// Reference to graph model for querying node group membership
    PBDataFlowGraphModel &mGraphModel;
};
