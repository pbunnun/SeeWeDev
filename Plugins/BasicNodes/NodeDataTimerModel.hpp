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
 * @file NodeDataTimerModel.hpp
 * @brief Periodic trigger generator for timed pipeline execution.
 *
 * This node generates periodic sync signals at configurable intervals, acting as
 * a heartbeat or clock source for pipelines. It's used to trigger processing at
 * fixed rates, create time-lapse captures, or pace data flow through the graph.
 *
 * **Key Features:**
 * - Configurable timer interval (milliseconds)
 * - Start/Stop controls via embedded widget
 * - Periodic sync signal output
 * - Qt QTimer-based implementation (reliable timing)
 *
 * **Typical Use Cases:**
 * - Fixed-rate image capture (time-lapse)
 * - Periodic processing trigger (every N seconds)
 * - Pipeline pacing and flow control
 * - Periodic data acquisition
 * - Frame rate limiting
 *
 * @see NodeDataTimerEmbeddedWidget for Start/Stop UI
 * @see QTimer for underlying timer mechanism
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "NodeDataTimerEmbeddedWidget.hpp"


using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class NodeDataTimerModel
 * @brief Generates periodic sync signals at configurable intervals.
 *
 * NodeDataTimerModel acts as a clock/heartbeat generator for pipelines, emitting
 * sync signals at regular intervals specified in milliseconds. It provides Start/Stop
 * controls via embedded widget and uses Qt's QTimer for reliable timing.
 *
 * **Port Configuration:**
 * - **Inputs:** None (timer is autonomous)
 * - **Output:** NodeData (SyncData) - Periodic trigger signal
 *
 * **Embedded Widget:**
 * - Interval input (milliseconds)
 * - Start button (begin periodic triggering)
 * - Stop button (halt triggering)
 * - Status indicator (running/stopped)
 *
 * **Timer Operation:**
 * ```cpp
 * QTimer *timer = new QTimer();
 * timer->setInterval(interval_ms);  // e.g., 1000 for 1Hz
 * connect(timer, &QTimer::timeout, this, &NodeDataTimerModel::em_timeout);
 * timer->start();  // Begin periodic triggering
 *
 * // On each timeout:
 * void em_timeout() {
 *     mpNodeData = std::make_shared<SyncData>();  // Generate sync signal
 *     emit dataUpdated(0);  // Propagate downstream
 * }
 * ```
 *
 * **Common Use Cases:**
 *
 * 1. **Time-Lapse Capture:**
 *    ```
 *    Timer(5000ms) → Camera → SaveImage
 *    // Capture photo every 5 seconds
 *    ```
 *
 * 2. **Periodic Processing:**
 *    ```
 *    Timer(1000ms) → ImageLoader → Process → Display
 *    // Process one image per second
 *    ```
 *
 * 3. **Frame Rate Limiting:**
 *    ```
 *    Timer(33ms) → VideoPlayer → Display
 *    // Limit playback to ~30 FPS
 *    ```
 *
 * 4. **Multi-Camera Synchronization:**
 *    ```
 *    Timer(100ms) → CombineSync ┬→ Camera1
 *                                ├→ Camera2
 *                                └→ Camera3
 *    // Trigger all cameras simultaneously every 100ms
 *    ```
 *
 * 5. **Periodic Monitoring:**
 *    ```
 *    Timer(60000ms) → Sensor → Log
 *    // Sample sensor every minute
 *    ```
 *
 * 6. **Animation/Slideshow:**
 *    ```
 *    Timer(2000ms) → ImageSequence → Display
 *    // Show next image every 2 seconds
 *    ```
 *
 * **Interval Configuration:**
 * - **Units**: Milliseconds (1000ms = 1 second)
 * - **Minimum**: Typically 1ms (actual minimum depends on OS timer resolution)
 * - **Maximum**: No hard limit (practical range: 1ms to hours)
 * - **Precision**: ±few milliseconds (OS-dependent, not real-time)
 *
 * **Timing Accuracy:**
 * - **Ideal**: Interval = 1000ms → triggers at t=0, 1000, 2000, 3000ms
 * - **Reality**: ±5-15ms jitter typical (Qt event loop, OS scheduling)
 * - **Long-term Drift**: Minimal (QTimer adjusts for accumulated error)
 * - **Not Suitable For**: Hard real-time requirements (use RTOS for <1ms precision)
 *
 * **Performance Characteristics:**
 * - Overhead: Negligible (<0.1ms per trigger)
 * - CPU Usage: Minimal when idle (event-driven)
 * - Memory: Small constant footprint (one QTimer instance)
 * - Thread Safety: Runs in main Qt event loop
 *
 * **Start/Stop Behavior:**
 * ```
 * State: Stopped
 *   - No timer running
 *   - No output generated
 *   - Click Start → Begin periodic triggering
 *
 * State: Running
 *   - Timer active
 *   - Sync signal emitted every interval
 *   - Click Stop → Halt triggering, reset state
 * ```
 *
 * **Use with Single-Shot Nodes:**
 * Many nodes (e.g., CVCamera) switch to single-shot mode when sync input connected:
 * ```
 * Timer → CVCamera (single-shot mode)
 * // Camera captures one frame per timer trigger
 * // vs. continuous streaming without timer
 * ```
 *
 * **Design Rationale:**
 * - Qt QTimer provides cross-platform reliable timing
 * - Event-driven (no polling) for efficiency
 * - Embedded widget for interactive control (no rebuild required)
 * - Outputs generic NodeData (compatible with all input types)
 *
 * **Limitations:**
 * - Not suitable for hard real-time (<1ms jitter) applications
 * - Timer runs in UI thread (very long processing can delay triggers)
 * - No phase synchronization between multiple timers
 * - Interval changes require stop/start cycle
 *
 * **Comparison with Other Timing Methods:**
 * - **vs. Camera FPS**: Timer provides stable rate independent of camera
 * - **vs. While Loops**: Timer is event-driven (no CPU spinning)
 * - **vs. External Clock**: Timer is software-based (no hardware required)
 *
 * @note For <10ms intervals, verify your OS timer resolution (may be 10-15ms minimum)
 * @note For precise synchronization, use hardware triggers or RTOS
 * @see NodeDataTimerEmbeddedWidget for UI controls
 * @see QTimer for Qt timer documentation
 */
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.


class NodeDataTimerModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    NodeDataTimerModel();

    virtual
    ~NodeDataTimerModel() override {}

    QJsonObject
    save() const override;

    void
    load(const QJsonObject &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    QPixmap
    minPixmap() const override { return _minPixmap; }


    static const QString _category;

    static const QString _model_name;

private Q_SLOTS :

    /**
     * @brief Handles timer timeout events and emits sync signal.
     * Called periodically by QTimer at configured interval.
     */
    void em_timeout();

private:

    std::shared_ptr<NodeData> mpNodeData { nullptr };   ///< Output sync signal
    NodeDataTimerEmbeddedWidget* mpEmbeddedWidget;      ///< Start/Stop controls

    QPixmap _minPixmap;  ///< Node icon

};

