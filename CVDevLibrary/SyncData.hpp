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
 * @file SyncData.hpp
 * @brief Synchronization signal data type for dataflow coordination.
 *
 * This file defines the SyncData class, which provides synchronization signals
 * for coordinating execution timing between nodes in the CVDev dataflow system.
 *
 * **Key Features:**
 * - **Timing Coordination:** Synchronizes node execution sequences
 * - **Event Triggering:** Signals when processing should occur
 * - **Timestamping:** Automatic timestamp tracking for sync events
 * - **State Tracking:** Active/Inactive state representation
 *
 * **Primary Purpose:**
 * SyncData acts as a timing signal or trigger that coordinates when nodes
 * should execute. Unlike regular data that carries information, SyncData
 * primarily carries timing information - "when" rather than "what".
 *
 * **Common Use Cases:**
 * - Manual triggering via push button nodes
 * - Timer-based periodic execution
 * - Frame-by-frame video processing control
 * - Sequential processing coordination
 * - Event-driven processing pipelines
 * - Gating/enabling processing branches
 *
 * **Dataflow Patterns:**
 * @code
 * // Manual trigger pattern
 * PushButton → [SyncData] → ProcessingNode → Result
 * 
 * // Timer-based periodic execution
 * TimerNode → [SyncData] → CameraCapture → [Image] → Display
 * 
 * // Synchronized multi-stage processing
 * TriggerNode → [SyncData] → Stage1 → [SyncData] → Stage2
 * 
 * // Gated execution
 * EnableButton → [SyncData] → SyncGate → ProcessingChain
 * @endcode
 *
 * **Timing Mechanism:**
 * Each SyncData instance carries a timestamp (from InformationData base class).
 * Nodes can use this timestamp to:
 * - Detect new sync signals
 * - Avoid reprocessing on same signal
 * - Measure timing between events
 * - Implement frame-rate control
 *
 * **State Semantics:**
 * - **Active (true):** Processing should proceed
 * - **Inactive (false):** Default/idle state
 * - State changes with set_data() update the timestamp
 *
 * **Design Philosophy:**
 * SyncData represents "execute now" signals rather than boolean logic.
 * It's about timing coordination, not true/false decisions. For boolean
 * logic, use BoolData instead.
 *
 * @see InformationData for timestamp functionality
 * @see BoolData for boolean logic data
 * @see TimerModel for sync generation
 * @see SyncGateModel for sync-based gating
 */

#pragma once

#include <QtNodes/NodeData>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class SyncData
 * @brief Synchronization signal container for dataflow timing coordination.
 *
 * Encapsulates a synchronization signal with automatic timestamping for
 * coordinating execution timing between nodes in the CVDev dataflow system.
 *
 * **Data Properties:**
 * - **Type Name:** "Sync"
 * - **Display Name:** "Syc"
 * - **Storage:** bool (Active/Inactive)
 * - **Timestamping:** Automatic on state changes
 * - **Purpose:** Timing coordination, not data transmission
 *
 * **Construction Examples:**
 * @code
 * // Default constructor (inactive state)
 * auto sync1 = std::make_shared<SyncData>();
 * 
 * // Create active sync signal
 * auto sync2 = std::make_shared<SyncData>(true);
 * 
 * // Trigger a sync event
 * auto triggerSync = std::make_shared<SyncData>();
 * triggerSync->set_data(true);  // Updates timestamp
 * @endcode
 *
 * **Typical Usage in Nodes:**
 * @code
 * // Receiving sync signals
 * void MyNode::setInData(std::shared_ptr<NodeData> data, PortIndex port) {
 *     if (auto syncData = std::dynamic_pointer_cast<SyncData>(data)) {
 *         // Check if it's a new sync signal by comparing timestamps
 *         if (syncData->timestamp() != mLastSyncTimestamp) {
 *             mLastSyncTimestamp = syncData->timestamp();
 *             // Process on this new sync signal
 *             processData();
 *         }
 *     }
 * }
 * 
 * // Generating sync signals
 * void TimerNode::onTimeout() {
 *     auto syncData = std::make_shared<SyncData>(true);
 *     // Timestamp is set automatically in constructor via set_data()
 *     emit dataUpdated(0);  // Notify connected nodes
 * }
 * @endcode
 *
 * **State vs Timestamp:**
 * - **State (bool):** Active/Inactive indicator
 * - **Timestamp:** When the sync signal was created/updated
 * - **Key Insight:** The timestamp is often more important than the state
 *
 * **Timestamp-Based Triggering:**
 * @code
 * class ImageProcessor : public PBNodeDelegateModel {
 *     qint64 mLastSyncTime = 0;
 *     
 *     void handleSync(std::shared_ptr<SyncData> sync) {
 *         qint64 currentTime = sync->timestamp();
 *         if (currentTime != mLastSyncTime) {
 *             mLastSyncTime = currentTime;
 *             // New sync signal - process image
 *             processImage();
 *         }
 *         // Ignore if timestamp unchanged (same signal)
 *     }
 * };
 * @endcode
 *
 * **String Representation:**
 * - Active state → "Active"
 * - Inactive state → "Inacive" [sic - note typo in original]
 *
 * **Information Display:**
 * The set_information() method generates:
 * @code
 * Data Type : Sync
 * Active
 * @endcode
 *
 * **Common Patterns:**
 * @code
 * // Manual trigger (push button)
 * void PushButtonModel::onButtonClicked() {
 *     mpSyncData = std::make_shared<SyncData>(true);
 *     dataUpdated(0);
 * }
 * 
 * // Periodic timer
 * void TimerModel::onTimerTick() {
 *     auto sync = std::make_shared<SyncData>(true);
 *     emit dataUpdated(0);
 * }
 * 
 * // Synchronized multi-input
 * void SyncGateModel::checkInputs() {
 *     if (allInputsReady()) {
 *         auto outputSync = std::make_shared<SyncData>(true);
 *         emit dataUpdated(0);
 *     }
 * }
 * @endcode
 *
 * **Frame-Rate Control:**
 * @code
 * // Video processing at controlled rate
 * TimerNode(30 FPS) → [SyncData] → VideoCaptureNode → ProcessingChain
 * 
 * // Each sync triggers one frame capture and processing
 * @endcode
 *
 * **SyncData vs BoolData:**
 * - **SyncData:** For timing/triggering ("execute now")
 * - **BoolData:** For logic/state ("this is true/false")
 * - SyncData emphasizes timestamp over state value
 * - BoolData emphasizes state value over timestamp
 *
 * **Best Practices:**
 * - Always create new SyncData instance for each trigger
 * - Use timestamp comparison to detect new signals
 * - Don't reuse SyncData instances across multiple triggers
 * - Cache previous timestamp to avoid reprocessing
 *
 * **Performance Considerations:**
 * - Creating new SyncData is lightweight (just bool + timestamp)
 * - Shared pointers enable efficient passing
 * - Timestamp comparison is fast (integer comparison)
 *
 * @note State defaults to false (inactive), not true.
 * @note String representation has typo "Inacive" (should be "Inactive").
 * @note Timestamp from InformationData is the primary mechanism.
 *
 * @see InformationData for timestamp functionality
 * @see BoolData for boolean logic
 * @see TimerModel for periodic sync generation
 * @see SyncGateModel for multi-input synchronization
 */
