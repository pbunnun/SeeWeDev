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
 * @file CVImageLoaderEmbeddedWidget.hpp
 * @brief Embedded widget UI for the image loader node
 * 
 * This file defines the user interface component embedded within the
 * CVImageLoaderModel node. It provides controls for:
 * - Loading image files or directories
 * - Displaying image thumbnails
 * - Navigating through image sequences (slideshow controls)
 * - Play/pause functionality for automatic playback
 */

#pragma once

#include <QVariant>
#include <QWidget>
#include <QLabel>

namespace Ui {
class CVImageLoaderEmbeddedWidget;
}

/**
 * @class CVImageLoaderEmbeddedWidget
 * @brief Interactive UI widget for loading and previewing images
 * 
 * This widget provides the user interface for the CVImageLoaderModel node,
 * featuring:
 * - File/directory browser button
 * - Image thumbnail display
 * - Slideshow controls (play/pause, forward, backward)
 * - Filename display
 * - Automatic resize notifications to parent node
 * 
 * The widget communicates with its parent node via signals, particularly
 * button_clicked_signal() for user actions and widget_resized_signal()
 * to notify the node when the widget geometry changes.
 * 
 * @note This widget uses Qt Designer UI file (CVImageLoaderEmbeddedWidget.ui)
 * @see CVImageLoaderModel for the node model that uses this widget
 */
class CVImageLoaderEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the embedded widget
     * 
     * Initializes the UI from the Designer file and sets up signal/slot
     * connections for the control buttons.
     * 
     * @param parent Parent widget (typically the node's graphics item)
     */
    explicit CVImageLoaderEmbeddedWidget( QWidget *parent = nullptr );
    
    /**
     * @brief Destructor
     * 
     * Cleans up the UI components created by Qt Designer.
     */
    ~CVImageLoaderEmbeddedWidget();

    /**
     * @brief Updates the displayed filename
     * 
     * Sets the text of the filename label to show which image is currently
     * loaded. Useful for providing user feedback in slideshow mode.
     * 
     * @param filename The filename to display (can be full path or just basename)
     */
    void
    set_filename( QString );

    /**
     * @brief Sets the active/inactive visual state
     * 
     * Changes the widget's appearance to indicate whether the node is
     * actively processing or idle.
     * 
     * @param active true for active state, false for inactive
     * @note Typically used to provide visual feedback during operations
     */
    void
    set_active( bool );

    /**
     * @brief Sets the play/pause button state
     * 
     * Updates the play/pause button icon and state to reflect whether
     * automatic slideshow playback is active or paused.
     * 
     * @param paused true to show play icon (paused state), false for pause icon (playing)
     */
    void
    set_flip_pause( bool );

    /**
     * @brief Toggles the play/pause button state
     * 
     * Convenience method to flip between play and pause states without
     * explicitly knowing the current state.
     */
    void
    revert_play_pause_state();

Q_SIGNALS:
    /**
     * @brief Signal emitted when a control button is clicked
     * 
     * Notifies the parent node which button was activated:
     * - Button 0: Open file/directory
     * - Button 1: Forward (next image)
     * - Button 2: Backward (previous image)
     * - Button 3: Play/Pause toggle
     * - Button 4: Filename button (quick reload)
     * 
     * @param button The button identifier (0-based index)
     * @note Connected to CVImageLoaderModel::em_button_clicked()
     */
    void
    button_clicked_signal( int button );

    /**
     * @brief Signal emitted when the widget is resized
     * 
     * Notifies the parent node that the widget geometry has changed,
     * allowing the node to update its bounding box and layout.
     * 
     * @note Connected to trigger embeddedWidgetSizeUpdated() in the model
     */
    void
    widget_resized_signal();

public Q_SLOTS:

    /**
     * @brief Slot for forward button click
     * 
     * Advances to the next image in the sequence.
     */
    void
    forward_button_clicked();

    /**
     * @brief Slot for backward button click
     * 
     * Goes back to the previous image in the sequence.
     */
    void
    backward_button_clicked();

    /**
     * @brief Slot for play/pause button click
     * 
     * Toggles automatic slideshow playback.
     */
    void
    play_pause_button_clicked();

    /**
     * @brief Slot for open button click
     * 
     * Opens a file/directory browser dialog.
     */
    void
    open_button_clicked();

    /**
     * @brief Slot for filename button click
     * 
     * Quick action to reload the current image or perform filename-related action.
     */
    void
    filename_button_clicked();

protected:
    /**
     * @brief Handles widget resize events
     * 
     * Overrides QWidget::resizeEvent to emit widget_resized_signal(),
     * ensuring the parent node is notified of geometry changes.
     * 
     * @param event The resize event details
     * @note Critical for maintaining proper node layout in the graph
     */
    void resizeEvent(QResizeEvent *event) override;

private:
    /** @brief UI components created by Qt Designer */
    Ui::CVImageLoaderEmbeddedWidget *ui;
};

