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
 * @file PropertyChangeCommand.hpp
 * @brief Undo/redo command for property changes in node delegates.
 *
 * This file defines the PropertyChangeCommand class, which implements Qt's undo/redo
 * framework for tracking and reverting property modifications in dataflow graph nodes.
 * Supports command merging for seamless user experience during rapid property adjustments.
 *
 * **Key Features:**
 * - **Undo/Redo Support:** Full integration with QUndoStack
 * - **Command Merging:** Combines rapid changes into single undo step
 * - **Type Safety:** QVariant-based value storage for any property type
 * - **Node Integration:** Works with PBNodeDelegateModel and QtNodes scenes
 *
 * **Common Use Cases:**
 * - Property browser edits (slider adjustments, text changes)
 * - Batch property modifications with undo support
 * - Interactive parameter tuning with revert capability
 * - Multi-step workflows with checkpoint rollback
 *
 * **Undo/Redo Integration Pattern:**
 * @code
 * // In MainWindow or property editor
 * QUndoStack* undoStack = new QUndoStack(this);
 * 
 * // When property changes
 * void onPropertyChanged(NodeId id, QString propId, 
 *                        QVariant oldVal, QVariant newVal) {
 *     auto* cmd = new PropertyChangeCommand(
 *         scene, id, delegateModel, propId, oldVal, newVal
 *     );
 *     undoStack->push(cmd);  // Execute and add to undo history
 * }
 * 
 * // User actions
 * undoStack->undo();  // Revert last change
 * undoStack->redo();  // Re-apply reverted change
 * @endcode
 *
 * **Command Merging Example:**
 * @code
 * // User drags slider from 0 to 100
 * // Without merging: 100 separate undo steps
 * // With merging: Single undo step (0 → 100)
 * 
 * PropertyChangeCommand("threshold", 0, 10);   // Push
 * PropertyChangeCommand("threshold", 10, 20);  // Merges
 * PropertyChangeCommand("threshold", 20, 30);  // Merges
 * // ... continues merging same property ...
 * PropertyChangeCommand("threshold", 90, 100); // Merges
 * 
 * // Undo once: 100 → 0 (single step)
 * @endcode
 *
 * **Supported Property Types:**
 * - Primitive: int, double, float, bool, QString
 * - Structures: Size, Rect, Point, FilePath
 * - Enumerations: EnumPropertyType with index
 * - Any type convertible to/from QVariant
 *
 * @see QUndoCommand for Qt undo/redo framework
 * @see PBNodeDelegateModel for node property interface
 * @see PropertyVector for property definitions
 */

#pragma once

#include <QUndoCommand>
#include <QVariant>
#include <QString>

#include <QtNodes/Definitions>

class PBNodeDelegateModel;

namespace QtNodes {
class BasicGraphicsScene;
}

