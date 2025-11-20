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

#pragma once

#include <QUndoCommand>
#include <QtNodes/internal/Definitions.hpp>
#include <map>
#include <QSize>

namespace QtNodes {
class BasicGraphicsScene;
}

/**
 * @brief Undo command for resizing one or more nodes.
 *
 * Records original and new sizes for a set of nodes and applies them on
 * undo/redo. Designed to be used from CVDev code (PBDataFlowGraphicsScene)
 * without editing upstream NodeEditor sources.
 */
class ResizeNodeCommand : public QUndoCommand
{
public:
    ResizeNodeCommand(QtNodes::BasicGraphicsScene *scene,
                      const std::map<QtNodes::NodeId, QSize> &oldWidgetSizes,
                      const std::map<QtNodes::NodeId, QSize> &newWidgetSizes,
                      QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override;
    bool mergeWith(const QUndoCommand *other) override;

private:
    QtNodes::BasicGraphicsScene *_scene{nullptr};

    // Maps node id -> widget size
    std::map<QtNodes::NodeId, QSize> _oldWidgetSizes;
    std::map<QtNodes::NodeId, QSize> _newWidgetSizes;
};
