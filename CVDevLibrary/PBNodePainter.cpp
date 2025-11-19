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

#include "PBNodePainter.hpp"
#include "PBNodeDelegateModel.hpp"

#include <QtNodes/internal/AbstractGraphModel.hpp>
#include <QtNodes/internal/AbstractNodeGeometry.hpp>
#include <QtNodes/internal/BasicGraphicsScene.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtNodes/internal/DataFlowGraphModel.hpp>
#include <QtNodes/internal/NodeDelegateModel.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/NodeState.hpp>
#include <QtNodes/internal/StyleCollection.hpp>

#include <QtCore/QMargins>
#include <cmath>

using namespace QtNodes;

void PBNodePainter::paint(QPainter *painter, NodeGraphicsObject &ngo) const
{
    // Check if node is minimized
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    
    auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&model);
    if (dataFlowModel) {
        auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
        if (delegateModel && delegateModel->isMinimize()) {
            // Node is minimized - hide the embedded widget if it exists
            if (auto *widget = model.nodeData<QWidget*>(nodeId, NodeRole::Widget)) {
                widget->hide();
            }
            
            // Draw only the minPixmap and checkboxes
            drawNodeRect(painter, ngo);  // Still draw the background

            // Draw the minPixmap in the center - resize to 70x70
            QPixmap minPixmap = delegateModel->minPixmap();
            if (!minPixmap.isNull()) {
                // Resize the pixmap to 70x70 to fit the minimized node
                minPixmap = minPixmap.scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation);

                AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();
                QSize size = geometry.size(nodeId);
                
                // Center the pixmap
                QPointF pixmapPos((size.width() - minPixmap.width()) / 2.0,
                                 (size.height() - minPixmap.height()) / 2.0);
                painter->drawPixmap(pixmapPos, minPixmap);
            }
            
            // Draw checkboxes
            drawMinimizeCheckbox(painter, ngo);
            drawLockCheckbox(painter, ngo);
            drawEnableCheckbox(painter, ngo);
            return;
        }
    }
    
    // Normal drawing - ensure widget is visible
    drawNodeRect(painter, ngo);
    drawConnectionPoints(painter, ngo);
    drawFilledConnectionPoints(painter, ngo);
    drawNodeCaption(painter, ngo);
    drawEntryLabels(painter, ngo);
    drawResizeRect(painter, ngo);
    drawValidationIcon(painter, ngo);
    drawEnableCheckbox(painter, ngo);  // Custom enable/disable checkbox
    drawLockCheckbox(painter, ngo);    // Custom lock position checkbox
    drawMinimizeCheckbox(painter, ngo); // Custom minimize checkbox
}

void PBNodePainter::drawNodeRect(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QSize size = geometry.size(nodeId);

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    QVariant var = model.nodeData(nodeId, NodeRole::ValidationState);

    QColor color = ngo.isSelected() ? nodeStyle.SelectedBoundaryColor
                                    : nodeStyle.NormalBoundaryColor;

    auto validationState = NodeValidationState::State::Valid;
    if (var.canConvert<NodeValidationState>()) {
        auto state = var.value<NodeValidationState>();
        validationState = state._state;
        switch (validationState) {
        case NodeValidationState::State::Error:
            color = nodeStyle.ErrorColor;
            break;
        case NodeValidationState::State::Warning:
            color = nodeStyle.WarningColor;
            break;
        default:
            break;
        }
    }

    float penWidth = ngo.nodeState().hovered() ? nodeStyle.HoveredPenWidth : nodeStyle.PenWidth;
    if (validationState != NodeValidationState::State::Valid) {
        float factor = (validationState == NodeValidationState::State::Error) ? 3.0f : 2.0f;
        penWidth *= factor;
    }
    
    // Make boundary thicker when selected
    if (ngo.isSelected()) {
        penWidth *= 2.0f;
    }

    QPen p(color, penWidth);
    painter->setPen(p);

    QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(2.0, size.height()));
    gradient.setColorAt(0.0, nodeStyle.GradientColor0);
    gradient.setColorAt(0.10, nodeStyle.GradientColor1);
    gradient.setColorAt(0.90, nodeStyle.GradientColor2);
    gradient.setColorAt(1.0, nodeStyle.GradientColor3);

    painter->setBrush(gradient);

    QRectF boundary(0, 0, size.width(), size.height());
    double const radius = 3.0;

    painter->drawRoundedRect(boundary, radius, radius);
}

