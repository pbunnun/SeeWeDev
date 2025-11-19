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
 * @file TemplateEmbeddedWidget.hpp
 * @brief Template widget demonstrating standard node UI controls.
 *
 * This file defines the TemplateEmbeddedWidget class, which serves as a reference
 * implementation for creating custom embedded widgets in nodes. It showcases common
 * UI elements including combo boxes, spin boxes, buttons, and text displays, providing
 * a foundation for developers creating new node types.
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QWidget>
#include <QSpinBox>

namespace Ui {
class TemplateEmbeddedWidget;
}

/**
 * @class TemplateEmbeddedWidget
 * @brief Reference template widget demonstrating common UI control patterns.
 *
 * This widget serves as a template and reference implementation for creating custom
 * embedded widgets in node models. It demonstrates best practices for common UI
 * controls including combo boxes, spin boxes, start/stop buttons, and dynamic text
 * displays.
 *
 * **Key Features:**
 * - Combo box for option selection
 * - Spin box for numeric value input
 * - Start/Stop/Send button controls
 * - Dynamic text display
 * - State management (active/inactive)
 * - Signal emission for user interactions
 *
 * **UI Components:**
 * - **Combo Box:** Dropdown selection for modes or options
 * - **Spin Box:** Integer value input with increment/decrement
 * - **Start Button:** Initiates operation (becomes inactive when running)
 * - **Stop Button:** Terminates operation (active when running)
 * - **Send Button:** Triggers single action or data transmission
 * - **Display Label:** Shows status text or dynamic information
 *
 * **Typical Usage Patterns:**
 *
 * 1. **Mode Selection with Start/Stop:**
 *    - User selects mode from combo box
 *    - Adjusts parameter with spin box
 *    - Clicks Start to begin operation
 *    - Clicks Stop to end operation
 *
 * 2. **Value Configuration:**
 *    - Set numeric value via spin box
 *    - Select processing type via combo box
 *    - Click Send to apply changes
 *
 * **State Management:**
 * - Active state: Start disabled, Stop enabled
 * - Inactive state: Start enabled, Stop disabled
 * - Prevents conflicting operations
 *
 * **Use Cases (for Custom Widgets):**
 * - Video capture controls (start/stop recording)
 * - Algorithm mode selection (method + parameters)
 * - Communication interfaces (connect/disconnect)
 * - Periodic task configuration (interval + start/stop)
 * - Device control panels (mode + settings + action buttons)
 *
 * **Development Guide:**
 * When creating a new embedded widget, use this template to:
 * 1. Copy the UI structure
 * 2. Modify controls to match requirements
 * 3. Implement signal handlers for user interactions
 * 4. Update display elements based on model state
 * 5. Emit signals to notify parent model of changes
 *
 * @see TemplateModel
 */
class TemplateEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a TemplateEmbeddedWidget.
     * @param parent Parent widget (typically nullptr for embedded widgets).
     *
     * Initializes all UI controls with default values.
     */
    explicit TemplateEmbeddedWidget( QWidget *parent = nullptr );
    
    /**
     * @brief Destructor.
     *
     * Cleans up UI components allocated during construction.
     */
    ~TemplateEmbeddedWidget();

    /**
     * @brief Retrieves the list of combo box options.
     * @return QStringList containing all available options.
     */
    QStringList
    get_combobox_string_list();

    /**
     * @brief Gets a pointer to the spin box control.
     * @return const QSpinBox* pointer for reading value and properties.
     *
     * Allows direct access to spin box for advanced queries.
     */
    const QSpinBox *
    get_spinbox();

    /**
     * @brief Sets the combo box selection by text.
     * @param value Option text to select.
     *
     * Programmatically selects a combo box item. Used when loading
     * saved configurations.
     */
    void
    set_combobox_value( QString value );

    /**
     * @brief Sets the display label text.
     * @param value Text to display.
     *
     * Updates the dynamic text display area with status or information.
     *
     * **Example:**
     * @code
     * widget->set_display_text("Processing: Frame 145");
     * widget->set_display_text("Status: Ready");
     * @endcode
     */
    void
    set_display_text( QString value );

    /**
     * @brief Sets the spin box value.
     * @param value Integer value to set.
     *
     * Programmatically sets the spin box value.
     */
    void
    set_spinbox_value( int value );

    /**
     * @brief Sets the active state of start/stop buttons.
     * @param startButtonActive true to enable Start, false to enable Stop.
     *
     * Controls the enabled state of Start and Stop buttons to reflect
     * the operational state (running vs stopped).
     *
     * **Example:**
     * @code
     * widget->set_active_button(true);  // Enable Start (stopped state)
     * widget->set_active_button(false); // Enable Stop (running state)
     * @endcode
     */
    void
    set_active_button( bool startButtonActive );

    /**
     * @brief Gets the currently selected combo box text.
     * @return QString containing the selected option.
     */
    QString
    get_combobox_text();

Q_SIGNALS:
    /**
     * @brief Signal emitted when any button is clicked.
     * @param button Button identifier (0=Start, 1=Stop, 2=Send, or custom mapping).
     *
     * This unified signal allows the parent model to handle all button
     * clicks with a single slot, using the button parameter to distinguish
     * which action was triggered.
     */
    void
    button_clicked_signal( int button );

    /**
     * @brief Emitted when widget is resized.
     *
     * Notifies the model to update node bounding box.
     */
    void
    widget_resized_signal();

public Q_SLOTS:
    /**
     * @brief Slot for Start button click.
     *
     * Emits button_clicked_signal(0) and typically disables Start,
     * enables Stop button.
     */
    void
    start_button_clicked();

    /**
     * @brief Slot for Stop button click.
     *
     * Emits button_clicked_signal(1) and typically enables Start,
     * disables Stop button.
     */
    void
    stop_button_clicked();

    /**
     * @brief Slot for spin box value changes.
     * @param New spin box value.
     *
     * Handles spin box value changes, allowing widget or model
     * to respond to parameter adjustments.
     */
    void
    spin_box_value_changed( int );

    /**
     * @brief Slot for combo box selection changes.
     * @param New selection index.
     *
     * Handles combo box selection changes, allowing dynamic
     * reconfiguration based on selected option.
     */
    void
    combo_box_current_index_changed( int );

    /**
     * @brief Slot for Send button click.
     *
     * Emits button_clicked_signal(2) for single-action triggers
     * or data transmission.
     */
    void
    send_button_clicked();

protected:
    /**
     * @brief Handles widget resize events.
     * @param event Resize event details
     *
     * Emits widget_resized_signal() to notify model of geometry changes.
     */
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::TemplateEmbeddedWidget *ui; ///< UI components generated by Qt Designer
};