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
 * @file CVUSBCameraEmbeddedWidget.hpp
 * @brief Embedded UI controls for CVUSBCameraModel node.
 *
 * This widget provides camera control interface displayed within the CVUSBCameraModel node:
 * - Camera device selection dropdown (0, 1, 2, ...)
 * - Start/Stop capture buttons
 * - Connection status indicator (visual feedback)
 *
 * The widget communicates with CVUSBCameraModel via Qt signals and is embedded directly
 * in the node's visual representation (no separate window).
 *
 * @see CVUSBCameraModel for the parent node implementation
 */

#pragma once

#include <QVariant>
#include <QWidget>
#include <QLabel>

/**
 * @struct CVUSBCameraProperty
 * @brief Camera identification and status.
 */
typedef struct CVUSBCameraProperty{
    int miCameraID{0};              ///< Device index (0, 1, 2, ...)
    bool mbCameraStatus{false};     ///< Connection status (true=connected, false=disconnected)
} CVUSBCameraProperty;

namespace Ui {
class CVUSBCameraEmbeddedWidget;
}

/**
 * @class CVUSBCameraEmbeddedWidget
 * @brief Qt widget providing camera controls embedded in CVUSBCameraModel node.
 *
 * This widget displays:
 * - **Camera ID ComboBox**: Select device (0, 1, 2, ...)
 * - **Start Button**: Begin capture
 * - **Stop Button**: End capture
 * - **Status Indicator**: Visual feedback (color-coded background or icon)
 *
 * **Widget Lifecycle:**
 * ```cpp
 * CVUSBCameraEmbeddedWidget *widget = new CVUSBCameraEmbeddedWidget(parent);
 * widget->set_camera_property(cameraProperty);  // Set initial device ID
 * connect(widget, &CVUSBCameraEmbeddedWidget::button_clicked_signal,
 *         model, &CVUSBCameraModel::em_button_clicked);
 * widget->set_ready_state(true);  // Enable controls when camera ready
 * ```
 *
 * **Button Click Signals:**
 * - button_clicked_signal(0): Start capture
 * - button_clicked_signal(1): Stop capture
 * - button_clicked_signal(2+): Camera ID changed to new value
 *
 * @see CVUSBCameraModel
 */
class CVUSBCameraEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVUSBCameraEmbeddedWidget( QWidget *parent = nullptr );
    ~CVUSBCameraEmbeddedWidget();

    /**
     * @brief Sets camera device ID and status.
     * @param prop CVUSBCameraProperty with miCameraID and mbCameraStatus
     */
    void
    set_camera_property( CVUSBCameraProperty );

    /**
     * @brief Replace camera ID options and select the provided current ID if available.
     * @param cameraIds List of camera IDs as strings
     * @param currentId The ID to select if present
     */
    void
    set_camera_id_options( const QStringList &cameraIds, int currentId );

    /**
     * @brief Gets current camera property (ID and status).
     * @return CVUSBCameraProperty struct
     */
    CVUSBCameraProperty
    get_camera_property() const { return mCVUSBCameraProperty; }

    /**
     * @brief Enables/disables Start button based on camera readiness.
     * @param ready true=enable Start button, false=disable (camera not available)
     */
    void
    set_ready_state( bool );

    /**
     * @brief Shows/hides widget or enables/disables controls.
     * @param active true=active and visible, false=inactive
     */
    void
    set_active( bool );

Q_SIGNALS:
    /**
     * @brief Emitted when any button is clicked.
     * @param button 0=Start, 1=Stop, 2+=Camera ID changed
     */
    void
    button_clicked_signal( int button );

public Q_SLOTS:
    /**
     * @brief Updates status indicator when camera connection changes.
     * @param connected true=show green/connected, false=show red/disconnected
     */
    void
    camera_status_changed( bool );

    /**
     * @brief Handles manual refresh button click.
     */
    void
    refresh_button_clicked();

    /**
     * @brief Handles Start button click (initiates capture).
     */
    void
    start_button_clicked();

    /**
     * @brief Handles Stop button click (halts capture).
     */
    void
    stop_button_clicked();

    /**
     * @brief Handles camera ID selection change.
     * @param index New device ID selected in ComboBox
     */
    void
    camera_id_combo_box_current_index_changed( int );

private:
    CVUSBCameraProperty mCVUSBCameraProperty;   ///< Current camera ID and status
    QLabel * mpTransparentLabel;        ///< Status overlay or background indicator
    Ui::CVUSBCameraEmbeddedWidget *ui;     ///< UI form generated from .ui file
};