void PBNodePainter::drawConnectionPoints(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    auto const &connectionStyle = StyleCollection::connectionStyle();
    
    // Get boundary color for selected nodes
    QColor boundaryColor = ngo.isSelected() ? nodeStyle.SelectedBoundaryColor
                                             : nodeStyle.NormalBoundaryColor;

    float diameter = nodeStyle.ConnectionPointDiameter;
    auto reducedDiameter = diameter * 0.6;

    for (PortType portType : {PortType::Out, PortType::In}) {
        auto portCountRole = (portType == PortType::Out) ? NodeRole::OutPortCount
                                                         : NodeRole::InPortCount;
        size_t const n = model.nodeData(nodeId, portCountRole).toUInt();

        for (PortIndex portIndex = 0; portIndex < n; ++portIndex) {
            QPointF p = geometry.portPosition(nodeId, portType, portIndex);

            auto const &dataType = model.portData(nodeId, portType, portIndex, PortRole::DataType)
                                       .value<NodeDataType>();

            double r = 1.0;

            NodeState const &state = ngo.nodeState();

            if (auto const *cgo = state.connectionForReaction()) {
                PortType requiredPort = cgo->connectionState().requiredPort();

                if (requiredPort == portType) {
                    ConnectionId possibleConnectionId = makeCompleteConnectionId(cgo->connectionId(),
                                                                                 nodeId,
                                                                                 portIndex);

                    bool const possible = model.connectionPossible(possibleConnectionId);

                    auto cp = cgo->sceneTransform().map(cgo->endPoint(requiredPort));
                    cp = ngo.sceneTransform().inverted().map(cp);

                    auto diff = cp - p;
                    double dist = std::sqrt(QPointF::dotProduct(diff, diff));

                    if (possible) {
                        double const thres = 40.0;
                        r = (dist < thres) ? (2.0 - dist / thres) : 1.0;
                    } else {
                        double const thres = 80.0;
                        r = (dist < thres) ? (dist / thres) : 1.0;
                    }
                }
            }

            // Check if port has connections - skip drawing if it does
            auto const &connected = model.connections(nodeId, portType, portIndex);
            if (!connected.empty()) {
                continue; // Don't draw empty connection point if already connected
            }

            if (connectionStyle.useDataDefinedColors()) {
                painter->setBrush(connectionStyle.normalColor(dataType.id));
            } else {
                painter->setBrush(nodeStyle.ConnectionPointColor);
            }

            // Draw input ports as plungers, output ports as circles
            if (portType == PortType::In) {
                // Draw a plunger shape (semicircle cupping outward from node)
                QPainterPath plungerPath;
                
                // Keep plunger radius same, but use thicker pen when selected
                double plungerRadius = reducedDiameter * r;
                
                // Semicircle arc from bottom to top, curving outward (to the left)
                plungerPath.arcMoveTo(QRectF(p.x() - plungerRadius * 2, p.y() - plungerRadius, 
                                              plungerRadius * 2, plungerRadius * 2), 
                                       270);  // Start at bottom
                plungerPath.arcTo(QRectF(p.x() - plungerRadius * 2, p.y() - plungerRadius, 
                                          plungerRadius * 2, plungerRadius * 2), 
                                   270, 180);  // Arc 180 degrees to top
                
                // Always use boundary color, and match boundary pen width when selected
                double penSize = ngo.isSelected() ? (reducedDiameter * r * 0.4 * 2.0) : (reducedDiameter * r * 0.4);
                painter->setPen(QPen(boundaryColor, penSize));
                painter->setBrush(Qt::NoBrush);
                painter->drawPath(plungerPath);
            } else {
                // Draw circle for output ports
                painter->drawEllipse(p, reducedDiameter * r, reducedDiameter * r);
            }
        }
    }

    if (ngo.nodeState().connectionForReaction()) {
        ngo.nodeState().resetConnectionForReaction();
    }
}

