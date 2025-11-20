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

#include "ResizeNodeCommand.hpp"
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/AbstractNodeGeometry.hpp>
#include <QtNodes/internal/AbstractGraphModel.hpp>

#include <QVariant>

ResizeNodeCommand::ResizeNodeCommand(QtNodes::BasicGraphicsScene *scene,
                                     const std::map<QtNodes::NodeId, QSize> &oldWidgetSizes,
                                     const std::map<QtNodes::NodeId, QSize> &newWidgetSizes,
                                     QUndoCommand *parent)
    : QUndoCommand(parent)
    , _scene(scene)
    , _oldWidgetSizes(oldWidgetSizes)
    , _newWidgetSizes(newWidgetSizes)
{
    setText("Resize nodes");
}

void ResizeNodeCommand::undo()
{
    if (!_scene)
        return;

    auto &model = _scene->graphModel();

    for (const auto &p : _oldWidgetSizes) {
        const QtNodes::NodeId nodeId = p.first;
        const QSize sz = p.second;

        // Apply old size to the widget
        if (auto w = model.nodeData<QWidget *>(nodeId, QtNodes::NodeRole::Widget)) {
            w->resize(sz);
        }

        // Trigger geometry update
        if (auto *ngo = _scene->nodeGraphicsObject(nodeId)) {
            ngo->setGeometryChanged();
            _scene->nodeGeometry().recomputeSize(nodeId);
            ngo->updateQWidgetEmbedPos();
            ngo->update();
            ngo->moveConnections();
        }
    }
}

void ResizeNodeCommand::redo()
{
    if (!_scene)
        return;

    auto &model = _scene->graphModel();

    for (const auto &p : _newWidgetSizes) {
        const QtNodes::NodeId nodeId = p.first;
        const QSize sz = p.second;

        // Apply new size to the widget
        if (auto w = model.nodeData<QWidget *>(nodeId, QtNodes::NodeRole::Widget)) {
            w->resize(sz);
        }

        // Trigger geometry update
        if (auto *ngo = _scene->nodeGraphicsObject(nodeId)) {
            ngo->setGeometryChanged();
            _scene->nodeGeometry().recomputeSize(nodeId);
            ngo->updateQWidgetEmbedPos();
            ngo->update();
            ngo->moveConnections();
        }
    }
}

int ResizeNodeCommand::id() const
{
    // Arbitrary unique id for this command type
    static const int idValue = qRegisterMetaType<ResizeNodeCommand*>();
    return idValue;
}

bool ResizeNodeCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id())
        return false;

    auto otherCmd = static_cast<const ResizeNodeCommand *>(other);

    // Merge only if operating on the same set of nodes
    if (otherCmd->_oldWidgetSizes.size() != _oldWidgetSizes.size())
        return false;

    for (const auto &p : _oldWidgetSizes) {
        auto it = otherCmd->_oldWidgetSizes.find(p.first);
        if (it == otherCmd->_oldWidgetSizes.end())
            return false;
        if (it->second != p.second)
            return false; // old sizes differ
    }

    // Update our newSizes to the other's newSizes (take latest)
    _newWidgetSizes = otherCmd->_newWidgetSizes;

    return true;
}
