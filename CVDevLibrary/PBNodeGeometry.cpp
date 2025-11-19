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

#include "PBNodeGeometry.hpp"
#include "PBNodeDelegateModel.hpp"

#include <QtNodes/internal/AbstractGraphModel.hpp>
#include <QtNodes/internal/DataFlowGraphModel.hpp>
#include <QWidget>

using namespace QtNodes;

PBNodeGeometry::PBNodeGeometry(AbstractGraphModel &graphModel)
    : DefaultHorizontalNodeGeometry(graphModel)
{
}

QPointF PBNodeGeometry::portPosition(NodeId const nodeId,
                                     PortType const portType,
                                     PortIndex const portIndex) const
{
    QSize size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
    QRectF const capRect = captionRect(nodeId);
    
    // Get the number of ports for this type
    PortCount nPorts = _graphModel.nodeData<PortCount>(nodeId,
        portType == PortType::Out ? NodeRole::OutPortCount : NodeRole::InPortCount);
    
    // Port spacing constant (matches the base class default)
    const unsigned int portSpacing = 10;
    
    // Calculate the vertical region available for ports (below caption)
    double captionHeight = capRect.height() + portSpacing; // caption + spacing
    double availableHeight = size.height() - captionHeight;
    
    // Calculate vertical position to center ports in the available space
    double verticalSpacing = (nPorts > 1) ? availableHeight / (nPorts + 1) : availableHeight / 2.0;
    double yPosition = captionHeight + verticalSpacing * (portIndex + 1);
    
    QPointF result;
    
    switch (portType) {
    case PortType::In: {
        // Input ports on the left side
        result = QPointF(0.0, yPosition);
        break;
    }
    
    case PortType::Out: {
        // Output ports on the right side
        result = QPointF(size.width(), yPosition);
        break;
    }
    
    default:
        break;
    }
    
    return result;
}

void PBNodeGeometry::recomputeSize(NodeId const nodeId) const
{
    // Check if this node is minimized
    auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&_graphModel);
    if (dataFlowModel) {
        auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
        if (delegateModel && delegateModel->isMinimize()) {
            // Set fixed size for minimized nodes - 70x70 square
            QSize minimizedSize(70, 70);
            _graphModel.setNodeData(nodeId, NodeRole::Size, minimizedSize);
            return;
        }
        
        // Check if Draw Entries is disabled
        if (delegateModel && !delegateModel->isDrawEntries()) {
            // When entries are hidden, compute a more compact size
            // First get the default size to get proper height calculation
            DefaultHorizontalNodeGeometry::recomputeSize(nodeId);
            QSize defaultSize = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
            
            // Determine the spacing to use
            // If caption is also hidden, use checkbox margin (4px) for minimal spacing
            // Otherwise, use standard port spacing (10px)
            const unsigned int spacing = delegateModel->captionVisible() ? 10 : 4;
            
            // Calculate new width without port text
            unsigned int width = 4 * spacing;

            if (auto w = _graphModel.nodeData<QWidget *>(nodeId, NodeRole::Widget)) {
                width += w->width(); // Add widget width
            }

            // Ensure minimum width for the caption (if visible)
            if (delegateModel->captionVisible()) {
                QRectF const capRect = captionRect(nodeId);
                width = std::max(width, static_cast<unsigned int>(capRect.width()) + 2 * spacing);
            }

            // Keep the same height but use reduced width
            QSize size(width, defaultSize.height());
            _graphModel.setNodeData(nodeId, NodeRole::Size, size);
            return;
        }
    }
    
    // Otherwise, use the default computation (with port labels)
    DefaultHorizontalNodeGeometry::recomputeSize(nodeId);
}

QPointF PBNodeGeometry::widgetPosition(NodeId const nodeId) const
{
    // Check if Draw Entries is disabled
    auto *dataFlowModel = dynamic_cast<DataFlowGraphModel*>(&_graphModel);
    if (dataFlowModel) {
        auto *delegateModel = dataFlowModel->delegateModel<PBNodeDelegateModel>(nodeId);
        if (delegateModel && !delegateModel->isDrawEntries()) {
            // When entries are hidden, position widget with symmetric margins
            QSize size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
            unsigned int captionHeight = delegateModel->captionVisible() ? captionRect(nodeId).height() : 0;

            if (auto w = _graphModel.nodeData<QWidget *>(nodeId, NodeRole::Widget)) {
                // Use reduced spacing when caption is hidden (4px to match checkbox margin)
                // Otherwise use standard spacing (10px)
                const double spacing = delegateModel->captionVisible() ? 10.0 : 4.0;
                const double widgetX = 2.0 * spacing; // Left margin
                
                // If the widget wants to use as much vertical space as possible,
                // place it immediately after the caption (or at top if no caption).
                if (w->sizePolicy().verticalPolicy() & QSizePolicy::ExpandFlag) {
                    return QPointF(widgetX, spacing + captionHeight);
                } else {
                    return QPointF(widgetX, (captionHeight + size.height() - w->height()) / 2.0);
                }
            }
            return QPointF();
        }
    }
    
    // Use default positioning (with port label spacing)
    return DefaultHorizontalNodeGeometry::widgetPosition(nodeId);
}
