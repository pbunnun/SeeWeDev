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
 * @file CVErodeAndDilateEmbeddedWidget.hpp
 * @brief Interactive widget for selecting morphological operation type.
 *
 * This file defines the embedded widget UI for morphological operation nodes,
 * providing radio button controls for selecting between:
 * - Erosion: Shrinks foreground regions, removes noise
 * - Dilation: Expands foreground regions, fills holes
 *
 * @see CVErodeAndDilateModel For the morphological operation node
 * @see cv::erode, cv::dilate for OpenCV morphological operations
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QWidget>

namespace Ui {
class CVErodeAndDilateEmbeddedWidget;
}

/**
 * @class CVErodeAndDilateEmbeddedWidget
 * @brief Interactive Qt widget for morphological operation selection.
 *
 * ## Overview
 * Provides a simple UI for choosing between erosion and dilation morphological
 * operations. Used by morphological processing nodes for interactive operation
 * mode selection.
 *
 * ## Widget Features
 * - **Erode Radio Button**: Selects erosion operation (shrink foreground)
 * - **Dilate Radio Button**: Selects dilation operation (expand foreground)
 * - **State Persistence**: Maintains selection across sessions
 *
 * ## Morphological Operations
 * - **Erosion**: Removes pixels from object boundaries, shrinks foreground
 *   * Use for: Noise removal, separating connected objects
 * - **Dilation**: Adds pixels to object boundaries, expands foreground
 *   * Use for: Filling holes, joining broken segments
 *
 * @see cv::erode for erosion implementation
 * @see cv::dilate for dilation implementation
 */
class CVErodeAndDilateEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:

    explicit CVErodeAndDilateEmbeddedWidget(QWidget *parent = nullptr);
    ~CVErodeAndDilateEmbeddedWidget();
    
    /**
     * @brief Gets the current operation state.
     * @return Current state (0=Erode, 1=Dilate)
     */
    int getCurrentState() const;
    
    /**
     * @brief Sets the operation state.
     * @param state New state (0=Erode, 1=Dilate)
     */
    void setCurrentState(const int state);

Q_SIGNALS:
    /**
     * @brief Emitted when user changes the selected operation.
     *
     * Notifies the model to reprocess image with new operation type.
     */
    void radioButton_clicked_signal();

private Q_SLOTS:

    /**
     * @brief Handles erode radio button click.
     *
     * Sets state to 0 (erosion) and emits signal.
     */
    void erode_radio_button_clicked();
    
    /**
     * @brief Handles dilate radio button click.
     *
     * Sets state to 1 (dilation) and emits signal.
     */
    void dilate_radio_button_clicked();

private:

    Ui::CVErodeAndDilateEmbeddedWidget* ui;  ///< Qt Designer UI form
    int currentState;                      ///< Current operation (0=Erode, 1=Dilate)

};

