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

#include "PBConnectionPainter.hpp"
#include "PBDataFlowGraphModel.hpp"
#include "PBNodeGroup.hpp"
#include "PBDataFlowGraphicsScene.hpp"
#include "PBNodeGroupGraphicsItem.hpp"

#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/AbstractGraphModel.hpp>
#include <QtNodes/internal/StyleCollection.hpp>
#include <QtGui/QPainter>
#include <cmath>
#include <QPainterPath>
#include <QPainterPathStroker>

using namespace QtNodes;

PBConnectionPainter::PBConnectionPainter(PBDataFlowGraphModel &graphModel)
    : mGraphModel(graphModel)
{
}

bool PBConnectionPainter::areNodesInSameGroup(NodeId outNodeId, NodeId inNodeId) const
{
    // Get the group IDs for both nodes
    GroupId outGroupId = mGraphModel.getPBNodeGroup(outNodeId);
    GroupId inGroupId = mGraphModel.getPBNodeGroup(inNodeId);
    
    // Both must be in a valid group and the same group
    return (outGroupId != InvalidGroupId && 
            inGroupId != InvalidGroupId && 
            outGroupId == inGroupId);
}

bool PBConnectionPainter::isEitherEndpointInMinimizedGroup(NodeId outNodeId, NodeId inNodeId) const
{
    GroupId outGroupId = mGraphModel.getPBNodeGroup(outNodeId);
    GroupId inGroupId = mGraphModel.getPBNodeGroup(inNodeId);

    if (outGroupId != InvalidGroupId) {
        const PBNodeGroup* outGroup = mGraphModel.getGroup(outGroupId);
        if (outGroup != nullptr && outGroup->isMinimized())
            return true;
    }

    if (inGroupId != InvalidGroupId) {
        const PBNodeGroup* inGroup = mGraphModel.getGroup(inGroupId);
        if (inGroup != nullptr && inGroup->isMinimized())
            return true;
    }

    return false;
}

QPainterPath PBConnectionPainter::cubicPathNormal(QPointF const &out, QPointF const &in) const
{
    // Standard cubic bezier path calculation (same as framework default)
    double const horizontalDistance = in.x() - out.x();
    double const controlPointDistance = std::abs(horizontalDistance) / 3.0;
    
    QPointF c1(out.x() + controlPointDistance, out.y());
    QPointF c2(in.x() - controlPointDistance, in.y());
    
    QPainterPath path(out);
    path.cubicTo(c1, c2, in);
    return path;
}

