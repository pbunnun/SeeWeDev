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
 * @file CVSobelAndScharrEmbeddedWidget.hpp
 * @brief Embedded widget for enabling/disabling Scharr filter mode.
 *
 * This file defines the CVSobelAndScharrEmbeddedWidget class, which provides a simple
 * checkbox interface for switching between standard Sobel and Scharr gradient filters.
 * The widget is embedded in the SobelAndScharrModel node to allow runtime selection
 * of the gradient computation method.
 */

#pragma once

#include <QWidget>

namespace Ui {
class CVSobelAndScharrEmbeddedWidget;
}

/**
 * @class CVSobelAndScharrEmbeddedWidget
 * @brief Widget for toggling Scharr filter mode in gradient computation.
 *
 * This widget provides a single checkbox that switches between Sobel and Scharr
 * gradient filter kernels. Both are derivative operators used for edge detection,
 * but Scharr provides better rotational symmetry and accuracy for gradient magnitude
 * computation at the cost of being limited to 3×3 kernels.
 *
 * **Sobel vs Scharr:**
 * - **Sobel:** General-purpose gradient filter, supports multiple kernel sizes (3, 5, 7, ...)
 *   - Kernels: [-1 0 1; -2 0 2; -1 0 1] for x-direction
 *   - Good for general edge detection
 *   - More flexible kernel size options
 * - **Scharr:** Optimized 3×3 gradient filter with better rotational invariance
 *   - Kernels: [-3 0 3; -10 0 10; -3 0 3] for x-direction
 *   - More accurate gradient magnitude and orientation
 *   - Only supports 3×3 kernels (kernel size must be -1 in cv::Scharr)
 *
 * **Key Features:**
 * - Simple checkbox interface for Scharr enable/disable
 * - Qt version compatibility (handles Qt 6.7+ checkStateChanged signature change)
 * - State query methods for checked and enabled status
 *
 * **Usage:**
 * - Unchecked: Use standard Sobel filter (configurable kernel size)
 * - Checked: Use Scharr filter (3×3 only, better accuracy)
 *
 * **Typical Workflow:**
 * 1. User checks/unchecks the "Use Scharr" checkbox
 * 2. Widget emits checkbox_checked_signal() with new state
 * 3. Parent model switches between cv::Sobel() and cv::Scharr()
 * 4. If Scharr selected, kernel size is forced to 3×3
 *
 * @see SobelAndScharrModel
 * @see cv::Sobel
 * @see cv::Scharr
 */
class CVSobelAndScharrEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVSobelAndScharrEmbeddedWidget.
     * @param parent Parent widget (typically nullptr for embedded widgets).
     *
     * Initializes the checkbox widget (typically unchecked for Sobel mode by default).
     */
    explicit CVSobelAndScharrEmbeddedWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor.
     *
     * Cleans up UI components allocated during construction.
     */
    ~CVSobelAndScharrEmbeddedWidget();

    /**
     * @brief Enables or disables the checkbox.
     * @param enable true to enable user interaction, false to disable.
     *
     * Controls whether the user can toggle the checkbox. Typically disabled
     * when certain parameter combinations are invalid.
     */
    void change_enable_checkbox(const bool enable);
    
    /**
     * @brief Programmatically sets the checkbox state.
     * @param state Qt::Checked or Qt::Unchecked.
     *
     * Sets the checkbox state without user interaction. Used when loading
     * saved node configurations or resetting to default.
     */
    void change_check_checkbox(const Qt::CheckState state);
    
    /**
     * @brief Checks if the checkbox is enabled.
     * @return true if enabled (user can interact), false if disabled.
     */
    bool checkbox_is_enabled();
    
    /**
     * @brief Checks if the checkbox is checked.
     * @return true if checked (Scharr mode), false if unchecked (Sobel mode).
     */
    bool checkbox_is_checked();

Q_SIGNALS:
    /**
     * @brief Signal emitted when checkbox state changes.
     * @param state New checkbox state (Qt::Checked or Qt::Unchecked as int).
     *
     * This signal is emitted whenever the user toggles the checkbox, allowing
     * the parent model to switch between Sobel and Scharr filter modes.
     */
    void checkbox_checked_signal( int state );

private Q_SLOTS:
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    /**
     * @brief Slot for checkbox state changes (Qt < 6.7).
     * @param state New state as int (Qt::Checked or Qt::Unchecked).
     *
     * Handles the stateChanged signal and emits checkbox_checked_signal().
     */
    void check_box_state_changed(int state);
#else
    /**
     * @brief Slot for checkbox state changes (Qt >= 6.7).
     * @param state New state as Qt::CheckState enum.
     *
     * Handles the checkStateChanged signal (new Qt 6.7+ signature) and
     * emits checkbox_checked_signal().
     */
    void check_box_check_state_changed(Qt::CheckState state);
#endif
private:
    Ui::CVSobelAndScharrEmbeddedWidget *ui; ///< UI components generated by Qt Designer
};

