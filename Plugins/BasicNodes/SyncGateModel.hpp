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
 * @file SyncGateModel.hpp
 * @brief Model for logical operations on synchronization and boolean signals.
 *
 * This file defines the SyncGateModel class for performing logical operations (AND, OR,
 * XOR, NAND, NOR, EQUAL, DIRECT) on synchronization signals and boolean data. It supports
 * dual inputs/outputs with configurable port routing via embedded widget, enabling flexible
 * signal flow control in automation and conditional processing workflows.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "BoolData.hpp"

#include "SyncGateEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct LogicGate
 * @brief Enumeration of logical gate operations for sync signals.
 *
 * Defines all supported logical operations that can be applied to synchronization
 * and boolean signals.
 */
typedef struct LogicGate
{
    /**
     * @enum LogicType
     * @brief Supported logical operations.
     *
     * **Comparison:**
     * - EQUAL: Output true if both inputs have same value
     *
     * **Standard Logic Gates:**
     * - AND: Output true if both inputs true
     * - OR: Output true if either input true
     * - XOR: Output true if exactly one input true (exclusive OR)
     * - NOR: Output true if both inputs false (NOT OR)
     * - NAND: Output false if both inputs true (NOT AND)
     *
     * **Pass-through:**
     * - DIRECT: Pass first input unchanged
     * - DIRECT_NOT: Pass inverted first input
     */
    enum LogicType
    {
        EQUAL = 0,       ///< a == b
        AND = 1,         ///< a && b
        OR = 2,          ///< a || b
        XOR = 3,         ///< a ^ b (exclusive OR)
        NOR = 4,         ///< !(a || b)
        NAND = 5,        ///< !(a && b)
        DIRECT = 6,      ///< a (pass-through)
        DIRECT_NOT = 7   ///< !a (inverter)
    };
} LogicGate;

/**
 * @struct SyncGateParameters
 * @brief Configuration parameters for sync gate operations.
 *
 * Stores the selected logical operation type.
 */
typedef struct SyncGateParameters
{
    int miOperation;  ///< Selected operation (LogicGate::LogicType value)
    
    /**
     * @brief Default constructor.
     *
     * Initializes with AND operation as default.
     */
    SyncGateParameters()
        : miOperation(LogicGate::AND)
    {
    }
} SyncGateParameters;

/**
 * @class SyncGateModel
 * @brief Node model for logical operations on synchronization signals.
 *
 * This model performs logical operations on synchronization signals (SyncData) and
 * boolean values (BoolData), supporting configurable dual inputs and outputs. It's
 * essential for building conditional logic, multi-source synchronization, and flow
 * control in automated processing pipelines.
 *
 * **Input Ports (configurable via embedded widget):**
 * - Port 0: SyncData or BoolData (first operand)
 * - Port 1: SyncData or BoolData (second operand, unused for DIRECT operations)
 *
 * **Output Ports (configurable via embedded widget):**
 * - Port 0: SyncData and BoolData (result of logical operation)
 * - Port 1: SyncData and BoolData (duplicate output for branching)
 *
 * **Logical Operations:**
 *
 * 1. **EQUAL:** Outputs true if both inputs have same boolean value
 *    - Use case: Detect when two conditions match
 *    - Example: sensor1_ready == sensor2_ready
 *
 * 2. **AND:** Outputs true only if both inputs are true
 *    - Use case: Require all conditions met before proceeding
 *    - Example: camera_ready AND calibration_complete
 *
 * 3. **OR:** Outputs true if either input is true
 *    - Use case: Trigger on any of multiple conditions
 *    - Example: manual_trigger OR timer_expired
 *
 * 4. **XOR:** Outputs true if exactly one input is true
 *    - Use case: Mutual exclusion, toggle detection
 *    - Example: button1_pressed XOR button2_pressed
 *
 * 5. **NOR:** Outputs true only if both inputs are false
 *    - Use case: Detect when neither condition is active
 *    - Example: NOR(error1, error2) → no errors
 *
 * 6. **NAND:** Outputs false only if both inputs are true
 *    - Use case: Universal gate, condition blocking
 *    - Example: NAND(overheating, overcurrent) → at least one safe
 *
 * 7. **DIRECT:** Passes first input unchanged
 *    - Use case: Signal duplication, routing
 *    - Example: Broadcast one sync signal to multiple paths
 *
 * 8. **DIRECT_NOT:** Inverts first input
 *    - Use case: Signal inversion, "not ready" detection
 *    - Example: Convert "busy" to "idle" signal
 *
 * **Port Configuration:**
 * The embedded widget allows enabling/disabling individual input and output ports:
 * - Disable input port 1 for single-input operations (DIRECT, DIRECT_NOT)
 * - Disable output port 1 if only single output needed
 * - Flexible routing for complex logic networks
 *
 * **Data Type Handling:**
 * - Accepts both SyncData (sync signals) and BoolData (boolean values)
 * - Outputs both types simultaneously for compatibility
 * - SyncData represents "ready" or "trigger" states
 * - BoolData represents general true/false conditions
 *
 * **Properties (Configurable):**
 * - **operation:** Logical operation type (LogicGate::LogicType enum)
 * - **in0_enabled:** Enable/disable input port 0 (via widget)
 * - **in1_enabled:** Enable/disable input port 1 (via widget)
 * - **out0_enabled:** Enable/disable output port 0 (via widget)
 * - **out1_enabled:** Enable/disable output port 1 (via widget)
 *
 * **Use Cases:**
 *
 * 1. **Multi-sensor synchronization:**
 *    @code
 *    [Camera1 Sync] ----\
 *                        [SyncGate: AND] -> [Combined Ready]
 *    [Camera2 Sync] ----/
 *    @endcode
 *
 * 2. **Conditional processing trigger:**
 *    @code
 *    [Quality > Threshold] (Bool) ----\
 *                                      [SyncGate: AND] -> [Save Image Trigger]
 *    [Frame Count > 100] (Bool) ------/
 *    @endcode
 *
 * 3. **Error detection:**
 *    @code
 *    [Temp OK] (Bool) ----\
 *                          [SyncGate: NOR] -> [Error Present]
 *    [Pressure OK] (Bool) /                    (true if neither OK)
 *    @endcode
 *
 * 4. **Signal inversion:**
 *    @code
 *    [Busy Signal] -> [SyncGate: DIRECT_NOT] -> [Idle Signal]
 *    @endcode
 *
 * 5. **Dual-output broadcasting:**
 *    @code
 *    [Trigger] -> [SyncGate: DIRECT, both outputs enabled] -> [Path1]
 *                                                           \-> [Path2]
 *    @endcode
 *
 * **Truth Tables:**
 * @code
 * AND:  0 0→0, 0 1→0, 1 0→0, 1 1→1
 * OR:   0 0→0, 0 1→1, 1 0→1, 1 1→1
 * XOR:  0 0→0, 0 1→1, 1 0→1, 1 1→0
 * NAND: 0 0→1, 0 1→1, 1 0→1, 1 1→0
 * NOR:  0 0→1, 0 1→0, 1 0→0, 1 1→0
 * EQUAL:0 0→1, 0 1→0, 1 0→0, 1 1→1
 * @endcode
 *
 * **Performance Notes:**
 * - Zero computational overhead (simple boolean logic)
 * - No blocking or delays
 * - Instant signal propagation
 *
 * @see SyncGateEmbeddedWidget
 * @see SyncData
 * @see BoolData
 * @see CombineSyncModel
 */
class SyncGateModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a SyncGateModel.
     *
     * Initializes with AND operation as default and creates the embedded
     * widget for port configuration.
     */
    SyncGateModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~SyncGateModel() override {}

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing operation and port states.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved parameters.
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 2 for both input and output (dual ports, configurable).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index (0 or 1).
     * @return SyncData or BoolData (both accepted/produced on each port).
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the output data.
     * @param port Port index (0 or 1).
     * @return Shared pointer to SyncData or BoolData result.
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and triggers logical operation.
     * @param nodeData Input SyncData or BoolData.
     * @param Port index (0 or 1).
     *
     * When both inputs are available (for binary operations), performs
     * the configured logical operation and updates outputs.
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief Returns the embedded port configuration widget.
     * @return Pointer to SyncGateEmbeddedWidget.
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets a model property.
     * @param Property name ("operation").
     * @param QVariant value (LogicGate::LogicType enum value).
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Returns the minimum node icon.
     * @return QPixmap icon.
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private Q_SLOTS :

    /**
     * @brief Slot for port configuration checkbox changes.
     *
     * Handles changes in the embedded widget's port enable/disable checkboxes,
     * reconfiguring the node's active ports dynamically.
     */
    void em_checkbox_checked();

private:
    SyncGateParameters mParams;                                ///< Operation configuration
    SyncGateEmbeddedWidget* mpEmbeddedWidget;                  ///< Port configuration widget
    std::shared_ptr<SyncData> mapSyncInData[2] { {nullptr} };  ///< Input sync signals
    std::shared_ptr<BoolData> mapBoolInData[2] { {nullptr} };  ///< Input boolean values
    std::shared_ptr<SyncData> mapSyncData[2] { {nullptr} };    ///< Output sync signals
    std::shared_ptr<BoolData> mapBoolData[2] { {nullptr} };    ///< Output boolean values
    QPixmap _minPixmap;                                        ///< Node icon

    /**
     * @brief Processes input signals and performs logical operation.
     * @param inSync Array of 2 input SyncData pointers.
     * @param inBool Array of 2 input BoolData pointers.
     * @param outSync Array of 2 output SyncData pointers.
     * @param outBool Array of 2 output BoolData pointers.
     * @param params Operation parameters (selected logic type).
     *
     * Executes the configured logical operation on the input signals and
     * updates both SyncData and BoolData outputs accordingly.
     */
    void processData(const std::shared_ptr<SyncData> (&inSync)[2], const std::shared_ptr<BoolData> (&inBool)[2],
                     std::shared_ptr<SyncData> (&outSync)[2], std::shared_ptr<BoolData> (&outBool)[2],
                     const SyncGateParameters & params);
};