QPainterPath PBConnectionPainter::cubicPath(QtNodes::ConnectionGraphicsObject const &connection) const
{
    QPointF const &in = connection.endPoint(PortType::In);
    QPointF const &out = connection.endPoint(PortType::Out);
    
    ConnectionId const connId = connection.connectionId();
    NodeId outNodeId = connId.outNodeId;
    NodeId inNodeId = connId.inNodeId;
    
    // Determine group-aware boundary points. For minimized groups we will
    // route connections to/from the compact group's left/right edge ports
    // instead of skipping the connection entirely.
    
    // If nodes are in the same group, use normal cubic bezier
    if (areNodesInSameGroup(outNodeId, inNodeId)) {
        return cubicPathNormal(out, in);
    }
    
    // Check if nodes are in different groups for cross-group routing
    GroupId outGroupId = mGraphModel.getPBNodeGroup(outNodeId);
    GroupId inGroupId = mGraphModel.getPBNodeGroup(inNodeId);
    
    // Get the groups if they exist
    const PBNodeGroup* outGroup = (outGroupId != InvalidGroupId) ? mGraphModel.getGroup(outGroupId) : nullptr;
    const PBNodeGroup* inGroup = (inGroupId != InvalidGroupId) ? mGraphModel.getGroup(inGroupId) : nullptr;
    
    // Apply group-aware routing if at least one node is in a group
    if (outGroup != nullptr || inGroup != nullptr) {
        // At least one node is in a group - apply boundary routing
        
        // Cast the base scene to our custom scene type
        auto* baseScene = connection.nodeScene();
        PBDataFlowGraphicsScene* scene = nullptr;
        if (baseScene) {
            scene = dynamic_cast<PBDataFlowGraphicsScene*>(baseScene);
        }
        
        QPointF outBoundaryPoint = out;
        QPointF inBoundaryPoint = in;
        bool outUsesGroupBoundary = false;
        bool inUsesGroupBoundary = false;
        
        if (scene != nullptr) {
            // Work in scene coordinates for group edges then map back to
            // connection-local coordinates. This avoids mixing scene X with
            // connection-local Y which produced inconsistent positions.

            // If output is in a group, compute the scene-point at the group's
            // right edge but with the output port's scene Y, then map back.
            if (outGroup != nullptr) {
                PBNodeGroupGraphicsItem* outGroupItem = scene->getGroupGraphicsItem(outGroupId);
                if (outGroupItem != nullptr) {
                    QRectF outGroupRect = outGroupItem->sceneBoundingRect();

                    // If the group is minimized, determine the index of this
                    // node+port among the group's active output ports and
                    // compute the corresponding Y along the group's right edge.
                    if (outGroup->isMinimized()) {
                        // Map group output ports -> (nodeId, nodePortIndex)
                        auto outputMap = outGroup->getOutputPortMapping(mGraphModel);

                        // Count active output ports (those that have connections)
                        unsigned int activeCount = 0;
                        int targetActiveIndex = -1;
                        for (const auto &p : outputMap) {
                            NodeId nid = p.second.first;
                            QtNodes::PortIndex nodePort = p.second.second;
                            auto const &connected = mGraphModel.connections(nid, QtNodes::PortType::Out, nodePort);
                            if (!connected.empty()) {
                                if (nid == outNodeId && nodePort == connId.outPortIndex) {
                                    targetActiveIndex = static_cast<int>(activeCount);
                                }
                                ++activeCount;
                            }
                        }

                        // If we found the target among active ports, compute Y
                        if (activeCount > 0 && targetActiveIndex >= 0) {
                            qreal h = outGroupRect.height();
                            qreal yScene = outGroupRect.top() + h * ((targetActiveIndex + 1.0) / (activeCount + 1.0));
                            QPointF outBoundaryScene(outGroupRect.right(), yScene);
                            outBoundaryPoint = connection.sceneTransform().inverted().map(outBoundaryScene);
                            outUsesGroupBoundary = true;
                        } else {
                            // Fallback to previous behavior using port's scene Y
                            QPointF sceneOut = connection.sceneTransform().map(out);
                            QPointF outBoundaryScene(outGroupRect.right(), sceneOut.y());
                            outBoundaryPoint = connection.sceneTransform().inverted().map(outBoundaryScene);
                        }
                    } else {
                        QPointF sceneOut = connection.sceneTransform().map(out);
                        QPointF outBoundaryScene(outGroupRect.right(), sceneOut.y());
                        outBoundaryPoint = connection.sceneTransform().inverted().map(outBoundaryScene);
                    }
                }
            }

            // If input is in a group, compute the scene-point at the group's
            // left edge but with the input port's scene Y, then map back.
            if (inGroup != nullptr) {
                PBNodeGroupGraphicsItem* inGroupItem = scene->getGroupGraphicsItem(inGroupId);
                if (inGroupItem != nullptr) {
                    QRectF inGroupRect = inGroupItem->sceneBoundingRect();

                    if (inGroup->isMinimized()) {
                        // Map group input ports -> (nodeId, nodePortIndex)
                        auto inputMap = inGroup->getInputPortMapping(mGraphModel);

                        // Count active input ports and find target index
                        unsigned int activeCount = 0;
                        int targetActiveIndex = -1;
                        for (const auto &p : inputMap) {
                            NodeId nid = p.second.first;
                            QtNodes::PortIndex nodePort = p.second.second;
                            auto const &connected = mGraphModel.connections(nid, QtNodes::PortType::In, nodePort);
                            if (!connected.empty()) {
                                if (nid == inNodeId && nodePort == connId.inPortIndex) {
                                    targetActiveIndex = static_cast<int>(activeCount);
                                }
                                ++activeCount;
                            }
                        }

                        if (activeCount > 0 && targetActiveIndex >= 0) {
                            qreal h = inGroupRect.height();
                            qreal yScene = inGroupRect.top() + h * ((targetActiveIndex + 1.0) / (activeCount + 1.0));
                            QPointF inBoundaryScene(inGroupRect.left(), yScene);
                            inBoundaryPoint = connection.sceneTransform().inverted().map(inBoundaryScene);
                            inUsesGroupBoundary = true;
                        } else {
                            QPointF sceneIn = connection.sceneTransform().map(in);
                            QPointF inBoundaryScene(inGroupRect.left(), sceneIn.y());
                            inBoundaryPoint = connection.sceneTransform().inverted().map(inBoundaryScene);
                        }
                    } else {
                        QPointF sceneIn = connection.sceneTransform().map(in);
                        QPointF inBoundaryScene(inGroupRect.left(), sceneIn.y());
                        inBoundaryPoint = connection.sceneTransform().inverted().map(inBoundaryScene);
                    }
                }
            }
        }
        
        // Only apply group-aware routing if we have meaningful separation
        // or if one of them isn't in a group (then boundary point equals port point)
        double const horizontalDistance = inBoundaryPoint.x() - outBoundaryPoint.x();
        
        if (std::abs(horizontalDistance) > 10.0) {
            // Create a smooth 3-section path between the appropriate
            // start/end points. If an endpoint uses the group boundary
            // we stop at that boundary point rather than the hidden node
            // port inside the minimized group.
            QPointF pathStart = outUsesGroupBoundary ? outBoundaryPoint : out;
            QPointF pathEnd = inUsesGroupBoundary ? inBoundaryPoint : in;

            QPainterPath path(pathStart);

            // Section 1: Horizontal from output port to output boundary (if different)
            if (!qFuzzyCompare(pathStart, outBoundaryPoint)) {
                path.lineTo(outBoundaryPoint);
            } else if (!qFuzzyCompare(pathStart, out)) {
                path.lineTo(outBoundaryPoint);
            }

            // Section 2: Cubic bezier between boundaries (or between port and boundary)
            double const controlPointDistance = std::abs(horizontalDistance) / 3.0;

            QPointF c1(outBoundaryPoint.x() + controlPointDistance, outBoundaryPoint.y());
            QPointF c2(inBoundaryPoint.x() - controlPointDistance, inBoundaryPoint.y());
            path.cubicTo(c1, c2, inBoundaryPoint);

            // Section 3: Horizontal into the input port if the input is not
            // using the group boundary. Otherwise we stop at the boundary.
            if (!inUsesGroupBoundary) {
                path.lineTo(in);
            }

            return path;
        } else {
            // Too close or overlapping - prefer a right-angle polyline when
            // at least one endpoint is grouped. This avoids a smooth cubic
            // when the group boundary aligns with the node edge and a
            // right-angle is visually expected.
            if (outGroup != nullptr || inGroup != nullptr) {
                // Use boundary-aware right-angle routing. If an endpoint uses
                // the group boundary, use those points instead of node ports.
                QPointF pathStart = outUsesGroupBoundary ? outBoundaryPoint : out;
                QPointF pathEnd = inUsesGroupBoundary ? inBoundaryPoint : in;

                // Choose a common X to route vertically at: prefer the
                // output boundary X if available, otherwise the input
                // boundary X.
                double boundaryX = (outGroup != nullptr) ? outBoundaryPoint.x() : inBoundaryPoint.x();

                QPainterPath path(pathStart);

                // Section 1: horizontal from output to boundaryX
                path.lineTo(QPointF(boundaryX, pathStart.y()));

                // Section 2: vertical from output Y to input Y at boundaryX
                path.lineTo(QPointF(boundaryX, pathEnd.y()));

                // Section 3: horizontal into the input (or boundary)
                path.lineTo(pathEnd);

                return path;
            }

            // Fallback to normal cubic bezier
            return cubicPathNormal(out, in);
        }
    }
    
    // Standard cubic bezier for ungrouped nodes or nodes in only one group
    return cubicPathNormal(out, in);
}

