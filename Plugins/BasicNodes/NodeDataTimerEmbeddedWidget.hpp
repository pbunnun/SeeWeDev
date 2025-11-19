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
 * @file NodeDataTimerEmbeddedWidget.hpp
 * @brief Embedded widget for configurable timer control with visual feedback.
 *
 * This file defines the NodeDataTimerEmbeddedWidget class, which provides a comprehensive
 * timer interface with dual time units (seconds and milliseconds), periodic/single-shot modes,
 * and real-time countdown display. The widget is designed for precise timing control in
 * data flow automation, synchronization, and scheduled event triggering.
 */

#pragma once

#include <QDebug>
#include <QWidget>
#include <QTimer>

namespace Ui {
class NodeDataTimerEmbeddedWidget;
}

/**
 * @class NodeDataTimerEmbeddedWidget
 * @brief Configurable timer widget with periodic and single-shot modes.
 *
 * This widget provides a sophisticated timer interface for controlling timed events in
 * the node graph. It offers dual time unit controls (seconds + milliseconds), periodic
 * or single-shot operation modes, start/stop/reset functionality, and real-time countdown
 * display for monitoring timer status.
 *
 * **Key Features:**
 * - Dual time inputs: Seconds (0-3600) and Milliseconds (0-999)
 * - Operation modes: Periodic (repeating) or Single-shot (one-time)
 * - Control buttons: Start, Stop, Reset
 * - Real-time countdown display showing remaining time
 * - Visual feedback of timer state (running/stopped)
 * - Integrated QTimer for accurate event triggering
 *
 * **Timer Modes:**
 * - **Periodic Mode:** Timer fires repeatedly at specified intervals
 *   - Use case: Regular frame capture, periodic sampling, heartbeat signals
 *   - Example: Trigger every 5 seconds for temperature logging
 * - **Single-shot Mode:** Timer fires once after delay, then stops
 *   - Use case: Delayed execution, timeout detection, scheduled one-time events
 *   - Example: Wait 2 seconds before starting image processing
 *
 * **Time Calculation:**
 * Total interval (ms) = (Seconds × 1000) + Milliseconds
 *
 * **Workflow:**
 * 1. User configures seconds and milliseconds spinboxes
 * 2. User selects Periodic or Single-shot mode
 * 3. User clicks Start button
 * 4. Timer begins countdown, updating remaining time display
 * 5. On timeout, widget emits timeout_signal()
 * 6. In periodic mode: timer resets and continues
 * 7. In single-shot mode: timer stops
 * 8. User can Stop or Reset at any time
 *
 * **State Management:**
 * - Start button: Enabled when stopped, disabled when running
 * - Stop button: Enabled when running, disabled when stopped
 * - Reset button: Always enabled, stops timer and resets to configured duration
 * - Spinboxes: Disabled when running to prevent mid-execution changes
 *
 * **Use Cases:**
 * - Frame rate limiting for video processing
 * - Periodic data acquisition from sensors
 * - Scheduled image capture or saving
 * - Timeout detection for operations
 * - Synchronized multi-node triggering
 * - Delayed execution sequences
 *
 * **Example Configurations:**
 * @code
 * // 5-second periodic timer
 * widget->set_second_spinbox(5);
 * widget->set_millisecond_spinbox(0);
 * widget->set_pf_combobox(0); // Periodic
 * 
 * // 250ms single-shot delay
 * widget->set_second_spinbox(0);
 * widget->set_millisecond_spinbox(250);
 * widget->set_pf_combobox(1); // Single-shot
 * @endcode
 *
 * @see NodeDataTimerModel
 * @see QTimer
 */
class NodeDataTimerEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a NodeDataTimerEmbeddedWidget.
     * @param parent Parent widget (typically nullptr for embedded widgets).
     *
     * Initializes the timer widget with default settings (typically 0.5s periodic mode),
     * creates the internal QTimer, and sets up UI connections.
     */
    explicit NodeDataTimerEmbeddedWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor.
     *
     * Stops the timer if running and cleans up allocated resources including QTimer.
     */
    ~NodeDataTimerEmbeddedWidget();

    /**
     * @brief Gets the configured seconds value.
     * @return Number of seconds (0-3600).
     */
    int get_second_spinbox() const;
    
    /**
     * @brief Gets the configured milliseconds value.
     * @return Number of milliseconds (0-999).
     */
    int get_millisecond_spinbox() const;
    
    /**
     * @brief Gets the selected timer mode.
     * @return Mode index: 0 = Periodic, 1 = Single-shot.
     */
    int get_pf_combobox() const;
    
    /**
     * @brief Gets the current remaining time.
     * @return Remaining time in seconds (floating-point for sub-second precision).
     *
     * Returns the time remaining until the next timeout event. Updated continuously
     * while the timer is running.
     */
    float get_remaining_time() const;
    
    /**
     * @brief Gets the start button enabled state.
     * @return true if start button is enabled, false otherwise.
     */
    bool get_start_button() const;
    
    /**
     * @brief Gets the stop button enabled state.
     * @return true if stop button is enabled, false otherwise.
     */
    bool get_stop_button() const;

    /**
     * @brief Sets the seconds spinbox value.
     * @param duration Number of seconds (0-3600).
     *
     * Programmatically sets the seconds component of the timer interval.
     * Used when loading saved configurations.
     */
    void set_second_spinbox(const int duration);
    
    /**
     * @brief Sets the milliseconds spinbox value.
     * @param duration Number of milliseconds (0-999).
     *
     * Programmatically sets the milliseconds component of the timer interval.
     */
    void set_millisecond_spinbox(const int duration);
    
    /**
     * @brief Sets the timer mode.
     * @param index Mode index: 0 = Periodic, 1 = Single-shot.
     *
     * Configures whether the timer repeats automatically or fires only once.
     */
    void set_pf_combobox(const int index);
    
    /**
     * @brief Sets the displayed remaining time.
     * @param time Remaining time in seconds.
     *
     * Updates the countdown display. Typically called internally during timer execution.
     */
    void set_remaining_time(const float time);
    
    /**
     * @brief Enables or disables the start button.
     * @param enable true to enable, false to disable.
     */
    void set_start_button(const bool enable);
    
    /**
     * @brief Enables or disables the stop button.
     * @param enable true to enable, false to disable.
     */
    void set_stop_button(const bool enable);

    /**
     * @brief Enables or disables all configuration controls.
     * @param enable true to enable, false to disable.
     *
     * Locks or unlocks the spinboxes and mode selector. Typically disabled
     * while the timer is running to prevent configuration changes mid-execution.
     */
    void set_widget_bundle(const bool enable);
    
    /**
     * @brief Updates mode-specific labels.
     * @param index Mode index (0 = Periodic, 1 = Single-shot).
     *
     * Adjusts UI labels to reflect the selected timer mode.
     */
    void set_pf_labels(const int index);
    
    /**
     * @brief Updates the remaining time label.
     * @param duration Remaining duration in seconds.
     *
     * Formats and displays the countdown timer in the UI.
     */
    void set_remaining_label(const float duration);

    /**
     * @brief Starts the internal QTimer.
     *
     * Initiates timer execution. For internal use by control slots.
     */
    void run() const;
    
    /**
     * @brief Stops the internal QTimer.
     *
     * Terminates timer execution. For internal use by control slots.
     */
    void terminate() const;

Q_SIGNALS :

    /**
     * @brief Signal emitted when the timer interval expires.
     *
     * This signal is emitted each time the timer reaches zero:
     * - In periodic mode: fires repeatedly at each interval
     * - In single-shot mode: fires once and timer stops
     *
     * Parent models connect to this signal to trigger timed operations.
     */
    void timeout_signal() const;

private Q_SLOTS :

    /**
     * @brief Slot for seconds spinbox value changes.
     * @param duration New seconds value.
     *
     * Updates the total interval when user changes seconds.
     */
    void second_spinbox_value_changed(int duration);
    
    /**
     * @brief Slot for milliseconds spinbox value changes.
     * @param duration New milliseconds value.
     *
     * Updates the total interval when user changes milliseconds.
     */
    void millisecond_spinbox_value_changed(int duration);
    
    /**
     * @brief Slot for timer mode selection changes.
     * @param index New mode index (0 = Periodic, 1 = Single-shot).
     *
     * Reconfigures the QTimer for periodic or single-shot behavior.
     */
    void pf_combo_box_current_index_changed(int index);
    
    /**
     * @brief Slot for start button click.
     *
     * Starts the timer, disables configuration controls, enables stop button.
     */
    void start_button_clicked();
    
    /**
     * @brief Slot for stop button click.
     *
     * Stops the timer, enables configuration controls, enables start button.
     */
    void stop_button_clicked();
    
    /**
     * @brief Slot for reset button click.
     *
     * Stops the timer and resets remaining time to configured duration.
     */
    void reset_button_clicked();

    /**
     * @brief Internal slot for single-shot timer completion.
     *
     * Handles cleanup after single-shot timer fires.
     */
    void on_singleShot();
    
    /**
     * @brief Internal slot for timer timeout events.
     *
     * Processes each timer tick, updates countdown display, emits timeout_signal().
     */
    void on_timeout();

private:
    Ui::NodeDataTimerEmbeddedWidget *ui; ///< UI components generated by Qt Designer
    int miInterval = 1;                   ///< Timer interval multiplier
    int miPeriod = 500;                   ///< Base period in milliseconds
    bool mbIsRunning = false;             ///< Timer running state flag

    QTimer* mpQTimer;                     ///< Internal Qt timer for event generation
};