/**
 * @class PropertyChangeCommand
 * @brief Undo/redo command for tracking property changes in node delegates.
 *
 * Implements QUndoCommand to provide undo/redo functionality for property
 * modifications in dataflow graph nodes. Supports command merging to combine
 * rapid successive changes (like slider dragging) into a single undo step.
 *
 * **Core Functionality:**
 * - **Undo/Redo:** Revert and re-apply property value changes
 * - **Command Merging:** Combine consecutive changes to same property
 * - **Scene Integration:** Updates QtNodes scene after property changes
 * - **Type Flexibility:** QVariant supports any property type
 *
 * **Inheritance:**
 * @code
 * QObject
 *   └── QUndoCommand
 *         └── PropertyChangeCommand
 * @endcode
 *
 * **Typical Usage:**
 * @code
 * // Setup undo stack in main window
 * QUndoStack* undoStack = new QUndoStack(mainWindow);
 * 
 * // Create undo/redo actions
 * QAction* undoAction = undoStack->createUndoAction(this, tr("&Undo"));
 * QAction* redoAction = undoStack->createRedoAction(this, tr("&Redo"));
 * undoAction->setShortcut(QKeySequence::Undo);  // Ctrl+Z
 * redoAction->setShortcut(QKeySequence::Redo);  // Ctrl+Y
 * 
 * // When property changes in editor
 * void PropertyEditor::onValueChanged(QString propId, QVariant newValue) {
 *     QVariant oldValue = model->getPropertyValue(propId);
 *     
 *     auto* cmd = new PropertyChangeCommand(
 *         scene, currentNodeId, model, propId, oldValue, newValue
 *     );
 *     undoStack->push(cmd);  // Executes redo() automatically
 * }
 * @endcode
 *
 * **Command Merging Behavior:**
 * @code
 * // Slider continuously adjusted from 0 to 100
 * // Frame 1:
 * PropertyChangeCommand(nodeId, "blur_size", 0, 5);    // Pushed
 * 
 * // Frame 2 (within merge window):
 * PropertyChangeCommand(nodeId, "blur_size", 5, 10);   // Merges with previous
 * 
 * // Frame 3:
 * PropertyChangeCommand(nodeId, "blur_size", 10, 15);  // Merges with previous
 * 
 * // ... many intermediate values ...
 * 
 * // Final:
 * PropertyChangeCommand(nodeId, "blur_size", 95, 100); // Merges with previous
 * 
 * // Result: Single undo step reverts 100 → 0
 * // Instead of: 100 individual undo steps
 * @endcode
 *
 * **Merge Conditions:**
 * Commands merge when ALL of these are true:
 * 1. Same node ID
 * 2. Same property ID
 * 3. Same delegate model
 * 4. Commands pushed consecutively (no other commands between)
 *
 * **Property Change Flow:**
 * @code
 * // 1. User edits property in browser
 * PropertyEditor detects change
 * 
 * // 2. Create command with old and new values
 * auto* cmd = new PropertyChangeCommand(..., oldVal, newVal);
 * 
 * // 3. Push to undo stack (executes redo())
 * undoStack->push(cmd);
 *   → cmd->redo() called
 *   → applyValue(newValue) sets property in model
 *   → scene->update() refreshes node graphics
 * 
 * // 4. User presses Ctrl+Z
 * undoStack->undo();
 *   → cmd->undo() called
 *   → applyValue(oldValue) restores original
 *   → scene->update() refreshes graphics
 * @endcode
 *
 * **Multi-Property Changes:**
 * @code
 * // User changes multiple properties
 * undoStack->beginMacro("Adjust Node Parameters");
 * 
 * undoStack->push(new PropertyChangeCommand(..., "threshold", 128, 150));
 * undoStack->push(new PropertyChangeCommand(..., "blur_size", 3, 5));
 * undoStack->push(new PropertyChangeCommand(..., "iterations", 1, 3));
 * 
 * undoStack->endMacro();
 * 
 * // Single undo reverts all three properties
 * @endcode
 *
 * @see QUndoCommand for Qt undo/redo framework
 * @see QUndoStack for command management
 * @see PBNodeDelegateModel::setModelProperty() for property application
 */
class PropertyChangeCommand : public QUndoCommand
{
public:
    /**
     * @brief Constructs a property change command for undo/redo.
     *
     * Creates a command that can revert and re-apply a property value change
     * in a node delegate model. Automatically adds descriptive text for undo stack.
     *
     * @param scene Pointer to the graphics scene containing the node
     * @param nodeId Unique identifier of the node being modified
     * @param delegateModel Pointer to the node's delegate model (property owner)
     * @param propertyId String identifier of the property being changed
     * @param oldValue Previous value of the property (for undo)
     * @param newValue New value of the property (for redo)
     *
     * **Example:**
     * @code
     * // In property browser value change handler
     * void onPropertyChanged(const QString& propId, const QVariant& value) {
     *     // Get current value before change
     *     QVariant oldVal = nodeModel->getPropertyValue(propId);
     *     
     *     // Create undo command
     *     auto* cmd = new PropertyChangeCommand(
     *         graphicsScene,           // Scene for updates
     *         selectedNodeId,          // Node being edited
     *         nodeModel,               // Model with property
     *         propId,                  // e.g., "threshold"
     *         oldVal,                  // e.g., QVariant(128)
     *         value                    // e.g., QVariant(150)
     *     );
     *     
     *     // Push to undo stack (automatically calls redo())
     *     undoStack->push(cmd);
     * }
     * @endcode
     *
     * **Command Text:**
     * The command text appears in undo menu as: "Change [propertyId]"
     * @code
     * // Example menu entries:
     * "Change threshold"
     * "Change blur_size"
     * "Change file_path"
     * @endcode
     *
     * @note The command takes ownership responsibility - will be deleted by QUndoStack
     * @note redo() is automatically called when pushed to QUndoStack
     *
     * @see QUndoStack::push() for command execution
     * @see undo() for reverting the change
     * @see redo() for applying the change
     */
    PropertyChangeCommand(QtNodes::BasicGraphicsScene *scene,
                          QtNodes::NodeId nodeId,
                          PBNodeDelegateModel *delegateModel,
                          const QString &propertyId,
                          const QVariant &oldValue,
                          const QVariant &newValue);

