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
 * @file CVRTSPCameraEmbeddedWidget.hpp
 * @brief Embedded UI controls for CVRTSPCameraModel node.
 *
 * This widget provides RTSP camera control interface displayed within the CVRTSPCameraModel node:
 * - RTSP URL line edit for entering stream URL
 * - Start/Stop capture buttons
 * - Connection status indicator (visual feedback)
 *
 * The widget communicates with CVRTSPCameraModel via Qt signals and is embedded directly
 * in the node's visual representation (no separate window).
 *
 * @see CVRTSPCameraModel for the parent node implementation
 */

#pragma once

#include <QVariant>
#include <QWidget>
#include <QLabel>
#include <QString>

/**
 * @struct CVRTSPCameraProperty
 * @brief RTSP camera URL and status.
 */
typedef struct CVRTSPCameraProperty{
    QString msRTSPUrl;              ///< RTSP stream URL (e.g., "rtsp://192.168.1.100:554/stream")
    bool mbCameraStatus{false};     ///< Connection status (true=connected, false=disconnected)
} CVRTSPCameraProperty;

namespace Ui {
class CVRTSPCameraEmbeddedWidget;
}

/**
 * @class CVRTSPCameraEmbeddedWidget
 * @brief Qt widget providing RTSP camera controls embedded in CVRTSPCameraModel node.
 *
 * This widget displays:
 * - **RTSP URL LineEdit**: Enter RTSP stream URL
 * - **Start Button**: Begin capture
 * - **Stop Button**: End capture
 * - **Status Indicator**: Visual feedback (color-coded background or icon)
 *
 * **Widget Lifecycle:**
 * ```cpp
 * CVRTSPCameraEmbeddedWidget *widget = new CVRTSPCameraEmbeddedWidget(parent);
 * widget->set_camera_property(cameraProperty);  // Set initial RTSP URL
 * connect(widget, &CVRTSPCameraEmbeddedWidget::button_clicked_signal,
 *         model, &CVRTSPCameraModel::em_button_clicked);
 * widget->set_ready_state(true);  // Enable controls when camera ready
 * ```
 *
 * **Button Click Signals:**
 * - button_clicked_signal(0): Start capture
 * - button_clicked_signal(1): Stop capture
 * - button_clicked_signal(2): RTSP URL changed
 *
 * @see CVRTSPCameraModel
 */
class CVRTSPCameraEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVRTSPCameraEmbeddedWidget( QWidget *parent = nullptr );
    ~CVRTSPCameraEmbeddedWidget();

    /**
     * @brief Sets RTSP URL and status.
     * @param prop CVRTSPCameraProperty with msRTSPUrl and mbCameraStatus
     */
    void
    set_camera_property( CVRTSPCameraProperty );

    /**
     * @brief Gets current camera property (URL and status).
     * @return CVRTSPCameraProperty struct
     */
    CVRTSPCameraProperty
    get_camera_property() const { return mCVRTSPCameraProperty; }

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
     * @brief Emitted when any button is clicked or URL is changed.
     * @param button 0=Start, 1=Stop, 2=URL changed
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
     * @brief Handles RTSP URL text change.
     */
    void
    rtsp_url_editing_finished();

private:
    CVRTSPCameraProperty mCVRTSPCameraProperty;   ///< Current RTSP URL and status
    QLabel * mpTransparentLabel;        ///< Status overlay or background indicator
    Ui::CVRTSPCameraEmbeddedWidget *ui;     ///< UI form generated from .ui file
};
