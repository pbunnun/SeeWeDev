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
 * @file TimerModel.hpp
 * @brief Model for periodic synchronization signal generation.
 *
 * This file defines the TimerModel class, which generates periodic sync signals at
 * configurable intervals using QTimer. It's essential for triggering time-based operations,
 * frame rate limiting, periodic sampling, and scheduled task execution in automated workflows.
 */

#pragma once

#include <QTimer>
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class TimerModel
 * @brief Node model for generating periodic synchronization signals.
 *
 * This model generates sync signals at regular intervals using Qt's QTimer, enabling
 * time-based triggering of downstream operations. It's controlled by the node's
 * enable/disable state and provides a simple, reliable timing source for automation.
 *
 * **Input Ports:**
 * None (signal generator/source node)
 *
 * **Output Ports:**
 * 1. **SyncData** - Periodic sync signal (fires at configured interval)
 *
 * **Operation:**
 * - When node is enabled: Timer starts and emits sync signals periodically
 * - When node is disabled: Timer stops, no signals emitted
 * - Interval configurable via millisecond_interval property
 * - Uses QTimer for accurate, non-blocking timing
 *
 * **Timing Mechanism:**
 * - QTimer emits timeout() signal at specified millisecond intervals
 * - timeout_function() slot creates and outputs SyncData
 * - Runs continuously while node is enabled
 * - Zero CPU overhead between timeouts (event-driven)
 *
 * **Properties (Configurable):**
 * - **millisecond_interval:** Time between sync signals in milliseconds
 *   - Default: 1000ms (1 second)
 *   - Range: 1ms to several hours (practical: 10ms to minutes)
 *   - Lower values = higher frequency, more overhead
 *
 * **Use Cases:**
 *
 * 1. **Frame Rate Limiting:**
 *    @code
 *    [Timer: 33ms] -> [Camera Capture] (30 fps)
 *    [Timer: 16ms] -> [Camera Capture] (60 fps)
 *    @endcode
 *
 * 2. **Periodic Image Saving:**
 *    @code
 *    [Timer: 60000ms] -> [SyncGate: AND] -> [SaveImage]
 *                              ^
 *    [Camera Frame Ready] ----/
 *    // Saves one frame per minute
 *    @endcode
 *
 * 3. **Regular Status Updates:**
 *    @code
 *    [Timer: 5000ms] -> [ImageProperties] -> [Display Info]
 *    // Shows image stats every 5 seconds
 *    @endcode
 *
 * 4. **Periodic Data Acquisition:**
 *    @code
 *    [Timer: 100ms] -> [Sensor Reader] -> [Data Logger]
 *    // Samples sensor at 10 Hz
 *    @endcode
 *
 * 5. **Heartbeat/Watchdog:**
 *    @code
 *    [Timer: 1000ms] -> [Heartbeat Display]
 *    // Visual confirmation system is running
 *    @endcode
 *
 * 6. **Scheduled Processing:**
 *    @code
 *    [Timer: 3600000ms] -> [Trigger Processing Pipeline]
 *    // Run analysis once per hour
 *    @endcode
 *
 * **Timing Accuracy:**
 * - QTimer accuracy: typically ±1-15ms depending on system load
 * - Not real-time: subject to OS scheduling
 * - Adequate for most automation tasks
 * - For sub-millisecond precision, use dedicated hardware timers
 *
 * **Enable/Disable Behavior:**
 * - **Enable:** Starts timer, begins emitting sync signals
 * - **Disable:** Stops timer, no more signals
 * - State change handled via enable_changed() slot
 * - Clean start/stop without resource leaks
 *
 * **Comparison with NodeDataTimerModel:**
 * - **TimerModel:** Simple periodic signal generation, always on when enabled
 * - **NodeDataTimerModel:** Full UI with start/stop buttons, countdown display,
 *   single-shot mode, user-interactive
 *
 * **Best Practices:**
 * 1. Set interval based on actual needs (avoid unnecessarily high rates)
 * 2. Disable when not needed to reduce system load
 * 3. Use SyncGate to combine with other conditions
 * 4. Consider downstream processing time when setting interval
 * 5. For variable timing, use NodeDataTimerModel instead
 *
 * **Performance Notes:**
 * - Negligible CPU usage when idle (event-driven)
 * - Each timeout creates small overhead
 * - Practical limits: 1000 Hz (1ms) on typical systems
 * - Higher rates may cause timing jitter
 *
 * **Example Configurations:**
 * @code
 * // 30 FPS frame capture
 * millisecond_interval: 33
 * 
 * // Once per second status update
 * millisecond_interval: 1000
 * 
 * // 10 Hz sensor sampling
 * millisecond_interval: 100
 * 
 * // Hourly scheduled task
 * millisecond_interval: 3600000
 * @endcode
 *
 * @see NodeDataTimerModel (interactive timer with UI)
 * @see QTimer
 * @see SyncData
 */
class TimerModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a TimerModel.
     *
     * Initializes the QTimer with default 1000ms interval (1 second).
     */
    TimerModel();

    /**
     * @brief Destructor.
     *
     * Stops the timer and frees allocated resources.
     */
    virtual
    ~TimerModel() override
    {
        if( mpTimer )
        {
            mpTimer->stop();
            delete mpTimer;
        }
    }

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing timer interval.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved interval.
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 0 for input (no inputs), 1 for output (sync signal).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index (0).
     * @return SyncData for output port.
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the sync signal data.
     * @param port Port index (0).
     * @return Shared pointer to SyncData.
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief No input data (no-op).
     * @param nodeData Unused.
     * @param Unused.
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override { }

    /**
     * @brief Returns nullptr (no embedded widget).
     * @return nullptr.
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets a model property.
     * @param Property name ("millisecond_interval").
     * @param QVariant value (integer milliseconds).
     *
     * Updates timer interval. If timer is running, restarts with new interval.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private Q_SLOTS:
    /**
     * @brief Slot for node enable/disable state changes.
     * @param Enabled state.
     *
     * Starts timer when enabled, stops when disabled.
     */
    void
    enable_changed( bool ) override;

    /**
     * @brief Slot called on each timer timeout.
     *
     * Creates a new SyncData and triggers data update, causing sync
     * signal to propagate to connected nodes.
     */
    void timeout_function();

private:
    QTimer * mpTimer;                                  ///< Qt timer for periodic triggering
    std::shared_ptr<SyncData> mpSyncData { nullptr };  ///< Output sync signal

    int miMillisecondInterval { 1000 };                ///< Timer interval in milliseconds
};

