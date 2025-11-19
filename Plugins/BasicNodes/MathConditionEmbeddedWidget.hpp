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
 * @file MathConditionEmbeddedWidget.hpp
 * @brief Embedded widget for configuring mathematical conditional operations.
 *
 * This file defines the MathConditionEmbeddedWidget class, which provides a user interface
 * for selecting comparison operators and threshold values used in conditional logic operations.
 * The widget enables users to configure conditions such as "greater than," "less than,"
 * "equal to," etc., with a corresponding numeric threshold value.
 */

#ifndef MATHCONDITIONEMBEDDEDWIDGET_H
#define MATHCONDITIONEMBEDDEDWIDGET_H

#include <QWidget>
#include <QSpinBox>

namespace Ui {
class MathConditionEmbeddedWidget;
}

/**
 * @class MathConditionEmbeddedWidget
 * @brief Widget for configuring conditional comparison operations.
 *
 * This widget provides an interface for setting up conditional tests, combining a
 * comparison operator selection (combo box) with a numeric threshold input (line edit).
 * It is typically embedded in nodes that perform conditional logic, such as
 * MathConditionModel, to enable users to define conditions like "x > 5" or "y <= 100".
 *
 * **Key Components:**
 * - Condition combo box: Selects the comparison operator
 * - Number text field: Specifies the threshold value
 * - Signal emission: Notifies parent model when condition changes
 *
 * **Supported Comparison Operators:**
 * - Greater than (>)
 * - Greater than or equal (>=)
 * - Less than (<)
 * - Less than or equal (<=)
 * - Equal to (==)
 * - Not equal to (!=)
 *
 * **Use Cases:**
 * - Threshold-based filtering (e.g., "temperature > 30")
 * - Range validation (e.g., "value <= 255")
 * - Conditional branching in data flows
 * - Trigger detection (e.g., "pressure == 0")
 * - Quality control checks (e.g., "error_rate < 0.05")
 *
 * **Workflow:**
 * 1. User selects comparison operator from combo box
 * 2. User enters threshold number in text field
 * 3. Widget emits condition_changed_signal() with both values
 * 4. Parent model applies condition to incoming data
 * 5. Output reflects conditional test result (boolean or filtered data)
 *
 * **Example Configuration:**
 * @code
 * // Widget configured for "x > 100"
 * widget->set_condition_text_index(0);  // Greater than
 * widget->set_condition_number("100");
 * // Emits: condition_changed_signal(0, "100")
 * @endcode
 *
 * @see MathConditionModel
 */
class MathConditionEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a MathConditionEmbeddedWidget.
     * @param parent Parent widget (typically nullptr for embedded widgets).
     *
     * Initializes the UI with a combo box for operator selection and a line edit
     * for threshold value input. Default condition is typically "greater than 0".
     */
    explicit MathConditionEmbeddedWidget( QWidget *parent = nullptr );
    
    /**
     * @brief Destructor.
     *
     * Cleans up UI components allocated during construction.
     */
    ~MathConditionEmbeddedWidget();

    /**
     * @brief Retrieves the list of available comparison operators.
     * @return QStringList containing all operator options (e.g., ">", ">=", "<", "<=", "==", "!=").
     *
     * Returns the complete list of comparison operators available in the combo box.
     */
    QStringList
    get_condition_string_list();

    /**
     * @brief Sets the selected comparison operator by index.
     * @param idx Index of the operator in the combo box (0-based).
     *
     * Programmatically selects a comparison operator. Used when loading saved
     * node configurations.
     *
     * **Example:**
     * @code
     * widget->set_condition_text_index(0); // Select ">" (greater than)
     * @endcode
     */
    void
    set_condition_text_index( int idx );

    /**
     * @brief Retrieves the currently selected operator index.
     * @return Index of the selected comparison operator (0-based).
     */
    int
    get_condition_text_index();

    /**
     * @brief Sets the threshold number value.
     * @param number Threshold value as a string (supports integers and floating-point).
     *
     * Sets the numeric threshold for the condition. The string format allows
     * flexible input including scientific notation.
     *
     * **Example:**
     * @code
     * widget->set_condition_number("3.14159");
     * widget->set_condition_number("1e-6");
     * @endcode
     */
    void
    set_condition_number( QString number );

    /**
     * @brief Retrieves the current threshold number value.
     * @return Threshold value as a QString.
     */
    QString
    get_condition_number( );

public Q_SLOTS:
    /**
     * @brief Slot triggered when the comparison operator selection changes.
     * @param idx Index of the newly selected operator.
     *
     * Handles the currentIndexChanged signal from the combo box and emits
     * condition_changed_signal() with the updated operator and current number.
     */
    void
    condition_combo_box_current_index_changed( int idx );

    /**
     * @brief Slot triggered when the threshold number text changes.
     * @param text New threshold value as text.
     *
     * Handles the textChanged signal from the line edit and emits
     * condition_changed_signal() with the current operator and updated number.
     */
    void
    condition_number_text_changed( const QString & text );

Q_SIGNALS:
    /**
     * @brief Signal emitted when the condition configuration changes.
     * @param cond_idx Index of the selected comparison operator.
     * @param number Threshold value as a string.
     *
     * This signal is emitted whenever either the operator or threshold value changes,
     * allowing the parent model to update its conditional logic immediately.
     *
     * **Example Usage:**
     * @code
     * connect(widget, &MathConditionEmbeddedWidget::condition_changed_signal,
     *         model, &MathConditionModel::updateCondition);
     * @endcode
     */
    void
    condition_changed_signal( int cond_idx, QString number );

private:
    Ui::MathConditionEmbeddedWidget *ui; ///< UI components generated by Qt Designer
};

#endif // MATHCONDITIONEMBEDDEDWIDGET_H
