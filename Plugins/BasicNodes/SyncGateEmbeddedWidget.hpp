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
 * @file SyncGateEmbeddedWidget.hpp
 * @brief Embedded widget for configuring synchronization gate port routing.
 *
 * This file defines the SyncGateEmbeddedWidget class, which provides checkboxes for
 * enabling/disabling individual input and output ports on the SyncGateModel node.
 * This allows dynamic port configuration for flexible synchronization signal routing
 * based on logical operations.
 */

#pragma once

#include <QWidget>

namespace Ui {
class SyncGateEmbeddedWidget;
}

/**
 * @class SyncGateEmbeddedWidget
 * @brief Widget for configuring synchronization gate input/output port states.
 *
 * This widget provides four independent checkboxes that control the enabled state
 * of the two input ports and two output ports on the SyncGateModel node. This allows
 * users to dynamically configure which ports are active, enabling flexible routing
 * of synchronization and boolean signals through logical operations.
 *
 * **Key Features:**
 * - Independent enable/disable for each of 2 input and 2 output ports
 * - Real-time port configuration without node recreation
 * - Qt version compatibility (handles Qt 6.7+ checkStateChanged signature change)
 * - Signal emission for port state changes
 *
 * **Port Configuration:**
 * - **Input Port 0:** First sync/bool input (can be disabled)
 * - **Input Port 1:** Second sync/bool input (can be disabled)
 * - **Output Port 0:** First sync/bool output (can be disabled)
 * - **Output Port 1:** Second sync/bool output (can be disabled)
 *
 * **Use Cases:**
 * - Single-input operations (disable one input port)
 * - Single-output operations (disable one output port)
 * - Simplified logical gates (AND with one output)
 * - Custom routing configurations
 * - UI simplification by hiding unused ports
 *
 * **Typical Configurations:**
 * @code
 * // AND gate with single output
 * In0: Enabled, In1: Enabled, Out0: Enabled, Out1: Disabled
 * 
 * // DIRECT pass-through
 * In0: Enabled, In1: Disabled, Out0: Enabled, Out1: Disabled
 * 
 * // Dual outputs from same logic
 * In0: Enabled, In1: Enabled, Out0: Enabled, Out1: Enabled
 * @endcode
 *
 * @see SyncGateModel
 */
class SyncGateEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a SyncGateEmbeddedWidget.
     * @param parent Parent widget (typically nullptr for embedded widgets).
     *
     * Initializes the four port control checkboxes (typically all enabled by default).
     */
    explicit SyncGateEmbeddedWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor.
     *
     * Cleans up UI components allocated during construction.
     */
    ~SyncGateEmbeddedWidget();

    /**
     * @brief Gets the state of input port 0 checkbox.
     * @return true if enabled, false if disabled.
     */
    bool get_in0_Checkbox() const;
    
    /**
     * @brief Gets the state of input port 1 checkbox.
     * @return true if enabled, false if disabled.
     */
    bool get_in1_Checkbox() const;
    
    /**
     * @brief Gets the state of output port 0 checkbox.
     * @return true if enabled, false if disabled.
     */
    bool get_out0_Checkbox() const;
    
    /**
     * @brief Gets the state of output port 1 checkbox.
     * @return true if enabled, false if disabled.
     */
    bool get_out1_Checkbox() const;
    
    /**
     * @brief Sets the state of input port 0 checkbox.
     * @param state true to enable, false to disable.
     */
    void set_in0_Checkbox(const bool state);
    
    /**
     * @brief Sets the state of input port 1 checkbox.
     * @param state true to enable, false to disable.
     */
    void set_in1_Checkbox(const bool state);
    
    /**
     * @brief Sets the state of output port 0 checkbox.
     * @param state true to enable, false to disable.
     */
    void set_out0_Checkbox(const bool state);
    
    /**
     * @brief Sets the state of output port 1 checkbox.
     * @param state true to enable, false to disable.
     */
    void set_out1_Checkbox(const bool state);

Q_SIGNALS :
    /**
     * @brief Signal emitted when any checkbox state changes.
     * @param checkbox Checkbox identifier (0=In0, 1=In1, 2=Out0, 3=Out1).
     * @param state New state (Qt::Checked or Qt::Unchecked as int).
     *
     * This signal allows the parent model to reconfigure ports dynamically
     * when the user changes any checkbox state.
     */
    void checkbox_checked_signal(int checkbox, int state);

private Q_SLOTS :
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    /**
     * @brief Slot for input port 0 checkbox state changes (Qt < 6.7).
     * @param arg1 New state as int.
     */
    void in0_checkbox_state_changed(int arg1);
    
    /**
     * @brief Slot for input port 1 checkbox state changes (Qt < 6.7).
     * @param arg1 New state as int.
     */
    void in1_checkbox_state_changed(int arg1);
    
    /**
     * @brief Slot for output port 0 checkbox state changes (Qt < 6.7).
     * @param arg1 New state as int.
     */
    void out0_checkbox_state_changed(int arg1);
    
    /**
     * @brief Slot for output port 1 checkbox state changes (Qt < 6.7).
     * @param arg1 New state as int.
     */
    void out1_checkbox_state_changed(int arg1);
#else
    /**
     * @brief Slot for input port 0 checkbox state changes (Qt >= 6.7).
     * @param arg1 New state as Qt::CheckState.
     */
    void in0_checkbox_check_state_changed(Qt::CheckState arg1);
    
    /**
     * @brief Slot for input port 1 checkbox state changes (Qt >= 6.7).
     * @param arg1 New state as Qt::CheckState.
     */
    void in1_checkbox_check_state_changed(Qt::CheckState arg1);
    
    /**
     * @brief Slot for output port 0 checkbox state changes (Qt >= 6.7).
     * @param arg1 New state as Qt::CheckState.
     */
    void out0_checkbox_check_state_changed(Qt::CheckState arg1);
    
    /**
     * @brief Slot for output port 1 checkbox state changes (Qt >= 6.7).
     * @param arg1 New state as Qt::CheckState.
     */
    void out1_checkbox_check_state_changed(Qt::CheckState arg1);
#endif

private:
    Ui::SyncGateEmbeddedWidget *ui; ///< UI components generated by Qt Designer
};

