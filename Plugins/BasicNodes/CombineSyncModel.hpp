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
 * @file CombineSyncModel.hpp
 * @brief Synchronization combiner node for coordinating multiple data streams.
 *
 * This utility node combines multiple synchronization signals using logical operations
 * (AND/OR), enabling complex pipeline orchestration and multi-source coordination.
 * It's essential for workflows requiring multiple inputs to be ready before proceeding,
 * or for triggering on any of several events.
 *
 * **Key Use Cases**:
 * - Synchronize multiple camera streams
 * - Wait for all preprocessing steps to complete
 * - Trigger on first available data source
 * - Coordinate parallel processing branches
 * - Implement conditional pipeline execution
 *
 * @see SyncData for synchronization signal format
 */

#pragma once

#include <QtCore/QObject>

#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "CombineSyncEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @enum CombineCondition
 * @brief Logical operation for combining synchronization signals.
 *
 * Defines how multiple input sync signals are combined to produce output:
 *
 * - **CombineCondition_AND** (0): Output triggers when ALL inputs are ready
 *   * Use for: Waiting for multiple data sources before processing
 *   * Example: Wait for both left and right camera frames for stereo processing
 *   * Behavior: Output fires only when Input1 AND Input2 are both ready
 *
 * - **CombineCondition_OR** (1): Output triggers when ANY input is ready
 *   * Use for: Processing whichever data arrives first
 *   * Example: Display first available frame from multiple cameras
 *   * Behavior: Output fires when Input1 OR Input2 (or both) are ready
 *
 * **Design Pattern**:
 * - AND: Synchronization barrier (all must arrive)
 * - OR: First-come-first-served (any can trigger)
 */
enum CombineCondition
{
    CombineCondition_AND = 0,  ///< Trigger when both inputs ready (logical AND)
    CombineCondition_OR        ///< Trigger when either input ready (logical OR)
};

/**
 * @class CombineSyncModel
 * @brief Combines multiple synchronization signals using logical AND/OR operations.
 *
 * This coordination node manages multiple synchronization streams, outputting a sync
 * signal based on the configured logical operation. It enables complex pipeline
 * orchestration by controlling when downstream nodes should execute based on the
 * readiness of multiple upstream sources.
 *
 * **Functionality**:
 * - Accepts variable number of SyncData input streams (minimum 2, maximum 10)
 * - Combines using AND (all ready) or OR (any ready) logic
 * - Outputs combined synchronization signal
 * - Interactive embedded widget for operation selection, input count, and reset
 * - Dynamic port addition/removal at runtime
 *
 * **Input Ports**:
 * - Port 0 to N-1: SyncData - Synchronization signals (N = input size, configurable 2-10)
 *
 * **Output Port**:
 * - Port 0: SyncData - Combined synchronization signal
 *
 * **Embedded Widget**:
 * - Row 1: QComboBox for selecting AND/OR operation
 * - Row 2: QSpinBox for setting number of inputs (2-10)
 * - Row 3: Reset button to clear all ready states
 * - Vertical layout for compact presentation
 *
 * **Operation Modes**:
 *
 * 1. **AND Mode** (Synchronization Barrier):
 *    - Output triggers only when ALL inputs have arrived
 *    - Ensures all required data is available before proceeding
 *    - Example pipeline:
 *      ```
 *      Camera1 → Process1 ─┐
 *      Camera2 → Process2 ─┼─[CombineSync:AND]─→ MultiCameraProcessing
 *      Camera3 → Process3 ─┘
 *      ```
 *    - Use cases:
 *      * Multi-camera synchronization (wait for all cameras)
 *      * Multi-sensor fusion (wait for all sensors)
 *      * Parallel preprocessing (wait for all branches)
 *
 * 2. **OR Mode** (First Available):
 *    - Output triggers when ANY input arrives
 *    - Allows processing whichever data is available first
 *    - Example pipeline:
 *      ```
 *      CameraA ─┐
 *      CameraB ─┼─[CombineSync:OR]─→ Display (show first available)
 *      CameraC ─┘
 *      ```
 *    - Use cases:
 *      * Redundant data sources (use whichever arrives first)
 *      * Fallback mechanisms (primary or backup sources)
 *      * Event-driven processing (respond to any trigger)
 *
 * **Timing Behavior**:
 *
 * **AND Mode Timing** (3 inputs example):
 * ```
 * Time:     0    1    2    3    4    5
 * Input1:   ─────X────────X─────────X
 * Input2:   ──────────X────────X────
 * Input3:   ───────X─────────X──────
 * Output:   ──────────X────────X────
 *           (fires when all ready)
 * ```
 *
 * **OR Mode Timing** (3 inputs example):
 * ```
 * Time:     0    1    2    3    4    5
 * Input1:   ─────X────────X─────────X
 * Input2:   ──────────X────────X────
 * Input3:   ───────X─────────X──────
 * Output:   ─────X─X──X────X─X─X────X
 *           (fires when any ready)
 * ```
 *
 * **Common Use Cases**:
 *
 * 1. **Multi-Camera Synchronization**:
 *    ```
 *    Camera1 → Preprocess1 ─┐
 *    Camera2 → Preprocess2 ─┼─[AND]─→ MultiViewProcessing
 *    Camera3 → Preprocess3 ─┘
 *    ```
 *
 * 2. **Parallel Processing Coordination**:
 *    ```
 *    Image → Branch1 (EdgeDetection) ─┐
 *          → Branch2 (ColorAnalysis)  ├─[AND]─→ CombineResults
 *          → Branch3 (TextureExtract) ┘
 *    ```
 *
 * 3. **Redundant Source Management**:
 *    ```
 *    PrimaryCamera ──┐
 *    BackupCamera1 ──┼─[OR]─→ Processing (use first available)
 *    BackupCamera2 ──┘
 *    ```
 *
 * 4. **Complex Event Trigger**:
 *    ```
 *    MotionDetector ──┐
 *    AudioTrigger ────┼─[OR]─→ StartRecording
 *    ManualTrigger ───┘
 *    ```
 *
 * **Dynamic Input Configuration**:
 * - Input count can be changed at runtime via:
 *   * Embedded widget spin box
 *   * Property browser "Input Size" field
 * - Ports are added/removed dynamically
 * - Node geometry updates automatically
 * - Minimum 2 inputs, maximum 10 inputs
 *
 * **Reset Functionality**:
 * - Reset button clears all internal ready states and sync values
 * - Useful when restarting pipeline or recovering from errors
 * - Does not change input count or operation mode
 *
 * **Performance**:
 * - Overhead: Negligible (simple boolean logic with O(N) complexity)
 * - Latency: Instant signal combination
 * - Suitable for real-time applications
 * - Efficient even with maximum 10 inputs
 *
 * **Design Decision**:
 * Default AND mode encourages proper synchronization in multi-source pipelines,
 * preventing partial processing with incomplete data. OR mode requires explicit
 * selection, ensuring users understand the first-available semantics. Variable
 * input size provides flexibility without requiring multiple node types.
 *
 * @see SyncData for synchronization signal data type
 */
class CombineSyncModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CombineSyncModel with AND operation default.
     */
    CombineSyncModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CombineSyncModel() override {}

    /**
     * @brief Serializes model state to JSON.
     * @return QJsonObject containing combine operation mode
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p JSON object with saved state
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports for the specified type.
     * @param portType Input or Output
     * @return Variable number for Input (minimum 2), 1 for Output (combined sync)
     */
    unsigned int
    nPorts( PortType portType ) const override;

    /**
     * @brief Returns the data type for the specified port.
     * @param portType Input or Output
     * @param portIndex Port index
     * @return SyncData for all ports
     */
    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Returns the output data (combined sync signal).
     * @param port Output port index (0)
     * @return Shared pointer to SyncData
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and evaluates combination logic.
     * @param nodeData Input sync signal (port 0 or 1)
     * @param port Input port index
     *
     * Evaluates the combination logic (AND/OR) and triggers output if condition met.
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    /**
     * @brief Returns the embedded widget (operation selector and input size control).
     * @return QWidget containing combo box for AND/OR selection and spin box for input count
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    bool
    resizable() const override { return false; }

    /**
     * @brief Updates model properties from the property browser.
     * @param property Property name (e.g., "operation")
     * @param value New property value
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;    ///< Node category: "Utility"
    static const QString _model_name;  ///< Unique model name: "Combine Sync"

private Q_SLOTS:
    /**
     * @brief Handles combo box selection change.
     * @param operation New operation mode ("AND" or "OR")
     *
     * Updates the combine condition and re-evaluates the logic.
     */
    void
    combine_operation_changed(const QString & operation);

    /**
     * @brief Handles input size spin box value change.
     * @param size New number of input ports (minimum 2)
     *
     * Updates the number of input ports dynamically.
     */
    void
    input_size_changed(int size);

    /**
     * @brief Handles reset button click.
     * 
     * Resets all ready states and sync values to false.
     */
    void
    reset_clicked();

private:

    CombineSyncEmbeddedWidget* mpEmbeddedWidget;    ///< Embedded widget container
    CombineCondition mCombineCondition{ CombineCondition_AND }; ///< Current combination mode
    unsigned int miInputSize { 2 };                 ///< Number of input ports (minimum 2)
    std::vector<bool> mvbReady;                     ///< Ready state for each input
    std::vector<bool> mvSyncValues;                 ///< Stored sync values from inputs
    std::shared_ptr< SyncData > mpSyncData;         ///< Output combined sync signal
};

