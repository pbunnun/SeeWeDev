//Copyright © 2020 - 2026, NECTEC, all rights reserved

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
 * @file CVVideoLoaderEmbeddedWidget.hpp
 * @brief Interactive control widget for video file playback.
 *
 * Provides a compact playback toolbar embedded inside the CVVideoLoaderModel node:
 * - **Filename button**: Opens a file-chooser dialog to select a video file
 * - **Play/Pause button**: Toggles between continuous playback and single-frame mode
 * - **Forward / Backward buttons**: Advance or rewind by one frame (step mode)
 * - **Seek slider**: Scrubs to any frame position in the video
 * - **Frame spinbox**: Jumps directly to a specific frame number
 *
 * All user interactions are compressed into two signals (`button_clicked_signal` and
 * `slider_value_signal`) so that CVVideoLoaderModel can handle all control events
 * from a single slot dispatch.
 *
 * Resize events are forwarded via `widget_resized_signal()` so that
 * PBNodeGeometry can recompute node layout when the widget changes size.
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QVariant>
#include <QWidget>
#include <QLabel>

namespace Ui {
class CVVideoLoaderEmbeddedWidget;
}

/**
 * @class CVVideoLoaderEmbeddedWidget
 * @brief Toolbar widget providing video playback controls for CVVideoLoaderModel.
 *
 * Embedded directly inside the node body. All button actions are encoded as
 * integer codes in button_clicked_signal() so the model needs only one slot
 * to dispatch all user interactions.
 *
 * **Button Codes (button_clicked_signal):**
 * - 0: Filename (open file dialog)
 * - 1: Play/Pause toggle
 * - 2: Forward (next frame)
 * - 3: Backward (previous frame)
 *
 * **Slider/Spinbox (slider_value_signal):**
 * - Both the seek slider and frame spinbox emit the same signal with the target
 *   frame number, allowing the model to seek to any frame position.
 */
class CVVideoLoaderEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    /// @name Construction & Destruction
    /// @{

    /**
     * @brief Constructs the widget and connects all UI element signals.
     * @param parent Parent widget (typically nullptr for embedded widgets).
     */
    explicit CVVideoLoaderEmbeddedWidget( QWidget *parent = nullptr );

    /// @brief Destructor; deletes the generated UI object.
    ~CVVideoLoaderEmbeddedWidget();

    /// @}

    /// @name State Setters
    /// @{

    /**
     * @brief Updates the filename label displayed in the widget.
     * @param filename Path or name of the currently loaded video file.
     */
    void set_filename( QString filename );

    /**
     * @brief Pauses the video (sets Play/Pause button to paused state).
     *
     * Called by the model when playback reaches the end of the video or
     * the node is disabled.
     */
    void set_flip_pause( bool pause );

    /**
     * @brief Sets the maximum value of the seek slider (total frame count - 1).
     * @param maxFrames Total number of frames in the loaded video.
     */
    void set_maximum_slider( int maxFrames );

    /**
     * @brief Moves the seek slider to the given frame position.
     *
     * Called by the model to keep the slider position in sync with actual playback.
     *
     * @param frameNo Frame index to display (0-based).
     */
    void set_slider_value( int frameNo );

    /**
     * @brief Programmatically toggles the play/pause button state.
     * @param playing true = play state; false = paused state.
     */
    void set_toggle_play( bool playing );

    /**
     * @brief Forces the widget to the paused state without emitting signals.
     *
     * Used when the model stops playback internally (e.g., end of video, node disabled).
     */
    void pause_video();

    /// @}

Q_SIGNALS:
    /// @name Output Signals
    /// @{

    /**
     * @brief Emitted when any control button is clicked.
     *
     * Button codes: 0=filename, 1=play/pause, 2=forward, 3=backward.
     *
     * @param button Integer code identifying which button was pressed.
     */
    void button_clicked_signal( int button );

    /**
     * @brief Emitted when the slider or spinbox value changes (seek request).
     * @param value Target frame index (0-based).
     */
    void slider_value_signal( int value );

    /**
     * @brief Emitted when the widget is resized.
     *
     * Connected to PBNodeGeometry to trigger node layout recompute.
     */
    void widget_resized_signal();

    /// @}

public Q_SLOTS:
    /// @name Button Slots (connected to UI buttons)
    /// @{

    /// @brief Handles forward button click; emits button_clicked_signal(2).
    void forward_button_clicked();

    /// @brief Handles backward button click; emits button_clicked_signal(3).
    void backward_button_clicked();

    /// @brief Handles play/pause toggle; emits button_clicked_signal(1).
    void play_pause_button_clicked();

    /// @brief Handles filename button click; emits button_clicked_signal(0).
    void filename_button_clicked();

    /**
     * @brief Handles seek slider value change; emits slider_value_signal(value).
     * @param value New slider position (frame index).
     */
    void slider_value_changed( int value );

    /**
     * @brief Handles frame spinbox value change; emits slider_value_signal(value).
     * @param value New frame number entered in the spinbox.
     */
    void frame_number_spinbox_value_changed( int value );

    /// @}

protected:
    /// @name Event Handling
    /// @{

    /**
     * @brief Filters wheel events on the slider to prevent accidental seeks.
     * @param obj Watched object.
     * @param event Incoming event.
     * @return true if event should be filtered (consumed); false to propagate.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @brief Forwards resize events as widget_resized_signal().
     * @param event Resize event with new widget dimensions.
     */
    void resizeEvent(QResizeEvent *event) override;

    /// @}

private:
    /// UI components generated by Qt Designer.
    Ui::CVVideoLoaderEmbeddedWidget *ui;
};
