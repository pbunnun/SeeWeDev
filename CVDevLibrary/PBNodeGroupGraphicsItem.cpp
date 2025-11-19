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

#include "PBNodeGroupGraphicsItem.hpp"
#include "PBDataFlowGraphicsScene.hpp"
#include "PBDataFlowGraphModel.hpp"
#include <QtNodes/internal/StyleCollection.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QPainter>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMenu>
#include <QTime>
#include <limits>

PBNodeGroupGraphicsItem::PBNodeGroupGraphicsItem(GroupId groupId, QGraphicsItem* parent)
    : QGraphicsRectItem(parent)
    , mGroupId(groupId)
{
    // Set Z-value to render behind nodes (nodes typically use z=0)
    setZValue(-10);
    
    // Create label
    mpLabel = new QGraphicsTextItem(this);
    // Make label not accept events so they bubble to the group
    mpLabel->setAcceptedMouseButtons(Qt::NoButton);
    mpLabel->setAcceptHoverEvents(false);
    QFont font = mpLabel->font();
    font.setBold(true);
    font.setPointSize(12);  // Increased from 10 to 12
    mpLabel->setFont(font);
    mpLabel->setDefaultTextColor(Qt::white);  // Default white color for label
    
    // Set flags - make it selectable and movable for user interaction
    // But don't allow selection by clicking (we'll handle that differently)
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
    
    // Make group not block mouse events to nodes above it
    // Do NOT force groups to always stack behind parent; this prevents
    // bringing them to front on selection. Keep normal stacking rules
    // so we can change z-values when selected.
    
    // Explicitly accept context menu events
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    
    // Default appearance
    setPen(QPen(QColor(150, 150, 150), 2, Qt::DashLine));
    setBrush(QBrush(QColor(200, 200, 200, 50)));
}

void PBNodeGroupGraphicsItem::setGroup(const PBNodeGroup& group)
{
    mName = group.name();
    mColor = group.color();
    mNodeIds = group.nodes();  // Store the node IDs
    mMinimized = group.isMinimized();  // Store minimized state
    
    // Update label
    mpLabel->setPlainText(mName);
    mpLabel->setDefaultTextColor(mLabelColor);  // Apply label color
    
    // Update appearance
    QColor borderColor = mColor.darker(130);
    borderColor.setAlpha(200);
    setPen(QPen(borderColor, 2, Qt::DashLine));
    setBrush(QBrush(mColor));
    
    update();
}

void PBNodeGroupGraphicsItem::setLabelColor(const QColor& color)
{
    mLabelColor = color;
    if (mpLabel) {
        mpLabel->setDefaultTextColor(color);
        update();
    }
}

