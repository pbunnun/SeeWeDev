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
 * @file CVRGBsetValueEmbeddedWidget.hpp
 * @brief Interactive widget for RGB channel value selection and control.
 *
 * This file defines the embedded widget UI for the RGBsetValueModel node,
 * providing controls for selecting which RGB/BGR channel to modify and
 * setting its value.
 */

#pragma once

#include <QWidget>

namespace Ui {
class CVRGBsetValueEmbeddedWidget;
}

/**
 * @class CVRGBsetValueEmbeddedWidget
 * @brief Interactive Qt widget for RGB/BGR channel value control.
 *
 * ## Overview
 * Provides a UI for selecting which color channel to modify and setting
 * its value. Used by RGBsetValueModel for interactive channel manipulation.
 *
 * ## Widget Features
 * - **Channel Selector**: Choose R, G, or B channel via combo box or radio buttons
 * - **Value Slider/Spinbox**: Set channel value from 0-255
 * - **Reset Button**: Restore default values (all channels to 0)
 * - **Live Preview**: Changes trigger immediate node reprocessing
 *
 * ## Signal/Slot Mechanism
 * The widget emits button_clicked_signal() when the user modifies settings,
 * which the model handles via em_button_clicked() to update parameters and
 * reprocess the image.
 *
 * @see RGBsetValueModel For the node that uses this widget
 */
class CVRGBsetValueEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVRGBsetValueEmbeddedWidget(QWidget *parent = nullptr);
    ~CVRGBsetValueEmbeddedWidget();

Q_SIGNALS:
    /**
     * @brief Emitted when user modifies channel selection or value.
     * @param button Unused parameter (legacy interface)
     *
     * Connected to RGBsetValueModel::em_button_clicked() to trigger
     * image reprocessing with the new channel settings.
     */
    void
    button_clicked_signal( int button );

private Q_SLOTS:
    /**
     * @brief Handles the reset button click.
     *
     * Resets all channel values to 0 and emits button_clicked_signal()
     * to notify the model of the change.
     */
    void reset_button_clicked();

private:
    Ui::CVRGBsetValueEmbeddedWidget *ui;  ///< Qt Designer UI form
};