QPainterPath PBConnectionPainter::getPainterStroke(QtNodes::ConnectionGraphicsObject const &connection) const
{
    ConnectionId const connId = connection.connectionId();
    NodeId outNodeId = connId.outNodeId;
    NodeId inNodeId = connId.inNodeId;

    // Compute the same boundary points used when painting so the stroke
    // bounding region includes the group-boundary segments even when
    // minimizing.

    auto cubic = cubicPath(connection);

    QPointF const &out = connection.endPoint(PortType::Out);
    QPainterPath result(out);

    unsigned int constexpr segments = 20;

    for (auto i = 0u; i < segments; ++i) {
        double ratio = double(i + 1) / segments;
        result.lineTo(cubic.pointAtPercent(ratio));
    }

    QPainterPathStroker stroker;
    stroker.setWidth(10.0);

    QPainterPath stroke = stroker.createStroke(result);

    // Ensure boundary points are included in the stroke bounds. Compute
    // the same out/in boundary points as used when painting and add
    // small ellipses so the bounding rect used by ConnectionGraphicsObject
    // includes the horizontal/vertical group-boundary segments.
    QPointF outBoundaryPoint = connection.endPoint(PortType::Out);
    QPointF inBoundaryPoint = connection.endPoint(PortType::In);
    bool outUsesGroupBoundary = false;
    bool inUsesGroupBoundary = false;

    GroupId outGroupId = mGraphModel.getPBNodeGroup(outNodeId);
    GroupId inGroupId = mGraphModel.getPBNodeGroup(inNodeId);

    auto* baseScene = connection.nodeScene();
    PBDataFlowGraphicsScene* scene = nullptr;
    if (baseScene) {
        scene = dynamic_cast<PBDataFlowGraphicsScene*>(baseScene);
    }

    if (scene != nullptr) {
        if (outGroupId != InvalidGroupId) {
            PBNodeGroupGraphicsItem* outGroupItem = scene->getGroupGraphicsItem(outGroupId);
            if (outGroupItem != nullptr) {
                QRectF outGroupRect = outGroupItem->sceneBoundingRect();

                const PBNodeGroup* outGroup = mGraphModel.getGroup(outGroupId);
                if (outGroup != nullptr && outGroup->isMinimized()) {
                    auto outputMap = outGroup->getOutputPortMapping(mGraphModel);
                    unsigned int activeCount = 0;
                    int targetActiveIndex = -1;
                    for (const auto &p : outputMap) {
                        NodeId nid = p.second.first;
                        QtNodes::PortIndex nodePort = p.second.second;
                        auto const &connected = mGraphModel.connections(nid, QtNodes::PortType::Out, nodePort);
                        if (!connected.empty()) {
                            if (nid == outNodeId && nodePort == connId.outPortIndex) {
                                targetActiveIndex = static_cast<int>(activeCount);
                            }
                            ++activeCount;
                        }
                    }

                    if (activeCount > 0 && targetActiveIndex >= 0) {
                        qreal h = outGroupRect.height();
                        qreal yScene = outGroupRect.top() + h * ((targetActiveIndex + 1.0) / (activeCount + 1.0));
                        QPointF outBoundaryScene(outGroupRect.right(), yScene);
                        outBoundaryPoint = connection.sceneTransform().inverted().map(outBoundaryScene);
                        outUsesGroupBoundary = true;
                    } else {
                        QPointF sceneOut = connection.sceneTransform().map(connection.endPoint(PortType::Out));
                        QPointF outBoundaryScene(outGroupRect.right(), sceneOut.y());
                        outBoundaryPoint = connection.sceneTransform().inverted().map(outBoundaryScene);
                    }
                } else {
                    QPointF sceneOut = connection.sceneTransform().map(connection.endPoint(PortType::Out));
                    QPointF outBoundaryScene(outGroupRect.right(), sceneOut.y());
                    outBoundaryPoint = connection.sceneTransform().inverted().map(outBoundaryScene);
                }
            }
        }

        if (inGroupId != InvalidGroupId) {
            PBNodeGroupGraphicsItem* inGroupItem = scene->getGroupGraphicsItem(inGroupId);
            if (inGroupItem != nullptr) {
                QRectF inGroupRect = inGroupItem->sceneBoundingRect();

                const PBNodeGroup* inGroup = mGraphModel.getGroup(inGroupId);
                if (inGroup != nullptr && inGroup->isMinimized()) {
                    auto inputMap = inGroup->getInputPortMapping(mGraphModel);
                    unsigned int activeCount = 0;
                    int targetActiveIndex = -1;
                    for (const auto &p : inputMap) {
                        NodeId nid = p.second.first;
                        QtNodes::PortIndex nodePort = p.second.second;
                        auto const &connected = mGraphModel.connections(nid, QtNodes::PortType::In, nodePort);
                        if (!connected.empty()) {
                            if (nid == inNodeId && nodePort == connId.inPortIndex) {
                                targetActiveIndex = static_cast<int>(activeCount);
                            }
                            ++activeCount;
                        }
                    }

                    if (activeCount > 0 && targetActiveIndex >= 0) {
                        qreal h = inGroupRect.height();
                        qreal yScene = inGroupRect.top() + h * ((targetActiveIndex + 1.0) / (activeCount + 1.0));
                        QPointF inBoundaryScene(inGroupRect.left(), yScene);
                        inBoundaryPoint = connection.sceneTransform().inverted().map(inBoundaryScene);
                        inUsesGroupBoundary = true;
                    } else {
                        QPointF sceneIn = connection.sceneTransform().map(connection.endPoint(PortType::In));
                        QPointF inBoundaryScene(inGroupRect.left(), sceneIn.y());
                        inBoundaryPoint = connection.sceneTransform().inverted().map(inBoundaryScene);
                    }
                } else {
                    QPointF sceneIn = connection.sceneTransform().map(connection.endPoint(PortType::In));
                    QPointF inBoundaryScene(inGroupRect.left(), sceneIn.y());
                    inBoundaryPoint = connection.sceneTransform().inverted().map(inBoundaryScene);
                }
            }
        }
    }

    auto const &connectionStyle = StyleCollection::connectionStyle();
    double padding = connectionStyle.pointDiameter() + 6.0;

    stroke.addEllipse(outBoundaryPoint, padding, padding);
    stroke.addEllipse(inBoundaryPoint, padding, padding);

    return stroke;
}

