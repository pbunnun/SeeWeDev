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
 * @file InformationDisplayEmbeddedWidget.hpp
 * @brief Embedded widget for displaying textual information with export capabilities.
 *
 * This file defines the InformationDisplayEmbeddedWidget class, which provides a text
 * display area for viewing streaming or accumulated information within a node's UI.
 * The widget includes features for clearing the display, exporting content to files,
 * and managing display buffer size to prevent memory overflow.
 */

#ifndef INFORMATIONDISPLAYEMBEDDEDWIDGET_H
#define INFORMATIONDISPLAYEMBEDDEDWIDGET_H

#include <QWidget>

namespace Ui {
class InformationDisplayEmbeddedWidget;
}

/**
 * @class InformationDisplayEmbeddedWidget
 * @brief Widget for displaying and managing textual information streams.
 *
 * This widget provides a QPlainTextEdit-based display area for showing textual information
 * flowing through the node graph. It is designed for real-time monitoring, debugging, and
 * logging purposes within the visual programming environment.
 *
 * **Key Features:**
 * - Plain text display with automatic scrolling
 * - Configurable line limit to prevent memory overflow
 * - Clear button to reset the display
 * - Export button to save content to text files
 * - Append-only interface for stream-like data
 *
 * **Line Management:**
 * The widget can limit the maximum number of displayed lines to prevent memory issues
 * during long-running operations. When the limit is reached, old lines are removed
 * automatically (FIFO behavior).
 *
 * **Use Cases:**
 * - Display numerical data from sensors or calculations
 * - Show debugging messages and status updates
 * - Log timestamps and event notifications
 * - Monitor streaming text data from external sources
 * - Display concatenated information from InfoConcatenateModel
 *
 * **Workflow:**
 * 1. Parent model appends text via appendPlainText()
 * 2. Widget displays text with automatic scrolling to latest content
 * 3. User can clear display or export to file as needed
 * 4. Line limit prevents unbounded memory growth
 *
 * @see InformationDisplayModel
 * @see InfoConcatenateModel
 * @see InformationData
 */
class InformationDisplayEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs an InformationDisplayEmbeddedWidget.
     * @param parent Parent widget (typically nullptr for embedded widgets).
     *
     * Initializes the text display area and control buttons (Clear, Export).
     * The display starts empty and ready to receive text.
     */
    explicit InformationDisplayEmbeddedWidget( QWidget *parent = nullptr );
    
    /**
     * @brief Destructor.
     *
     * Cleans up UI components and releases allocated resources.
     */
    ~InformationDisplayEmbeddedWidget();
    
    /**
     * @brief Appends text to the display area.
     * @param Text string to append (can include newlines).
     *
     * Adds the provided text to the end of the current display content.
     * Automatically scrolls to show the latest appended text. If max line
     * count is set and exceeded, removes old lines from the beginning.
     *
     * **Example:**
     * @code
     * widget->appendPlainText("Temperature: 25.3°C");
     * widget->appendPlainText("Pressure: 1013.2 hPa\n");
     * @endcode
     */
    void appendPlainText( QString );
    
    /**
     * @brief Sets the maximum number of lines to display.
     * @param maxLines Maximum line count (0 = unlimited).
     *
     * Configures the line limit for the display buffer. When the limit is reached,
     * the oldest lines are removed to make room for new content. Setting to 0
     * disables the limit (caution: may cause memory issues with long-running streams).
     *
     * **Recommended Values:**
     * - Interactive monitoring: 100-500 lines
     * - Debug logging: 1000-5000 lines
     * - Long-term data collection: Use export and periodic clearing instead
     *
     * **Example:**
     * @code
     * widget->setMaxLineCount(1000); // Keep last 1000 lines
     * @endcode
     */
    void setMaxLineCount( int maxLines );
Q_SIGNALS:
    /**
     * @brief Signal emitted when the display is clicked.
     *
     * Can be used to notify the parent model to select or focus the node.
     */
    void
    widgetClicked(); // Signal to request node selection

public Q_SLOTS:
    /**
     * @brief Slot to clear all displayed text.
     *
     * Removes all content from the display area, resetting it to empty state.
     * Triggered when the user clicks the "Clear" button or called programmatically
     * to reset the display.
     */
    void
    clear_button_clicked();

    /**
     * @brief Slot to export displayed text to a file.
     *
     * Opens a file dialog allowing the user to save the current display content
     * to a text file. Triggered when the user clicks the "Export" button.
     *
     * **File Format:** Plain text (.txt)
     *
     * **Use Case:** Save logged data, debugging output, or monitoring results
     * for later analysis or reporting.
     */
    void
    export_button_clicked();

protected:
    /**
     * @brief Event filter to capture focus events on child widgets.
     * @param obj The object receiving the event.
     * @param event The event being processed.
     * @return true if the event is handled, false to continue default processing.
     *
     * This filter detects when the text display area gains or loses focus,
     * emitting signals to notify the parent model about selection changes.
     */
    bool eventFilter(QObject* obj, QEvent* event) override;
    /**
     * @brief Handles mouse press events to emit widgetClicked signal.
     * @param event Mouse event information.
     *
     * Overrides the default mousePressEvent to emit a signal when the widget
     * is clicked. This allows the parent model to respond by selecting or
     * focusing the corresponding node in the graph.
     */
    void mousePressEvent(QMouseEvent* event) override;
private:
    Ui::InformationDisplayEmbeddedWidget *ui; ///< UI components generated by Qt Designer
};

#endif // INFORMATIONDISPLAYEMBEDDEDWIDGET_H
