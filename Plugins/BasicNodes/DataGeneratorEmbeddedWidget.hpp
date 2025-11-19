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
 * @file DataGeneratorEmbeddedWidget.hpp
 * @brief Interactive widget for data generation and input.
 *
 * This file defines the embedded widget UI for the DataGeneratorModel node,
 * providing controls for:
 * - Selecting data type from dropdown menu
 * - Entering data values via text input
 * - Dynamic UI based on selected data type
 *
 * @see DataGeneratorModel For the data generation node that uses this widget
 */

#pragma once

#include <QWidget>

namespace Ui {
class DataGeneratorEmbeddedWidget;
}

/**
 * @class DataGeneratorEmbeddedWidget
 * @brief Interactive Qt widget for manual data input and type selection.
 *
 * ## Overview
 * Provides a UI for creating various data types manually, useful for testing,
 * debugging, and providing constant values to pipelines. Used by DataGeneratorModel
 * for interactive data creation.
 *
 * ## Widget Features
 * - **Data Type Combo Box**: Select data type (Integer, Double, String, etc.)
 * - **Text Input Field**: Enter value in appropriate format
 * - **Format Validation**: Ensures input matches selected data type
 * - **Live Updates**: Changes trigger immediate data regeneration
 *
 * ## Supported Data Types
 * The combo box provides options for various data types supported by the node system
 * (exact list defined in comboxboxStringList static member).
 *
 * @see DataGeneratorModel For data type generation and validation
 */
class DataGeneratorEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataGeneratorEmbeddedWidget(QWidget *parent = nullptr);
    ~DataGeneratorEmbeddedWidget();

    /**
     * @brief Gets the list of available data types.
     * @return QStringList of data type names for the combo box
     */
    QStringList get_combobox_string_list() const;

    /**
     * @brief Gets the currently selected data type index.
     * @return Index in the combo box (0-based)
     */
    int get_combobox_index() const;

    /**
     * @brief Gets the current text input value.
     * @return String entered by user (raw, not yet parsed)
     */
    QString get_text_input() const;

    /**
     * @brief Sets the selected data type.
     * @param index Index of data type to select
     */
    void set_combobox_index(const int index);

    /**
     * @brief Sets the text input field value.
     * @param input String to display in text field
     */
    void set_text_input(const QString& input);

Q_SIGNALS :

    /**
     * @brief Emitted when user modifies data type or value.
     *
     * Notifies the model to regenerate output data with new type/value.
     */
    void widget_clicked_signal();

private Q_SLOTS :

    /**
     * @brief Handles data type selection change.
     * @param index New combo box index
     *
     * Updates UI and emits signal to regenerate data.
     */
    void combo_box_current_index_changed( int );

    /**
     * @brief Handles text input changes.
     *
     * Emits signal to parse and regenerate data when user modifies value.
     */
    void plain_text_edit_text_changed();

private:
    Ui::DataGeneratorEmbeddedWidget *ui;              ///< Qt Designer UI form

    static const QStringList comboxboxStringList;     ///< Available data type names
};