void PBNodePainter::drawFilledConnectionPoints(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());
    
    // Get boundary color for selected nodes
    QColor boundaryColor = ngo.isSelected() ? nodeStyle.SelectedBoundaryColor
                                             : nodeStyle.NormalBoundaryColor;

    auto diameter = nodeStyle.ConnectionPointDiameter;

    for (PortType portType : {PortType::Out, PortType::In}) {
        size_t const n = model
                             .nodeData(nodeId,
                                       (portType == PortType::Out) ? NodeRole::OutPortCount
                                                                   : NodeRole::InPortCount)
                             .toUInt();

        for (PortIndex portIndex = 0; portIndex < n; ++portIndex) {
            QPointF p = geometry.portPosition(nodeId, portType, portIndex);

            auto const &connected = model.connections(nodeId, portType, portIndex);

            if (!connected.empty()) {
                // Use boundary color for connected ports
                painter->setPen(boundaryColor);
                painter->setBrush(boundaryColor);

                // Draw both input and output ports as filled circles when connected
                painter->drawEllipse(p, diameter * 0.4, diameter * 0.4);
            }
        }
    }
}

void PBNodePainter::drawNodeCaption(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    if (!model.nodeData(nodeId, NodeRole::CaptionVisible).toBool())
        return;

    QString const name = model.nodeData(nodeId, NodeRole::Caption).toString();

    QFont f = painter->font();
    f.setBold(true);

    QPointF position = geometry.captionPosition(nodeId);

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    painter->setFont(f);
    painter->setPen(nodeStyle.FontColor);
    painter->drawText(position, name);

    f.setBold(false);
    painter->setFont(f);
}

void PBNodePainter::drawEntryLabels(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    // Check if we should draw entry labels
    auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&model);
    if (dataFlowModel) {
        auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
        if (delegateModel && !delegateModel->isDrawEntries()) {
            // Don't draw entry labels when Draw Entries is false
            return;
        }
    }

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    for (PortType portType : {PortType::Out, PortType::In}) {
        unsigned int n = model.nodeData<unsigned int>(nodeId,
                                                      (portType == PortType::Out)
                                                          ? NodeRole::OutPortCount
                                                          : NodeRole::InPortCount);

        for (PortIndex portIndex = 0; portIndex < n; ++portIndex) {
            auto const &connected = model.connections(nodeId, portType, portIndex);

            QPointF p = geometry.portTextPosition(nodeId, portType, portIndex);

            if (connected.empty())
                painter->setPen(nodeStyle.FontColorFaded);
            else
                painter->setPen(nodeStyle.FontColor);

            QString s;

            if (model.portData<bool>(nodeId, portType, portIndex, PortRole::CaptionVisible)) {
                s = model.portData<QString>(nodeId, portType, portIndex, PortRole::Caption);
            } else {
                auto portData = model.portData(nodeId, portType, portIndex, PortRole::DataType);

                s = portData.value<NodeDataType>().name;
            }

            painter->drawText(p, s);
        }
    }
}

void PBNodePainter::drawResizeRect(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    if (model.nodeFlags(nodeId) & NodeFlag::Resizable) {
        painter->setBrush(Qt::gray);
        painter->drawEllipse(geometry.resizeHandleRect(nodeId));
    }
}

void PBNodePainter::drawValidationIcon(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QVariant var = model.nodeData(nodeId, NodeRole::ValidationState);
    if (!var.canConvert<NodeValidationState>())
        return;

    auto state = var.value<NodeValidationState>();
    if (state._state == NodeValidationState::State::Valid)
        return;

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    QSize size = geometry.size(nodeId);

    QIcon icon(":/info-tooltip.svg");
    QSize iconSize(16, 16);
    QPixmap pixmap = icon.pixmap(iconSize);

    QColor color = (state._state == NodeValidationState::State::Error) ? nodeStyle.ErrorColor
                                                                       : nodeStyle.WarningColor;

    QPointF center(size.width(), 0.0);
    center += QPointF(iconSize.width() / 2.0, -iconSize.height() / 2.0);

    painter->save();

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(center, iconSize.width() / 2.0 + 2.0, iconSize.height() / 2.0 + 2.0);

    QPainter imgPainter(&pixmap);
    imgPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    imgPainter.fillRect(pixmap.rect(), nodeStyle.FontColor);
    imgPainter.end();

    painter->drawPixmap(center.toPoint() - QPoint(iconSize.width() / 2, iconSize.height() / 2),
                        pixmap);

    painter->restore();
}

