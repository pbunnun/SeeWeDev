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
 * @file CVImageROIEmbeddedWidget.hpp
 * @brief Embedded UI controls for CVImageROI selection node.
 *
 * This widget provides Apply/Reset buttons for interactive ROI (Region of Interest)
 * selection workflows. It's embedded in the CVImageROI node to confirm or discard
 * user-drawn ROI rectangles.
 *
 * **Typical Workflow:**
 * 1. User draws ROI rectangle on image display
 * 2. Click Apply → Confirms ROI, crops image to selected region
 * 3. Click Reset → Discards ROI, reverts to full image
 *
 * @see CVCVImageROI for the parent ROI selection node
 */

#pragma once

#include <QWidget>

namespace Ui {
class CVImageROIEmbeddedWidget;
}

/**
 * @class CVImageROIEmbeddedWidget
 * @brief Qt widget with Apply/Reset buttons for ROI selection confirmation.
 *
 * This simple widget provides two-button interface for ROI operations:
 * - **Apply Button**: Confirms current ROI selection, crops image
 * - **Reset Button**: Cancels ROI, restores full image
 *
 * Buttons can be enabled/disabled programmatically based on ROI state
 * (e.g., disable Apply until valid ROI is drawn).
 *
 * **Usage Pattern:**
 * ```cpp
 * CVImageROIEmbeddedWidget *widget = new CVImageROIEmbeddedWidget(parent);
 * connect(widget, &CVImageROIEmbeddedWidget::button_clicked_signal,
 *         model, &CVImageROIModel::handle_button_click);
 * 
 * // After user draws ROI:
 * widget->enable_apply_button(true);   // Enable Apply
 * widget->enable_reset_button(true);   // Enable Reset
 * 
 * // After Apply clicked:
 * widget->enable_apply_button(false);  // Disable until new ROI drawn
 * ```
 *
 * **Button Signals:**
 * - button_clicked_signal(0): Apply button clicked → Confirm ROI
 * - button_clicked_signal(1): Reset button clicked → Clear ROI
 *
 * @see CVCVImageROI for complete ROI selection node
 */
class CVImageROIEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVImageROIEmbeddedWidget(QWidget *parent = nullptr);
    ~CVImageROIEmbeddedWidget();

    /**
     * @brief Enables or disables the Apply button.
     * @param enable true=enable (ROI valid), false=disable (no ROI or invalid)
     */
    void enable_apply_button(const bool enable);

    /**
     * @brief Enables or disables the Reset button.
     * @param enable true=enable (ROI exists to reset), false=disable (nothing to reset)
     */
    void enable_reset_button(const bool enable);

Q_SIGNALS :

    /**
     * @brief Emitted when Apply or Reset button is clicked.
     * @param button 0=Apply (confirm ROI), 1=Reset (clear ROI)
     */
    void button_clicked_signal( int button );

private Q_SLOTS :

    /**
     * @brief Handles Apply button click.
     * Emits button_clicked_signal(0) to parent node.
     */
    void apply_button_clicked();

    /**
     * @brief Handles Reset button click.
     * Emits button_clicked_signal(1) to parent node.
     */
    void reset_button_clicked();

private:
    Ui::CVImageROIEmbeddedWidget *ui; ///< UI form generated from .ui file
};