void PBNodeGroupGraphicsItem::updateBounds(const std::map<NodeId, QPointF>& nodePositions,
                                          const std::map<NodeId, QSizeF>& nodeSizes)
{
    if (nodePositions.empty()) {
        hide();
        return;
    }
    
    // Prevent recursive updates during position changes
    if (mUpdatingPosition) {
        return;
    }
    
    mUpdatingPosition = true;
    
    QRectF bounds;
    
    if (mMinimized) {
        // When minimized, show a compact box. Anchor it to the last known
        // expanded top-left if available so the minimize/expand button stays
        // in the same place the user expects. Otherwise fall back to centering
        // the compact box on the group node positions.
        qreal width = 150;
        qreal height = 100;

        if (mHasSavedTopLeft) {
            bounds = QRectF(mSavedTopLeft, QSizeF(width, height));
        } else {
            // Use the center of all nodes as the position
            qreal sumX = 0, sumY = 0;
            int count = 0;
            
            for (const auto& pair : nodePositions) {
                sumX += pair.second.x();
                sumY += pair.second.y();
                count++;
            }
            
            if (count > 0) {
                qreal centerX = sumX / count;
                qreal centerY = sumY / count;
                bounds = QRectF(centerX - width/2, centerY - height/2, width, height);
            }
        }
    } else {
        // Calculate bounding box of all nodes (expanded state)
        qreal minX = std::numeric_limits<qreal>::max();
        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxX = std::numeric_limits<qreal>::lowest();
        qreal maxY = std::numeric_limits<qreal>::lowest();

        for (const auto& pair : nodePositions) {
            NodeId nodeId = pair.first;
            QPointF pos = pair.second;

            // Get node size
            QSizeF size(200, 100);  // Default size
            auto sizeIt = nodeSizes.find(nodeId);
            if (sizeIt != nodeSizes.end()) {
                size = sizeIt->second;
            }

            // Update bounds
            minX = qMin(minX, pos.x());
            minY = qMin(minY, pos.y());
            maxX = qMax(maxX, pos.x() + size.width());
            maxY = qMax(maxY, pos.y() + size.height());
        }

        // Ensure symmetric left/right padding, but larger top padding for label area
        qreal areaWidth = (maxX - minX);
        qreal areaHeight = (maxY - minY);

        qreal centerX = (minX + maxX) / 2.0;
        qreal centerY = (minY + maxY) / 2.0;

        qreal left = centerX - areaWidth / 2.0 - PBNodeGroupGraphicsItem::PADDING_HORIZONTAL;
        qreal top = centerY - areaHeight / 2.0 - PBNodeGroupGraphicsItem::LABEL_TOP_PADDING;  // Larger top padding for label
        qreal width = areaWidth + 2.0 * PBNodeGroupGraphicsItem::PADDING_HORIZONTAL;
        qreal height = areaHeight + PBNodeGroupGraphicsItem::LABEL_TOP_PADDING + PBNodeGroupGraphicsItem::PADDING_VERTICAL;  // Larger top, normal bottom

        bounds = QRectF(left, top, width, height);

        // Save the top-left of the expanded bounds so we can anchor the
        // minimized box in the same place (keeps minimize button stable).
        mSavedTopLeft = QPointF(left, top);
        mHasSavedTopLeft = true;
    }
    
    // During updates, prevent itemChange from emitting signals
    // Set the local rect first
    setRect(0, 0, bounds.width(), bounds.height());
    
    // Then update the position in scene coordinates
    // The flag prevents itemChange from emitting groupMoved again
    setPos(bounds.topLeft());
    
    // Position label - centered horizontally, with gap from top
    if (mMinimized) {
        // Center the label both horizontally and vertically when minimized
        qreal labelX = bounds.width() / 2.0 - mpLabel->boundingRect().width() / 2;
        qreal labelY = bounds.height() / 2.0 - mpLabel->boundingRect().height() / 2;
        mpLabel->setPos(labelX, labelY);
    } else {
        // Position at center-top with gap for expanded state
        qreal labelX = bounds.width() / 2.0 - mpLabel->boundingRect().width() / 2;
        qreal labelY = 0.0;  // Reduced from LABEL_TOP_PADDING (20) to move label higher
        mpLabel->setPos(labelX, labelY);
    }
    
    show();
    update();
    
    mUpdatingPosition = false;
}

