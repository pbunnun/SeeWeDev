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
 * @file CVBlendImagesEmbeddedWidget.hpp
 * @brief Interactive widget for image blending mode selection.
 *
 * This file defines the embedded widget UI for the CVBlendImagesModel node,
 * providing controls for selecting between simple addition and weighted blending modes.
 */

#pragma once

#include <QWidget>

namespace Ui {
class CVBlendImagesEmbeddedWidget;
}

/**
 * @class CVBlendImagesEmbeddedWidget
 * @brief Interactive Qt widget for blending mode selection.
 *
 * ## Overview
 * Provides a UI for selecting between different image blending modes.
 * Used by CVBlendImagesModel for interactive blending configuration.
 *
 * ## Widget Features
 * - **Add Mode**: Simple pixel-wise addition (cv::add)
 * - **Add Weighted Mode**: Weighted blending with alpha/beta control (cv::addWeighted)
 * - **Radio Buttons**: Visual mode selection interface
 * - **State Persistence**: Maintains selection across sessions
 *
 * ## Blending Modes
 * 1. **Add**: output = image1 + image2 (no weighting)
 * 2. **Add Weighted**: output = alpha×image1 + beta×image2 + gamma
 *
 * @see CVBlendImagesModel For the node that uses this widget
 */
class CVBlendImagesEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVBlendImagesEmbeddedWidget(QWidget *parent = nullptr);
    ~CVBlendImagesEmbeddedWidget();

    /**
     * @brief Gets the current blending mode state.
     * @return Current state (0=Add, 1=Add Weighted)
     */
    int getCurrentState() const;
    
    /**
     * @brief Sets the blending mode state.
     * @param state New state to set (0=Add, 1=Add Weighted)
     *
     * Updates the radio button selection to match the specified mode.
     */
    void setCurrentState(const int state);

Q_SIGNALS:
    /**
     * @brief Emitted when user changes the blending mode.
     *
     * Connected to CVBlendImagesModel to trigger reprocessing with
     * the newly selected blending mode.
     */
    void radioButton_clicked_signal();

private Q_SLOTS:

    /**
     * @brief Handles Add radio button click.
     *
     * Sets state to 0 (simple addition mode) and emits signal
     * to notify the model of the mode change.
     */
    void add_radio_button_clicked();

    /**
     * @brief Handles Add Weighted radio button click.
     *
     * Sets state to 1 (weighted blending mode) and emits signal
     * to notify the model of the mode change.
     */
    void add_weighted_radio_button_clicked();

private:
    Ui::CVBlendImagesEmbeddedWidget *ui;  ///< Qt Designer UI form
    int currentState;                   ///< Current blending mode (0=Add, 1=Add Weighted)
};