    /**
     * @brief Reverts the property to its previous value.
     *
     * Called by QUndoStack when user triggers undo operation.
     * Restores the property to its state before this command was executed.
     *
     * **Example Flow:**
     * @code
     * // Initial state: threshold = 128
     * // User changes to: threshold = 150
     * auto* cmd = new PropertyChangeCommand(..., "threshold", 128, 150);
     * undoStack->push(cmd);  // Executes redo(), threshold = 150
     * 
     * // User presses Ctrl+Z
     * undoStack->undo();     // Calls cmd->undo(), threshold = 128
     * @endcode
     *
     * @note Triggers scene update to refresh node graphics
     * @see redo() for re-applying the change
     * @see applyValue() for internal implementation
     */
    void undo() override;

    /**
     * @brief Applies the property's new value.
     *
     * Called by QUndoStack when:
     * 1. Command is first pushed to stack (initial execution)
     * 2. User triggers redo operation after undo
     *
     * **Example Flow:**
     * @code
     * // Create and push command
     * auto* cmd = new PropertyChangeCommand(..., "blur_size", 3, 5);
     * undoStack->push(cmd);
     *   → redo() called automatically
     *   → blur_size = 5
     * 
     * // User undoes
     * undoStack->undo();
     *   → undo() called
     *   → blur_size = 3
     * 
     * // User redoes (Ctrl+Y)
     * undoStack->redo();
     *   → redo() called
     *   → blur_size = 5
     * @endcode
     *
     * @note Triggers scene update to refresh node graphics
     * @see undo() for reverting the change
     * @see applyValue() for internal implementation
     */
    void redo() override;

    /**
     * @brief Returns the command ID for merging support.
     *
     * Provides a unique integer ID that enables the undo framework to identify
     * mergeable commands. Commands with the same ID can potentially merge via
     * mergeWith().
     *
     * @return int Command ID (PROPERTY_CHANGE_COMMAND_ID = 1001) or -1 for no merging
     *
     * **Merging Mechanism:**
     * @code
     * // QUndoStack checks when pushing new command:
     * if (newCmd->id() == topCmd->id() && newCmd->id() != -1) {
     *     if (topCmd->mergeWith(newCmd)) {
     *         // Merged! Delete newCmd, keep topCmd with updated values
     *         return;
     *     }
     * }
     * // Otherwise, push as separate command
     * @endcode
     *
     * **Use Cases:**
     * - Slider dragging: Merge all intermediate values into single undo step
     * - Text typing: Merge character-by-character changes
     * - Spinbox increment/decrement: Merge rapid clicks
     *
     * @note Returns PROPERTY_CHANGE_COMMAND_ID (1001) to enable merging
     * @note Return -1 to disable merging for this command type
     *
     * @see mergeWith() for merge implementation
     * @see QUndoCommand::id() for base documentation
     */
    int id() const override;