void PBNodeGroupGraphicsItem::paint(QPainter* painter,
                                   const QStyleOptionGraphicsItem* option,
                                   QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Draw rounded rectangle with selection highlight
    QPen drawPen = pen();
    if (isSelected()) {
        // Highlight when selected with thicker, solid border
        drawPen.setWidth(3);
        drawPen.setStyle(Qt::SolidLine);
        drawPen.setColor(QColor(100, 150, 255));
    }
    painter->setPen(drawPen);
    painter->setBrush(brush());
    painter->drawRoundedRect(rect(), CORNER_RADIUS, CORNER_RADIUS);
    
    // Draw minimize button in top-left corner
    QRectF buttonRect(PBNodeGroupGraphicsItem::PADDING_HORIZONTAL + 12, CORNER_RADIUS + 1, MINIMIZE_BUTTON_SIZE, MINIMIZE_BUTTON_SIZE);
    painter->setPen(QPen(QColor(100, 100, 100), 1));
    painter->setBrush(QBrush(QColor(220, 220, 220)));
    painter->drawRoundedRect(buttonRect, 3, 3);
    
    // Draw minimize/expand symbol
    QPen symbolPen(QColor(80, 80, 80), 2);
    symbolPen.setCapStyle(Qt::RoundCap);
    painter->setPen(symbolPen);
    
    if (mMinimized) {
        // Draw '+' symbol for expand
        qreal cx = buttonRect.center().x();
        qreal cy = buttonRect.center().y();
        qreal size = 5;
        painter->drawLine(cx - size, cy, cx + size, cy);  // Horizontal line
        painter->drawLine(cx, cy - size, cx, cy + size);  // Vertical line
    } else {
        // Draw '-' symbol for minimize
        qreal cx = buttonRect.center().x();
        qreal cy = buttonRect.center().y();
        qreal size = 5;
        painter->drawLine(cx - size, cy, cx + size, cy);  // Horizontal line
    }

    // If minimized, draw group ports along left/right edges.
    if (mMinimized) {
        auto *ps = dynamic_cast<PBDataFlowGraphicsScene*>(scene());
        if (ps) {
            auto *pbModel = dynamic_cast<PBDataFlowGraphModel*>(&ps->graphModel());
            if (pbModel) {
                const PBNodeGroup* group = pbModel->getGroup(mGroupId);
                if (group) {
                    // Count only ports that have active connections (links)
                    unsigned int mInputs = 0;
                    unsigned int nOutputs = 0;

                    // Map group ports -> (nodeId, nodePortIndex)
                    auto inputMap = group->getInputPortMapping(*pbModel);
                    for (const auto &p : inputMap) {
                        NodeId nid = p.second.first;
                        QtNodes::PortIndex nodePort = p.second.second;
                        auto const &connected = pbModel->connections(nid, QtNodes::PortType::In, nodePort);
                        if (!connected.empty())
                            ++mInputs;
                    }

                    auto outputMap = group->getOutputPortMapping(*pbModel);
                    for (const auto &p : outputMap) {
                        NodeId nid = p.second.first;
                        QtNodes::PortIndex nodePort = p.second.second;
                        auto const &connected = pbModel->connections(nid, QtNodes::PortType::Out, nodePort);
                        if (!connected.empty())
                            ++nOutputs;
                    }

                    const auto &nodeStyle = QtNodes::StyleCollection::nodeStyle();
                    qreal portRadius = nodeStyle.ConnectionPointDiameter * 0.6; // unconnected port size

                    painter->setPen(nodeStyle.NormalBoundaryColor);
                    painter->setBrush(nodeStyle.ConnectionPointColor);

                    qreal w = rect().width();
                    qreal h = rect().height();

                    // Place port centers exactly on the left/right edges so each
                    // dot is half inside the group and half outside (center at x=0 or x=w).
                    qreal leftX = 0.0;
                    qreal rightX = w;

                    // Draw m input dots along left edge (evenly spaced)
                    if (mInputs > 0) {
                        for (unsigned int i = 0; i < mInputs; ++i) {
                            qreal y = h * ((i + 1.0) / (mInputs + 1.0));
                            painter->drawEllipse(QPointF(leftX, y), portRadius, portRadius);
                        }
                    }

                    // Draw n output dots along right edge (evenly spaced)
                    if (nOutputs > 0) {
                        for (unsigned int i = 0; i < nOutputs; ++i) {
                            qreal y = h * ((i + 1.0) / (nOutputs + 1.0));
                            painter->drawEllipse(QPointF(rightX, y), portRadius, portRadius);
                        }
                    }
                }
            }
        }
    }
}

