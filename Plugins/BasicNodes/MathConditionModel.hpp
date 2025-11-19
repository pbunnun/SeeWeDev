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
 * @file MathConditionModel.hpp
 * @brief Conditional sync trigger based on numeric comparison.
 *
 * This node implements a mathematical condition evaluator that outputs a sync signal
 * when a specified numeric condition is met. It's used for flow control in pipelines,
 * enabling conditional execution based on numeric thresholds or comparisons.
 *
 * **Key Features:**
 * - Embedded widget for condition configuration
 * - Comparison operators: <, ≤, =, ≥, >, ≠
 * - Numeric threshold value (integer or floating-point)
 * - Sync signal output when condition is TRUE
 * - No output when condition is FALSE
 *
 * **Typical Use Cases:**
 * - Trigger actions when measurements exceed thresholds
 * - Control flow based on computed values (area, count, intensity)
 * - Implement decision logic in pipelines
 * - Quality control pass/fail decisions
 *
 * @see MathConditionEmbeddedWidget for condition configuration UI
 * @see SyncData for trigger signal type
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "MathConditionEmbeddedWidget.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class MathConditionModel
 * @brief Evaluates numeric conditions and outputs sync signals on TRUE.
 *
 * MathConditionModel acts as a decision node that compares input values against
 * a threshold using a selected comparison operator. When the condition evaluates
 * to TRUE, it emits a sync signal; when FALSE, no output is produced.
 *
 * **Port Configuration:**
 * - **Input:** SyncData (implicitly carries numeric value to be tested)
 * - **Output:** SyncData (emitted only when condition is TRUE)
 *
 * **Embedded Widget:**
 * - **Condition Dropdown:** Select comparison operator
 *   - Index 0: < (less than)
 *   - Index 1: ≤ (less than or equal)
 *   - Index 2: = (equal)
 *   - Index 3: ≥ (greater than or equal)
 *   - Index 4: > (greater than)
 *   - Index 5: ≠ (not equal)
 * - **Number Input:** Threshold value (double precision)
 *
 * **Condition Evaluation Logic:**
 * ```cpp
 * double input_value = sync_input->value();  // Extract from SyncData
 * double threshold = mdConditionNumber;
 * int operator_idx = miConditionIndex;
 *
 * bool result = false;
 * switch (operator_idx) {
 *     case 0: result = (input_value < threshold); break;
 *     case 1: result = (input_value <= threshold); break;
 *     case 2: result = (input_value == threshold); break;  // Exact equality (careful with floats!)
 *     case 3: result = (input_value >= threshold); break;
 *     case 4: result = (input_value > threshold); break;
 *     case 5: result = (input_value != threshold); break;
 * }
 *
 * if (result) {
 *     output_sync_signal();  // Propagate sync downstream
 * } else {
 *     no_output();  // Block sync propagation
 * }
 * ```
 *
 * **Common Use Cases:**
 *
 * 1. **Threshold-Based Alerts:**
 *    ```
 *    Temperature → MathCondition(> 75°C) → TriggerAlarm
 *    ```
 *
 * 2. **Quality Control:**
 *    ```
 *    PartArea → MathCondition(< 100 || > 200) → RejectPart
 *    ```
 *
 * 3. **Count-Based Processing:**
 *    ```
 *    ObjectCount → MathCondition(≥ 5) → SaveImage
 *    ```
 *
 * 4. **Range Filtering:**
 *    ```
 *    Measurement ┬→ MathCondition(≥ 10) → Gate1
 *                └→ MathCondition(< 20) → Gate2
 *    [Only values in [10, 20) pass both gates]
 *    ```
 *
 * 5. **Batch Processing Control:**
 *    ```
 *    FrameNumber → MathCondition(= 100) → SaveCheckpoint
 *    ```
 *
 * 6. **Dynamic Thresholding:**
 *    ```
 *    IntensityMean → MathCondition(> dynamic_threshold) → Trigger
 *    ```
 *
 * **Comparison Operator Details:**
 * - **< (Less Than)**: Triggers when input strictly below threshold
 * - **≤ (Less or Equal)**: Triggers when input below or exactly at threshold
 * - **= (Equal)**: Triggers on exact match (use with caution for floating-point!)
 * - **≥ (Greater or Equal)**: Triggers when input above or exactly at threshold
 * - **> (Greater Than)**: Triggers when input strictly above threshold
 * - **≠ (Not Equal)**: Triggers when input differs from threshold (useful for event detection)
 *
 * **Floating-Point Equality Warning:**
 * For operator "=" with floating-point values, consider numerical precision:
 * ```
 * // Instead of: value == 3.14159
 * // Use range: value ≥ 3.14158 AND value ≤ 3.14160
 * // Or epsilon: abs(value - 3.14159) < 0.00001
 * ```
 *
 * **Performance:**
 * - Evaluation: O(1) - single comparison
 * - Latency: < 1μs (negligible)
 * - No buffering or state accumulation
 *
 * **Design Rationale:**
 * - Embedded widget provides intuitive condition setup
 * - Sync-based I/O integrates naturally with trigger-based pipelines
 * - No output on FALSE prevents unnecessary downstream processing
 * - Double precision supports wide range of numeric applications
 *
 * **Troubleshooting:**
 * - **No output ever**: Check operator and threshold settings
 * - **Unexpected triggers**: Verify input value type and range
 * - **Equal operator unreliable**: Use range operators (≥ and ≤) instead for floats
 *
 * @note For complex conditions (AND/OR logic), cascade multiple MathCondition nodes
 * @see MathConditionEmbeddedWidget for condition configuration interface
 * @see SyncData for trigger signal details
 */
class MathConditionModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    MathConditionModel();

    virtual
    ~MathConditionModel() override {}

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
    embeddedWidget() override { return mpEmbeddedWidget; }

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

private Q_SLOTS:
    
    /**
     * @brief Handles condition configuration changes from embedded widget.
     * @param cond_idx Condition operator index (0=<, 1=≤, 2==, 3=≥, 4=>, 5=≠)
     * @param number Threshold value as string (converted to double)
     */
    void
    em_changed(int cond_idx, QString number);

private:
    MathConditionEmbeddedWidget * mpEmbeddedWidget; ///< Condition configuration widget

    std::shared_ptr<SyncData> mpSyncData;   ///< Output sync signal (emitted on TRUE)
    double mdConditionNumber;               ///< Numeric threshold value
    QString msConditionNumber;              ///< Threshold as string (for widget)
    int miConditionIndex;                   ///< Operator index (0-5)
};