class SyncData : public InformationData
{
public:
    /**
     * @brief Default constructor creating inactive sync signal.
     *
     * Creates a SyncData instance with inactive (false) state.
     * Timestamp is set to current time via base class.
     */
    SyncData()
        : mbSync(false)
    {
    }

    /**
     * @brief Constructor with initial state.
     * @param state Initial sync state (true = active, false = inactive).
     *
     * Creates a SyncData instance with the specified state.
     * Typically used with true to create active sync signals.
     *
     * **Example:**
     * @code
     * // Create active sync trigger
     * auto sync = std::make_shared<SyncData>(true);
     * @endcode
     */
    SyncData( const bool state )
        : mbSync(state)
    {}

    /**
     * @brief Returns the data type information.
     * @return NodeDataType with name "Sync" and display "Syc".
     *
     * Provides type identification for the node system's type checking
     * and connection validation. Sync ports only connect to other Sync ports.
     */
    NodeDataType
    type() const override
    {
        return { "Sync", "Syc" };
    }

    /**
     * @brief Returns a reference to the sync state.
     * @return Reference to the internal bool state.
     *
     * Provides direct access to the sync state. Modifying through
     * this reference does NOT update the timestamp.
     *
     * **Warning:** Direct modification bypasses timestamp updates.
     * Use set_data() when creating new sync signals.
     *
     * @code
     * bool isActive = sync->data();  // Read
     * sync->data() = true;           // Write (no timestamp update!)
     * @endcode
     */
    bool&
    data()
    {
        return mbSync;
    }

    /**
     * @brief Sets the sync state and updates timestamp.
     * @param data New sync state.
     *
     * Updates the stored state and automatically updates the timestamp
     * from the InformationData base class. This is the preferred method
     * for creating new sync signals.
     *
     * **Typical Usage:**
     * @code
     * // Generate new sync trigger
     * auto syncData = std::make_shared<SyncData>();
     * syncData->set_data(true);  // Sets state and updates timestamp
     * emit dataUpdated(0);       // Notify connected nodes
     * @endcode
     */
    void
    set_data( bool data )
    {
        mbSync = data;
        InformationData::set_timestamp();
    }

    /**
     * @brief Returns string representation of the sync state.
     * @return QString "Active" if true, "Inacive" if false.
     *
     * Provides a human-readable string representation for UI display,
     * logging, or debugging.
     *
     * @note Contains typo: "Inacive" instead of "Inactive".
     *
     * @code
     * SyncData sync(true);
     * qDebug() << sync.state_str();  // Outputs: "Active"
     * 
     * SyncData inactive(false);
     * qDebug() << inactive.state_str();  // Outputs: "Inacive"
     * @endcode
     */
    QString
    state_str() const
    {
        return mbSync? QString("Active") : QString("Inacive");
    }

    /**
     * @brief Generates formatted information string.
     *
     * Creates a human-readable string representation of the sync signal
     * for display in debug views or information panels.
     *
     * **Format:**
     * @code
     * Data Type : Sync
     * <Active|Inacive>
     * @endcode
     *
     * Example outputs:
     * @code
     * Data Type : Sync
     * Active
     * @endcode
     *
     * @code
     * Data Type : Sync
     * Inacive
     * @endcode
     */
    void set_information() override
    {
        mQSData = QString("Data Type : Sync \n");
        mQSData += state_str();
        mQSData += QString("\n");
    }

private:
    /**
     * @brief The stored sync state.
     *
     * Internal storage for the synchronization state.
     * - true: Active sync signal
     * - false: Inactive state
     *
     * Access through data() or set_data() methods.
     * The timestamp (from InformationData) is often more important
     * than this state value for sync coordination.
     */
    bool mbSync;

};