QPainterPath PBNodeGroupGraphicsItem::shape() const
{
    // For context menus and mouse events, we need to accept the entire interior
    // Even though visually we only highlight the border
    QPainterPath path;
    QRectF r = rect();
    
    // Include the entire rectangle to receive context menu and double-click events
    // This allows right-clicking anywhere in the group to show menu
    path.addRect(r);
    
    return path;
}

void PBNodeGroupGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    mLastMousePos = event->scenePos();
    QPointF localPos = mapFromScene(event->scenePos());
    
    // Bring this group to front when clicked
    bringToFront();
    
    // Check if clicked on minimize button
    QRectF buttonRect(PBNodeGroupGraphicsItem::PADDING_HORIZONTAL + 12, CORNER_RADIUS + 1, MINIMIZE_BUTTON_SIZE, MINIMIZE_BUTTON_SIZE);
    if (buttonRect.contains(localPos)) {
        emit toggleMinimizeRequested(mGroupId);
        event->accept();
        return;
    }
    
    QGraphicsRectItem::mousePressEvent(event);
}

void PBNodeGroupGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    // Accept right-click releases to prevent scene from processing them
    if (event->button() == Qt::RightButton) {
        event->accept();
        return;
    }
    QGraphicsRectItem::mouseReleaseEvent(event);
}

QVariant PBNodeGroupGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene() && !mUpdatingPosition) {
        // Calculate delta from current position
        QPointF newPos = value.toPointF();
        QPointF currentPos = pos();
        QPointF delta = newPos - currentPos;
        
        // If the group is minimized and the position change originates from
        // user interaction (not from updateBounds), remember the new top-left
        // so future updates anchor the minimized box to this position.
        if (mMinimized) {
            mSavedTopLeft = newPos;
            mHasSavedTopLeft = true;
        }

        if (!delta.isNull() && delta.manhattanLength() > 0.1) {
            // Emit signal for the scene to handle moving nodes
            emit groupMoved(mGroupId, delta);
            // Allow the group to move with the delta
            return newPos;
        }
    }
    else if (change == ItemSelectedChange) {
        if (value.toBool()) {
            // Bring to front when selected
            bringToFront();
        } else {
            // Send back behind nodes when deselected
            setZValue(-10);

            // Restore saved z-values for member nodes
            if (scene()) {
                auto *ps = dynamic_cast<PBDataFlowGraphicsScene*>(scene());
                if (ps) {
                    for (const auto &pair : mSavedNodeZ) {
                        NodeId nid = pair.first;
                        qreal oldZ = pair.second;
                        if (auto *ngo = ps->nodeGraphicsObject(nid)) {
                            ngo->setZValue(oldZ);
                        }
                    }
                }
            }
            mSavedNodeZ.clear();
        }
    }
    
    return QGraphicsRectItem::itemChange(change, value);
}

void PBNodeGroupGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    // Let itemChange handle the movement via ItemPositionChange
    QGraphicsRectItem::mouseMoveEvent(event);
}

void PBNodeGroupGraphicsItem::bringToFront()
{
    if (!scene()) return;

    // Set this group's z-value well above other items
    setZValue(10000.0);

    // Also raise all member nodes so they display over other nodes/groups
    auto *ps = dynamic_cast<PBDataFlowGraphicsScene*>(scene());
    if (!ps) {
        return;
    }

    // Save current z-values and then raise nodes
    mSavedNodeZ.clear();
    for (NodeId nid : mNodeIds) {
        if (auto *ngo = ps->nodeGraphicsObject(nid)) {
            // Save old z
            mSavedNodeZ[nid] = ngo->zValue();
            // Raise slightly above group
            ngo->setZValue(10001.0);
        }
    }

    // No debug logging in production build
}

void PBNodeGroupGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // Context menu is now handled in PBFlowGraphicsView::contextMenuEvent
    // Just accept the event to prevent it from propagating to the scene
    event->accept();
}