    /**
     * @brief Attempts to merge this command with another.
     *
     * Combines consecutive property changes into a single undo step for
     * improved user experience. Merges only if both commands affect the
     * same property on the same node.
     *
     * @param other Pointer to the newer command being pushed
     * @return true if merge succeeded (commands combined), false otherwise
     *
     * **Merge Conditions:**
     * All must be true to merge:
     * 1. `other` is also a PropertyChangeCommand
     * 2. Same node ID
     * 3. Same property ID
     * 4. Same delegate model pointer
     *
     * **Merge Behavior:**
     * @code
     * // Command A already on stack: threshold 100 → 120
     * // Command B being pushed:     threshold 120 → 140
     * 
     * if (A.mergeWith(B)) {
     *     // A becomes:  threshold 100 → 140 (kept)
     *     // B deleted (not added to stack)
     *     // Single undo step: 140 → 100
     * }
     * @endcode
     *
     * **Example Sequence:**
     * @code
     * // User drags slider continuously
     * PropertyChangeCommand(..., "threshold", 0, 10);    // Pushed to stack
     * PropertyChangeCommand(..., "threshold", 10, 20);   // Merges: 0 → 20
     * PropertyChangeCommand(..., "threshold", 20, 30);   // Merges: 0 → 30
     * PropertyChangeCommand(..., "threshold", 30, 40);   // Merges: 0 → 40
     * // ... continues ...
     * PropertyChangeCommand(..., "threshold", 90, 100);  // Merges: 0 → 100
     * 
     * // Result: Stack has ONE command (0 → 100)
     * // Undo once: 100 → 0
     * @endcode
     *
     * **No Merge Examples:**
     * @code
     * // Different properties: No merge
     * PropertyChangeCommand(..., "threshold", 100, 120);  // Pushed
     * PropertyChangeCommand(..., "blur_size", 3, 5);      // Different property, pushed separately
     * 
     * // Different nodes: No merge
     * PropertyChangeCommand(..., nodeId1, ..., 100, 120); // Pushed
     * PropertyChangeCommand(..., nodeId2, ..., 50, 60);   // Different node, pushed separately
     * @endcode
     *
     * **Implementation Note:**
     * @code
     * bool PropertyChangeCommand::mergeWith(const QUndoCommand* other) {
     *     auto* otherCmd = dynamic_cast<const PropertyChangeCommand*>(other);
     *     if (!otherCmd) return false;
     *     
     *     // Check if same property on same node
     *     if (_nodeId != otherCmd->_nodeId) return false;
     *     if (_propertyId != otherCmd->_propertyId) return false;
     *     if (_delegateModel != otherCmd->_delegateModel) return false;
     *     
     *     // Merge: Keep old value, update to new value
     *     _newValue = otherCmd->_newValue;
     *     return true;
     * }
     * @endcode
     *
     * @note Only the top command on the stack can merge with incoming commands
     * @note Merged commands are deleted by QUndoStack
     *
     * @see id() for merge enablement
     * @see QUndoCommand::mergeWith() for base documentation
     */
    bool mergeWith(const QUndoCommand *other) override;

private:
    /**
     * @brief Internal helper to apply a property value.
     *
     * Sets the property in the delegate model and triggers scene update.
     * Used by both undo() and redo() to apply old or new values.
     *
     * @param value QVariant containing the value to apply
     *
     * **Implementation:**
     * @code
     * void PropertyChangeCommand::applyValue(const QVariant& value) {
     *     // Set property in node model
     *     _delegateModel->setModelProperty(_propertyId, value);
     *     
     *     // Trigger node recomputation
     *     _delegateModel->compute();
     *     
     *     // Update graphics
     *     _scene->update();
     * }
     * @endcode
     *
     * @note Triggers full node update cycle (property set, compute, graphics refresh)
     */
    void applyValue(const QVariant &value);

private:
    /**
     * @brief Pointer to the graphics scene containing the node.
     */
    QtNodes::BasicGraphicsScene *_scene;

    /**
     * @brief Unique identifier of the node being modified.
     */
    QtNodes::NodeId _nodeId;

    /**
     * @brief Pointer to the node's delegate model (property owner).
     */
    PBNodeDelegateModel *_delegateModel;

    /**
     * @brief String identifier of the property (e.g., "threshold", "blur_size").
     */
    QString _propertyId;

    /**
     * @brief Previous property value (for undo).
     */
    QVariant _oldValue;

    /**
     * @brief New property value (for redo).
     */
    QVariant _newValue;
    
    /**
     * @brief Unique command ID for merge support.
     *
     * Commands with matching IDs can potentially merge via mergeWith().
     * Value: 1001
     */
    static constexpr int PROPERTY_CHANGE_COMMAND_ID = 1001;
};
