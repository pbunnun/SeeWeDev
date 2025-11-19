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
 * @file CombineSyncEmbeddedWidget.hpp
 * @brief Interactive widget for CombineSync node control.
 *
 * This file defines the embedded widget UI for the CombineSyncModel node,
 * providing controls for:
 * - Combine operation selection (AND/OR)
 * - Input port count configuration (2-10)
 * - Reset button for clearing ready states
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QWidget>

namespace Ui {
class CombineSyncEmbeddedWidget;
}

/**
 * @class CombineSyncEmbeddedWidget
 * @brief Interactive Qt widget for synchronization combiner control.
 *
 * Provides UI controls for configuring how multiple sync signals are combined.
 * Used by CombineSyncModel for runtime configuration.
 *
 * ## Widget Features
 * - **Operation ComboBox**: Select AND or OR logic
 * - **Input Size SpinBox**: Set number of input ports (2-10)
 * - **Reset Button**: Clear all ready states and sync values
 *
 * @see CombineSyncModel For the sync combiner node
 */
class CombineSyncEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CombineSyncEmbeddedWidget(QWidget *parent = nullptr);
    ~CombineSyncEmbeddedWidget() override;

    /**
     * @brief Sets the current operation mode.
     * @param index 0 for AND, 1 for OR
     */
    void set_operation(int index);

    /**
     * @brief Sets the number of input ports.
     * @param size Number of inputs (2-10)
     */
    void set_input_size(int size);

    /**
     * @brief Gets the current operation index.
     * @return 0 for AND, 1 for OR
     */
    int get_operation() const;

    /**
     * @brief Gets the current input size.
     * @return Number of input ports
     */
    int get_input_size() const;

Q_SIGNALS:
    /**
     * @brief Emitted when operation selection changes.
     * @param operation "AND" or "OR"
     */
    void operation_changed_signal(const QString& operation);

    /**
     * @brief Emitted when input size changes.
     * @param size New number of input ports
     */
    void input_size_changed_signal(int size);

    /**
     * @brief Emitted when reset button is clicked.
     */
    void reset_clicked_signal();

private:
    Ui::CombineSyncEmbeddedWidget *ui;  ///< Qt Designer UI form
};