void PBNodePainter::drawEnableCheckbox(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    // Get the delegate model to check enable state
    // Cast to DataFlowGraphModel to access delegateModel method
    auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&model);
    if (!dataFlowModel)
        return;
    
    auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
    if (!delegateModel)
        return;

    bool isEnabled = delegateModel->isEnable();

    // Position checkbox at bottom-left corner (matches resize handle size)
    QSize size = geometry.size(nodeId);
    const double checkboxSize = 8.0;   // Size of the checkbox (same as resize handle: 7-8px)
    const double margin = 4.0;         // Margin from edges
    
    QPointF checkboxPos(margin, size.height() - checkboxSize - margin);
    QRectF checkboxRect(checkboxPos, QSizeF(checkboxSize, checkboxSize));

    // Draw checkbox with color based on enable state
    painter->save();
    painter->setPen(QPen(Qt::black, 1.0));  // Thinner border for smaller checkbox
    
    if (isEnabled) {
        painter->setBrush(QColor(0, 200, 0));  // Green for enabled
    } else {
        painter->setBrush(QColor(200, 0, 0));  // Red for disabled
    }
    
    painter->drawRect(checkboxRect);
    
    // Draw checkmark if enabled
    if (isEnabled) {
        painter->setPen(QPen(Qt::white, 1.5));  // Thinner checkmark for smaller box
        // Draw a simple checkmark
        QPointF p1 = checkboxPos + QPointF(1.5, checkboxSize / 2.0);
        QPointF p2 = checkboxPos + QPointF(checkboxSize / 3.0, checkboxSize - 2.0);
        QPointF p3 = checkboxPos + QPointF(checkboxSize - 1.5, 1.5);
        
        painter->drawLine(p1, p2);
        painter->drawLine(p2, p3);
    }
    
    painter->restore();
}

void PBNodePainter::drawLockCheckbox(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    // Get the delegate model to check lock state
    // Cast to DataFlowGraphModel to access delegateModel method
    auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&model);
    if (!dataFlowModel)
        return;
    
    auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
    if (!delegateModel)
        return;

    bool isLocked = delegateModel->isLockPosition();

    // Position checkbox at top-right corner
    QSize size = geometry.size(nodeId);
    const double checkboxSize = 8.0;   // Size of the checkbox (same as enable checkbox)
    const double margin = 4.0;         // Margin from edges
    
    QPointF checkboxPos(size.width() - checkboxSize - margin, margin);
    QRectF checkboxRect(checkboxPos, QSizeF(checkboxSize, checkboxSize));

    // Draw checkbox with color based on lock state
    painter->save();
    painter->setPen(QPen(Qt::black, 1.0));  // Thinner border for smaller checkbox
    
    if (isLocked) {
        painter->setBrush(QColor(200, 100, 0));  // Orange for locked
    } else {
        painter->setBrush(QColor(150, 150, 150));  // Gray for unlocked
    }
    
    painter->drawRect(checkboxRect);
    
    // Draw lock icon
    painter->setPen(QPen(Qt::white, 1.2));
    
    if (isLocked) {
        // Draw closed lock: shackle (arc) + body (rect)
        double shackleTop = checkboxPos.y() + 1.5;
        double shackleCenterX = checkboxPos.x() + checkboxSize / 2.0;
        double shackleWidth = checkboxSize * 0.4;
        
        // Draw shackle arc
        QRectF shackleRect(shackleCenterX - shackleWidth / 2.0, shackleTop,
                           shackleWidth, shackleWidth);
        painter->drawArc(shackleRect, 0 * 16, 180 * 16);  // Top half circle
        
        // Draw lock body
        double bodyTop = checkboxPos.y() + checkboxSize * 0.45;
        double bodyHeight = checkboxSize * 0.5;
        double bodyWidth = checkboxSize * 0.6;
        QRectF bodyRect(shackleCenterX - bodyWidth / 2.0, bodyTop,
                        bodyWidth, bodyHeight);
        painter->drawRect(bodyRect);
    } else {
        // Draw open lock: shackle arc (offset) + body (rect)
        double shackleTop = checkboxPos.y() + 1.5;
        double shackleCenterX = checkboxPos.x() + checkboxSize / 2.0 + 1.0;  // Offset right
        double shackleWidth = checkboxSize * 0.4;
        
        // Draw open shackle arc
        QRectF shackleRect(shackleCenterX - shackleWidth / 2.0, shackleTop - 1.0,
                           shackleWidth, shackleWidth);
        painter->drawArc(shackleRect, 45 * 16, 135 * 16);  // Partial arc, open
        
        // Draw lock body
        double bodyTop = checkboxPos.y() + checkboxSize * 0.45;
        double bodyHeight = checkboxSize * 0.5;
        double bodyWidth = checkboxSize * 0.6;
        QRectF bodyRect(shackleCenterX - bodyWidth / 2.0 - 1.0, bodyTop,
                        bodyWidth, bodyHeight);
        painter->drawRect(bodyRect);
    }
    
    painter->restore();
}

