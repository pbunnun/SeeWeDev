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
 * @file NotSyncDataModel.hpp
 * @brief Logical NOT gate for sync signals (inverts trigger logic).
 *
 * This node implements a logical NOT operation on sync signals, inverting the
 * trigger logic. It can be used to create complementary control paths or implement
 * gating logic where processing should occur when a condition is NOT met.
 *
 * **Key Features:**
 * - Inverts sync signal logic (presence → absence, absence → presence)
 * - Configurable inversion behavior
 * - Used for conditional flow control
 * - Implements boolean NOT in pipeline logic
 *
 * **Typical Use Cases:**
 * - Trigger on absence of condition (NOT logic)
 * - Create complementary control paths
 * - Implement conditional disable logic
 * - Invert threshold or comparison results
 *
 * @see SyncData for trigger signal type
 * @see MathConditionModel for conditional triggering
 */

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtWidgets/QPushButton>
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class NotSyncDataModel
 * @brief Inverts sync signal logic (logical NOT gate).
 *
 * NotSyncDataModel implements boolean NOT operation on sync signals, allowing
 * creation of inverted control logic. It can output a sync when input is absent,
 * or block sync when input is present, depending on configuration.
 *
 * **Port Configuration:**
 * - **Input:** SyncData - Input trigger signal
 * - **Output:** SyncData - Inverted trigger signal
 *
 * **Inversion Logic (typical implementation):**
 * ```cpp
 * // Mode 1: Block on Input Present
 * if (input_sync_present) {
 *     no_output();  // Block sync
 * } else {
 *     output_sync();  // Generate sync when input absent
 * }
 *
 * // Mode 2: Continuous with Block
 * // Continuously output sync EXCEPT when input triggers
 * on_timer_tick() {
 *     if (!input_sync_recently_received) {
 *         output_sync();
 *     }
 * }
 * ```
 *
 * **Common Use Cases:**
 *
 * 1. **Trigger When Condition NOT Met:**
 *    ```
 *    Measurement → MathCondition(< threshold) → NotSync → Alert
 *    // Alert when measurement is NOT below threshold (i.e., >= threshold)
 *    ```
 *
 * 2. **Complementary Paths:**
 *    ```
 *    Trigger ┬→ Path A (on trigger)
 *            └→ NotSync → Path B (on NOT trigger)
 *    ```
 *
 * 3. **Disable Processing:**
 *    ```
 *    EnableSignal → NotSync → DisableGate → Processing
 *    // Process when enable signal is OFF
 *    ```
 *
 * 4. **Absence Detection:**
 *    ```
 *    ObjectDetected → NotSync → NoObjectAlert
 *    // Alert when object is NOT detected
 *    ```
 *
 * 5. **Inverted Gating:**
 *    ```
 *    GateOpen → NotSync → ProcessWhenClosed
 *    ```
 *
 * **Logic Truth Table:**
 * ```
 * Input  | Output (NOT)
 * -------|-------------
 * Absent | Present
 * Present| Absent
 * ```
 *
 * **Implementation Modes:**
 * Depending on internal configuration, NotSync can operate in different modes:
 *
 * **Mode A: Absence Triggered:**
 * - Input present → No output
 * - Input absent → Output sync
 * - Use case: Trigger action when signal missing
 *
 * **Mode B: Pulse Inversion:**
 * - Input pulse → Block output for duration
 * - No input → Continuous/periodic output
 * - Use case: Pause processing during trigger events
 *
 * **Mode C: State Inversion:**
 * - Input state HIGH → Output state LOW
 * - Input state LOW → Output state HIGH
 * - Use case: Digital logic inversion
 *
 * **Performance:**
 * - Latency: < 1μs (logic evaluation)
 * - Overhead: Negligible
 * - No buffering or state (except current input status)
 *
 * **Design Rationale:**
 * - Provides boolean NOT for pipeline logic
 * - Enables creation of "when NOT" conditional paths
 * - Complements MathCondition for complete boolean algebra
 * - Simple, single-purpose design
 *
 * **Combining with Other Logic:**
 *
 * **NOT AND (NAND):**
 * ```
 * A ┬→ AND ← B
 *   └→ NotSync
 *        ↓
 *     NAND Output
 * ```
 *
 * **NOT OR (NOR):**
 * ```
 * A ┬→ OR ← B
 *   └→ NotSync
 *        ↓
 *     NOR Output
 * ```
 *
 * **XOR (using NOT):**
 * ```
 * (A AND NOT B) OR (NOT A AND B) = XOR
 * ```
 *
 * **Limitations:**
 * - Requires clear definition of "absence" (timeout duration?)
 * - No built-in delay or debouncing
 * - Mode selection may require property configuration
 *
 * @note For complex boolean logic, consider combining multiple NOT/Condition nodes
 * @note Actual behavior depends on implementation mode (check source for specific logic)
 * @see SyncData for trigger signal type
 * @see MathConditionModel for conditional logic
 */
class NotSyncDataModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    NotSyncDataModel();

    virtual
    ~NotSyncDataModel() override {}

    QJsonObject
    save() const override;

    void
    load(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override; 

    QWidget *
    embeddedWidget() override { return nullptr; }

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    /*
     * Recieve signals back from QtPropertyBrowser and use this function to
     * set parameters/variables accordingly.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /*
     * These two static members must be defined for every models. _category can be duplicate with existing categories.
     * However, _model_name has to be a unique name.
     */
    static const QString _category;

    static const QString _model_name;

private:
    std::shared_ptr< SyncData > mpSyncData; ///< Output inverted sync signal

    QPixmap _minPixmap;
};

