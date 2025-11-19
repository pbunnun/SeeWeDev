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
 * @file CVCameraCalibrationEmbeddedWidget.hpp
 * @brief Interactive widget for camera calibration control and image management.
 *
 * This file defines the embedded widget UI for the CVCameraCalibrationModel node,
 * providing comprehensive controls for:
 * - Capturing calibration images (manual or automatic)
 * - Managing calibration image collection
 * - Triggering calibration computation
 * - Exporting calibration parameters
 * - Reviewing captured images
 *
 * @see CVCameraCalibrationModel For the node that uses this widget
 */

#pragma once

#include <QVariant>
#include <QWidget>
#include <QLabel>

namespace Ui {
class CVCameraCalibrationEmbeddedWidget;
}

/**
 * @class CVCameraCalibrationEmbeddedWidget
 * @brief Interactive Qt widget for camera calibration workflow control.
 *
 * ## Overview
 * Provides a comprehensive UI for the camera calibration process, managing the
 * entire workflow from image capture through calibration computation to parameter
 * export. Used by CVCameraCalibrationModel for interactive calibration operations.
 *
 * ## Widget Features
 * - **Capture Button**: Manually capture current frame for calibration
 * - **Auto-Capture Checkbox**: Enable automatic capture at intervals
 * - **Calibrate Button**: Trigger calibration computation from captured images
 * - **Export Button**: Save calibration parameters to file
 * - **Navigation Buttons**: Forward/Backward through captured images
 * - **Remove Button**: Delete currently displayed calibration image
 * - **Status Display**: Show filename, image count, calibration state
 *
 * ## Calibration Workflow
 * 1. Position calibration pattern (checkerboard) in view
 * 2. Capture multiple images from different angles/positions
 * 3. Review captured images (navigate forward/backward)
 * 4. Remove bad images if needed
 * 5. Trigger calibration computation
 * 6. Export computed parameters
 *
 * ## Auto-Capture Mode
 * When enabled, automatically captures frames at regular intervals,
 * simplifying the data collection process for calibration.
 *
 * @see CVCameraCalibrationModel For calibration algorithm and parameter management
 */
class CVCameraCalibrationEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVCameraCalibrationEmbeddedWidget( QWidget *parent = nullptr );
    ~CVCameraCalibrationEmbeddedWidget();

    /**
     * @brief Sets the displayed filename/path label.
     * @param filename Path to calibration data file or current image
     *
     * Updates the UI to show which file is currently in use for
     * loading/saving calibration parameters.
     */
    void
    set_filename( QString );

    /**
     * @brief Sets the calibration active state indicator.
     * @param active true if calibration parameters are loaded/computed
     *
     * Updates UI indicators to show whether valid calibration
     * parameters are currently active.
     */
    void
    set_active( bool );

    /**
     * @brief Sets the auto-capture checkbox state.
     * @param flag true to enable auto-capture, false to disable
     *
     * Programmatically controls the auto-capture checkbox, allowing
     * the model to enable/disable automatic image capture.
     */
    void
    set_auto_capture_flag( bool );

    /**
     * @brief Updates the total image count display.
     * @param count Total number of captured calibration images
     *
     * Shows the user how many images have been collected for
     * calibration (typically 10-20 images recommended).
     */
    void
    update_total_images( int );

    /**
     * @brief Sets the current image number in the review sequence.
     * @param number Current image index being displayed (1-based)
     *
     * Updates the display to show which image in the collection
     * is currently being reviewed.
     */
    void
    set_image_number( int );
    
Q_SIGNALS:
    /**
     * @brief Emitted when any button is clicked.
     * @param button Button identifier code
     *
     * Signals the model which action was triggered:
     * - Capture: Add current frame to calibration set
     * - Calibrate: Compute calibration from collected images
     * - Export: Save parameters to file
     * - Forward/Backward: Navigate through images
     * - Remove: Delete current image from set
     */
    void
    button_clicked_signal( int button );

public Q_SLOTS:

    /**
     * @brief Handles the capture button click.
     *
     * Triggered when user manually requests to capture the current
     * frame for inclusion in the calibration image set.
     */
    void
    capture_button_clicked();

    /**
     * @brief Handles the calibrate button click.
     *
     * Triggered when user requests calibration computation from
     * the collected image set. Initiates the calibration algorithm.
     */
    void
    calibrate_button_clicked();

    /**
     * @brief Handles the export button click.
     *
     * Triggered when user requests to save computed calibration
     * parameters to a file for later use.
     */
    void
    export_button_clicked();

    /**
     * @brief Handles the forward button click.
     *
     * Navigates to the next calibration image in the sequence
     * for review and potential removal.
     */
    void
    forward_button_clicked();

    /**
     * @brief Handles the backward button click.
     *
     * Navigates to the previous calibration image in the sequence
     * for review and potential removal.
     */
    void
    backward_button_clicked();

    /**
     * @brief Handles auto-capture checkbox state change.
     * @param state New checkbox state (Qt::Checked or Qt::Unchecked)
     *
     * Enables or disables automatic frame capture mode, which
     * periodically captures frames without manual intervention.
     */
    void
    auto_checkbox_stateChanged(int state);

    /**
     * @brief Handles the remove button click.
     *
     * Deletes the currently displayed calibration image from the
     * collection. Useful for removing poor quality captures.
     */
    void
    remove_button_clicked();

private:
    Ui::CVCameraCalibrationEmbeddedWidget *ui;  ///< Qt Designer UI form
};
