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
 * @file CVFloodFillEmbeddedWidget.hpp
 * @brief Interactive widget for flood fill tolerance configuration.
 *
 * This file defines the embedded widget UI for the CVFloodFillModel node,
 * providing spinbox controls for setting color tolerance ranges used in
 * the flood fill operation.
 *
 * The widget supports both grayscale and color images, dynamically showing
 * appropriate controls based on image type.
 *
 * @see CVFloodFillModel For the flood fill operation node
 * @see cv::floodFill for OpenCV flood fill implementation
 */

#pragma once

#include <QWidget>
#include <QSpinBox>

namespace Ui {
class CVFloodFillEmbeddedWidget;
}

/**
 * @class CVFloodFillEmbeddedWidget
 * @brief Interactive Qt widget for flood fill color tolerance configuration.
 *
 * ## Overview
 * Provides spinbox controls for setting upper and lower color tolerance bounds
 * used in flood fill operations. The widget adapts its interface based on whether
 * the image is grayscale or color. Used by CVFloodFillModel for interactive tolerance
 * adjustment.
 *
 * ## Widget Features
 * - **Lower/Upper Tolerance Spinboxes**: Set color difference thresholds
 * - **Grayscale Mode**: Single tolerance value for intensity
 * - **Color Mode**: Separate B, G, R tolerance values
 * - **Mask Status Label**: Shows if optional mask is active
 * - **Dynamic UI**: Shows/hides controls based on image type
 *
 * ## Flood Fill Tolerance
 * Tolerance values define how similar a pixel must be to the seed point to be filled:
 * - **Lower Tolerance**: Pixels can be darker by this amount
 * - **Upper Tolerance**: Pixels can be brighter by this amount
 * - Range: [seed_color - lower, seed_color + upper]
 *
 * @see CVFloodFillModel For flood fill algorithm and seed point selection
 * @see cv::floodFill for tolerance parameter usage
 */
class CVFloodFillEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVFloodFillEmbeddedWidget(QWidget *parent = nullptr);
    ~CVFloodFillEmbeddedWidget();

    /**
     * @brief Sets the mask status label text.
     * @param active true if mask input is active, false otherwise
     *
     * Updates UI to show whether the optional mask input is being used.
     */
    void set_maskStatus_label(const bool active);

    /**
     * @brief Shows/hides controls based on image channel count.
     * @param channels Number of image channels (1=grayscale, 3=color)
     *
     * Displays appropriate spinboxes for the image type:
     * - 1 channel: Show grayscale tolerance controls
     * - 3 channels: Show B, G, R tolerance controls
     */
    void toggle_widgets(const int channels);

    /**
     * @brief Sets the tolerance values programmatically.
     * @param lower Lower tolerance array [B, G, R, Gray]
     * @param upper Upper tolerance array [B, G, R, Gray]
     *
     * Updates all spinboxes with the specified tolerance values.
     */
    void set_lower_upper(const int (&lower)[4], const int (&upper)[4]);

Q_SIGNALS:

    /**
     * @brief Emitted when any spinbox value changes.
     * @param spinbox Spinbox identifier (0-7)
     * @param value New spinbox value
     *
     * Notifies the model to update tolerance parameters and reprocess.
     */
    void spinbox_clicked_signal( int spinbox, int value );

private Q_SLOTS:

    /**
     * @brief Handles lower blue tolerance change.
     * @param value New value
     */
    void lower_b_spinbox_value_changed( int );

    /**
     * @brief Handles lower green tolerance change.
     * @param value New value
     */
    void lower_g_spinbox_value_changed( int );

    /**
     * @brief Handles lower red tolerance change.
     * @param value New value
     */
    void lower_r_spinbox_value_changed( int );

    /**
     * @brief Handles lower grayscale tolerance change.
     * @param value New value
     */
    void lower_gray_spinbox_value_changed( int );

    /**
     * @brief Handles upper blue tolerance change.
     * @param value New value
     */
    void upper_b_spinbox_value_changed( int );

    /**
     * @brief Handles upper green tolerance change.
     * @param value New value
     */
    void upper_g_spinbox_value_changed( int );

    /**
     * @brief Handles upper red tolerance change.
     * @param value New value
     */
    void upper_r_spinbox_value_changed( int );

    /**
     * @brief Handles upper grayscale tolerance change.
     * @param value New value
     */
    void upper_gray_spinbox_value_changed( int );

private:
    Ui::CVFloodFillEmbeddedWidget *ui;  ///< Qt Designer UI form
};