void PBNodePainter::drawMinimizeCheckbox(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();

    // Get the delegate model to check minimize state
    // Cast to DataFlowGraphModel to access delegateModel method
    auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&model);
    if (!dataFlowModel)
        return;
    
    auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
    if (!delegateModel)
        return;

    // Don't draw minimize checkbox if the node cannot be minimized
    if (!delegateModel->canMinimize())
        return;

    bool isMinimized = delegateModel->isMinimize();

    // Position checkbox at top-left corner
    const double checkboxSize = 8.0;   // Size of the checkbox (same as other checkboxes)
    const double margin = 4.0;         // Margin from edges
    
    QPointF checkboxPos(margin, margin);
    QRectF checkboxRect(checkboxPos, QSizeF(checkboxSize, checkboxSize));

    // Draw checkbox with color based on minimize state
    painter->save();
    painter->setPen(QPen(Qt::black, 1.0));  // Thinner border for smaller checkbox
    
    if (isMinimized) {
        painter->setBrush(QColor(100, 100, 200));  // Blue for minimized
    } else {
        painter->setBrush(QColor(150, 150, 150));  // Gray for normal
    }
    
    painter->drawRect(checkboxRect);
    
    // Draw minimize icon
    painter->setPen(QPen(Qt::white, 1.2));
    
    if (isMinimized) {
        // Draw expand icon (two arrows pointing outward)
        double centerX = checkboxPos.x() + checkboxSize / 2.0;
        double centerY = checkboxPos.y() + checkboxSize / 2.0;
        double arrowSize = checkboxSize * 0.25;
        
        // Top-left arrow
        painter->drawLine(QPointF(centerX - arrowSize, centerY - arrowSize),
                         QPointF(centerX - arrowSize * 0.3, centerY - arrowSize));
        painter->drawLine(QPointF(centerX - arrowSize, centerY - arrowSize),
                         QPointF(centerX - arrowSize, centerY - arrowSize * 0.3));
        
        // Bottom-right arrow
        painter->drawLine(QPointF(centerX + arrowSize, centerY + arrowSize),
                         QPointF(centerX + arrowSize * 0.3, centerY + arrowSize));
        painter->drawLine(QPointF(centerX + arrowSize, centerY + arrowSize),
                         QPointF(centerX + arrowSize, centerY + arrowSize * 0.3));
    } else {
        // Draw minimize icon (horizontal line)
        double lineY = checkboxPos.y() + checkboxSize * 0.5;
        double lineStartX = checkboxPos.x() + checkboxSize * 0.2;
        double lineEndX = checkboxPos.x() + checkboxSize * 0.8;
        
        painter->drawLine(QPointF(lineStartX, lineY), QPointF(lineEndX, lineY));
    }
    
    painter->restore();
}