void PBConnectionPainter::paint(QPainter *painter, QtNodes::ConnectionGraphicsObject const &cgo) const
{
    ConnectionId const cid = cgo.connectionId();
    NodeId outNodeId = cid.outNodeId;
    NodeId inNodeId = cid.inNodeId;

    // Compute custom path and draw even when endpoints are in minimized groups
    auto customPath = cubicPath(cgo);

    (void)customPath; // used below; keep variable alive for clarity
    
    auto const &connectionStyle = StyleCollection::connectionStyle();
    
    // Draw hovered or selected state
    bool const hovered = cgo.connectionState().hovered();
    bool const selected = cgo.isSelected();
    
    if (hovered || selected) {
        double const lineWidth = connectionStyle.lineWidth();
        QPen pen;
        pen.setWidth(static_cast<int>(2 * lineWidth));
        pen.setColor(selected ? connectionStyle.selectedHaloColor()
                              : connectionStyle.hoveredColor());
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(customPath);
    }
    
    // Draw sketch line if needed
    ConnectionState const &state = cgo.connectionState();
    if (state.requiresPort()) {
        QPen pen;
        pen.setWidth(static_cast<int>(connectionStyle.constructionLineWidth()));
        pen.setColor(connectionStyle.constructionColor());
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(customPath);
    }
    
    // Draw normal line
    if (!state.requiresPort()) {
        QColor normalColorOut = connectionStyle.normalColor();
        QColor normalColorIn = connectionStyle.normalColor();
        
        bool useGradientColor = false;
        AbstractGraphModel const &graphModel = cgo.graphModel();
        
        if (connectionStyle.useDataDefinedColors()) {
            auto const cId = cgo.connectionId();
            auto dataTypeOut = graphModel
                                   .portData(cId.outNodeId,
                                             PortType::Out,
                                             cId.outPortIndex,
                                             PortRole::DataType)
                                   .value<NodeDataType>();
            auto dataTypeIn = graphModel.portData(cId.inNodeId, 
                                                  PortType::In, 
                                                  cId.inPortIndex, 
                                                  PortRole::DataType)
                                  .value<NodeDataType>();
            
            useGradientColor = (dataTypeOut.id != dataTypeIn.id);
            normalColorOut = connectionStyle.normalColor(dataTypeOut.id);
            normalColorIn = connectionStyle.normalColor(dataTypeIn.id);
        }
        
        double const lineWidth = connectionStyle.lineWidth();
        QPen p;
        p.setWidth(lineWidth);
        
        if (useGradientColor) {
            painter->setBrush(Qt::NoBrush);
            QColor cOut = normalColorOut;
            if (selected)
                cOut = cOut.darker(200);
            p.setColor(cOut);
            painter->setPen(p);
            
            unsigned int constexpr segments = 60;
            for (unsigned int i = 0ul; i < segments; ++i) {
                double ratioPrev = double(i) / segments;
                double ratio = double(i + 1) / segments;
                
                if (i == segments / 2) {
                    QColor cIn = normalColorIn;
                    if (selected)
                        cIn = cIn.darker(200);
                    p.setColor(cIn);
                    painter->setPen(p);
                }
                
                painter->drawLine(customPath.pointAtPercent(ratioPrev),
                                customPath.pointAtPercent(ratio));
            }
        } else {
            QColor c = normalColorOut;
            if (selected)
                c = c.darker(200);
            p.setColor(c);
            painter->setPen(p);
            painter->setBrush(Qt::NoBrush);
            painter->drawPath(customPath);
        }
    }
    
    // Draw end points
    double const pointDiameter = connectionStyle.pointDiameter();
    painter->setPen(connectionStyle.constructionColor());
    painter->setBrush(connectionStyle.constructionColor());
    double const pointRadius = pointDiameter / 2.0;
    // If an endpoint uses a group boundary, draw the endpoint dot at the
    // boundary point instead of the (hidden) node port.
    QPointF displayOut = cgo.out();
    QPointF displayIn = cgo.in();

    // Recompute boundary points for display purposes (same logic as above)
    // This mirrors the computation in getPainterStroke/cubicPath but is
    // kept local for simplicity.
    {
        ConnectionId const connId = cgo.connectionId();
        GroupId outGroupId = mGraphModel.getPBNodeGroup(connId.outNodeId);
        GroupId inGroupId = mGraphModel.getPBNodeGroup(connId.inNodeId);
        auto* baseScene = cgo.nodeScene();
        PBDataFlowGraphicsScene* scene = nullptr;
        if (baseScene) scene = dynamic_cast<PBDataFlowGraphicsScene*>(baseScene);

        if (scene) {
            if (outGroupId != InvalidGroupId) {
                PBNodeGroupGraphicsItem* outGroupItem = scene->getGroupGraphicsItem(outGroupId);
                const PBNodeGroup* outGroup = mGraphModel.getGroup(outGroupId);
                if (outGroupItem && outGroup && outGroup->isMinimized()) {
                    QRectF outRect = outGroupItem->sceneBoundingRect();
                    auto outputMap = outGroup->getOutputPortMapping(mGraphModel);
                    unsigned int activeCount = 0;
                    int targetActiveIndex = -1;
                    for (const auto &p : outputMap) {
                        NodeId nid = p.second.first;
                        QtNodes::PortIndex nodePort = p.second.second;
                        auto const &connected = mGraphModel.connections(nid, QtNodes::PortType::Out, nodePort);
                        if (!connected.empty()) {
                            if (nid == connId.outNodeId && nodePort == connId.outPortIndex) {
                                targetActiveIndex = static_cast<int>(activeCount);
                            }
                            ++activeCount;
                        }
                    }
                    if (activeCount > 0 && targetActiveIndex >= 0) {
                        qreal yScene = outRect.top() + outRect.height() * ((targetActiveIndex + 1.0) / (activeCount + 1.0));
                        QPointF outBoundaryScene(outRect.right(), yScene);
                        displayOut = cgo.sceneTransform().inverted().map(outBoundaryScene);
                    }
                }
            }

            if (inGroupId != InvalidGroupId) {
                PBNodeGroupGraphicsItem* inGroupItem = scene->getGroupGraphicsItem(inGroupId);
                const PBNodeGroup* inGroup = mGraphModel.getGroup(inGroupId);
                if (inGroupItem && inGroup && inGroup->isMinimized()) {
                    QRectF inRect = inGroupItem->sceneBoundingRect();
                    auto inputMap = inGroup->getInputPortMapping(mGraphModel);
                    unsigned int activeCount = 0;
                    int targetActiveIndex = -1;
                    for (const auto &p : inputMap) {
                        NodeId nid = p.second.first;
                        QtNodes::PortIndex nodePort = p.second.second;
                        auto const &connected = mGraphModel.connections(nid, QtNodes::PortType::In, nodePort);
                        if (!connected.empty()) {
                            if (nid == connId.inNodeId && nodePort == connId.inPortIndex) {
                                targetActiveIndex = static_cast<int>(activeCount);
                            }
                            ++activeCount;
                        }
                    }
                    if (activeCount > 0 && targetActiveIndex >= 0) {
                        qreal yScene = inRect.top() + inRect.height() * ((targetActiveIndex + 1.0) / (activeCount + 1.0));
                        QPointF inBoundaryScene(inRect.left(), yScene);
                        displayIn = cgo.sceneTransform().inverted().map(inBoundaryScene);
                    }
                }
            }
        }
    }

    painter->drawEllipse(displayOut, pointRadius, pointRadius);
    painter->drawEllipse(displayIn, pointRadius, pointRadius);
}
