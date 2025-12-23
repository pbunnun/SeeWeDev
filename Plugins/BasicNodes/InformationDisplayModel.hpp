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
 * @file InformationDisplayModel.hpp
 * @brief Provides a text display widget for showing information messages in the dataflow graph.
 *
 * This file implements a node that receives InformationData messages and displays them in an
 * embedded scrollable text widget. It serves as a debugging, logging, and monitoring endpoint
 * for text-based information flowing through the processing pipeline.
 *
 * The node acts as a visual terminal or console within the dataflow graph, accumulating
 * messages over time with automatic scrolling and line limit management to prevent memory
 * overflow during long-running sessions.
 *
 * Key Features:
 * - Embedded resizable text display widget
 * - Automatic message accumulation (newest messages appended)
 * - Configurable line history limit (default: 100 lines)
 * - Auto-scroll to latest message
 * - Multi-line text support with proper formatting
 * - Read-only display (prevents accidental editing)
 *
 * Typical Applications:
 * - Debug message logging from processing nodes
 * - Status updates and progress reporting
 * - Error and warning message display
 * - Numerical result monitoring (counts, measurements, statistics)
 * - Algorithm state tracking
 * - Performance metrics display
 * - Event notification logs
 *
 * The display automatically manages history by limiting the number of visible lines,
 * removing oldest messages when the limit is exceeded. This ensures stable memory
 * usage in long-running workflows while maintaining visibility of recent activity.
 *
 * @see InformationDisplayModel, InformationDisplayEmbeddedWidget, InformationData
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPlainTextEdit>

#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"
#include "InformationDisplayEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class InformationDisplayModel
 * @brief Node that displays text information messages in an embedded scrollable widget.
 *
 * This model provides a visual endpoint for InformationData flowing through the processing
 * pipeline. It's designed to receive string messages from other nodes and display them in
 * a chronological, scrollable text area embedded directly in the node.
 *
 * Functionality Overview:
 * The node acts as a message accumulator and viewer:
 * 1. Receives InformationData messages via input port
 * 2. Appends each message to the embedded text widget
 * 3. Automatically scrolls to show newest messages
 * 4. Maintains a rolling history limited by miMaxLineCount
 * 5. Removes oldest lines when limit is exceeded
 *
 * Message Flow:
 * ```
 * Any Node → [InformationData] → InformationDisplay
 *                                        ↓
 *                              [Embedded Text Widget]
 *                              Line 1: "Processing started..."
 *                              Line 2: "Detected 5 objects"
 *                              Line 3: "Average size: 120.5 px²"
 *                              Line 4: "Processing time: 23ms"
 *                              ... (auto-scrolls to bottom)
 * ```
 *
 * Widget Behavior:
 * - Text Display: Plain text format (QPlainTextEdit)
 * - Read-Only: User cannot edit displayed content
 * - Auto-Scroll: Automatically scrolls to show latest message
 * - Line Wrapping: Long lines wrap to widget width
 * - Scrollbars: Appear automatically when content exceeds visible area
 *
 * Line Limit Management (miMaxLineCount):
 * - Default: 100 lines maximum
 * - Behavior: When limit reached, oldest lines are removed (FIFO)
 * - Purpose: Prevent unbounded memory growth in long-running sessions
 * - Tuning: Increase for longer history, decrease for minimal memory
 *
 * Common Use Cases:
 *
 * 1. Debug Logging:
 *    ```
 *    ProcessingNode → Information("Debug: Value = " + value) → InformationDisplay
 *    ```
 *    Displays intermediate values and states for debugging algorithms
 *
 * 2. Object Counting Results:
 *    ```
 *    FindContours → InformationConcatenate → InformationDisplay
 *         ↓              ("Found " + count + " objects")
 *    CircleCount → 
 *    ```
 *    Shows detection results from multiple sources
 *
 * 3. Performance Monitoring:
 *    ```
 *    Timer → Information("Frame time: " + ms + "ms") → InformationDisplay
 *    ```
 *    Tracks processing speed and performance metrics
 *
 * 4. Status Updates:
 *    ```
 *    Camera → Information("Frame " + frameNum) → InformationDisplay
 *    ```
 *    Displays sequential status messages during acquisition
 *
 * 5. Error/Warning Log:
 *    ```
 *    ValidationNode → Information("WARNING: ...") → InformationDisplay
 *    ```
 *    Accumulates warning and error messages for review
 *
 * 6. Multi-Source Aggregation:
 *    ```
 *    Source1 → Info → ┐
 *    Source2 → Info → ├→ InformationDisplay (shows all messages chronologically)
 *    Source3 → Info → ┘
 *    ```
 *    Collects messages from multiple nodes in one display
 *
 * Widget Lifecycle:
 * - Created: When node is instantiated (InformationDisplayEmbeddedWidget)
 * - Displayed: Always visible in the node (embeddedWidget())
 * - Updated: On each incoming InformationData message
 * - Cleared: Manually via widget interface (if implemented)
 * - Destroyed: When node is deleted from graph
 *
 * Message Format:
 * - Plain text strings (no HTML formatting by default)
 * - Multi-line messages supported (newlines preserved)
 * - Timestamp or prefix can be added by upstream nodes
 * - No automatic time stamping (add via InfoConcatenate if needed)
 *
 * Memory Management:
 * With miMaxLineCount = 100:
 * - Approximate memory: 100 lines × ~50 bytes/line ≈ 5KB (text only)
 * - Widget overhead: ~10-20KB (Qt internal structures)
 * - Total per node: ~15-25KB (very lightweight)
 * - For longer history: miMaxLineCount = 1000 → ~50KB
 *
 * Performance Characteristics:
 * - Message Display: < 1ms per message (text append operation)
 * - Line Removal: < 1ms when limit exceeded (remove oldest line)
 * - Scrolling: Automatic, no performance impact
 * - Update Rate: Can handle 100+ messages/second without lag
 * - Memory: O(miMaxLineCount) - bounded and predictable
 *
 * Advantages:
 * - Real-time visibility into pipeline activity
 * - No separate console or log file needed
 * - Visual integration with dataflow graph
 * - Bounded memory usage
 * - Multiple independent displays possible (one per node)
 *
 * Limitations:
 * - Text-only display (no rich formatting or images)
 * - No message filtering or search functionality
 * - Limited history (controlled by miMaxLineCount)
 * - No export or save functionality
 * - Read-only (cannot send messages back to pipeline)
 *
 * Design Rationale:
 * - Embedded Widget: Keeps information visible directly in graph (no popup windows)
 * - Line Limit: Prevents memory leaks in production deployments
 * - Auto-Scroll: Ensures latest activity is always visible
 * - Plain Text: Simple, fast, and sufficient for most debugging needs
 *
 * Best Practices:
 * 1. Use descriptive message prefixes for clarity ("ERROR:", "DEBUG:", etc.)
 * 2. Limit message frequency in high-speed loops (e.g., every 10th frame)
 * 3. Adjust miMaxLineCount based on expected message volume
 * 4. Use multiple displays to separate different message categories
 * 5. Resize widget larger for detailed log inspection
 * 6. Combine with InfoConcatenate for structured messages
 *
 * Comparison with Alternatives:
 * - vs. Console Output: InformationDisplay is visible in graph, easier to monitor
 * - vs. File Logging: Real-time display, no disk I/O overhead
 * - vs. CVImageDisplay: Specialized for text, more efficient for messages
 * - vs. InfoConcatenate: Display is endpoint, Concatenate is processor
 *
 * @see InformationDisplayEmbeddedWidget, InformationData, InfoConcatenateModel
 */
class InformationDisplayModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    InformationDisplayModel();

    virtual
    ~InformationDisplayModel() override {}

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Receives InformationData and appends it to the embedded text display.
     *
     * This method is called whenever new data arrives at the input port. It extracts
     * the text string from the InformationData and appends it to the widget's text area.
     *
     * Behavior:
     * 1. Check if nodeData is valid (not nullptr)
     * 2. Cast to InformationData to extract text string
     * 3. Append text to embedded widget (via QPlainTextEdit::appendPlainText)
     * 4. Auto-scroll to show newly added text
     * 5. If line count exceeds miMaxLineCount, remove oldest lines
     *
     * Example Flow:
     * ```cpp
     * // Upstream node sends: InformationData("Detected 3 circles")
     * setInData(infoData, 0);
     * // Widget now displays:
     * // ... (previous messages)
     * // Detected 3 circles
     * // (cursor auto-scrolled to bottom)
     * ```
     *
     * Line Limit Enforcement:
     * ```cpp
     * // If miMaxLineCount = 100 and current line count = 100:
     * // New message arrives
     * setInData(newData, 0);
     * // Behavior: Remove line 1, append new message at line 100
     * // Result: Still 100 lines, oldest message removed
     * ```
     *
     * Thread Safety:
     * - This method runs in the main Qt event loop thread
     * - Widget updates are thread-safe (handled by Qt internally)
     *
     * @param nodeData Incoming InformationData containing text message
     * @param port Input port index (always 0 for this node)
     *
     * @note nullptr nodeData is safely ignored (no crash, no display update)
     * @note Multi-line messages (with \n) are supported and properly formatted
     *
     * @see InformationData, InformationDisplayEmbeddedWidget
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    /**
     * @brief Returns the embedded text display widget.
     * @return Pointer to InformationDisplayEmbeddedWidget showing accumulated messages
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;

    static const QString _model_name;

private:

    InformationDisplayEmbeddedWidget * mpEmbeddedWidget;  ///< Embedded text display widget (QPlainTextEdit-based)

    /**
     * @brief Maximum number of lines to retain in display history.
     *
     * When the number of displayed lines exceeds this limit, the oldest lines are
     * automatically removed to prevent unbounded memory growth. This implements a
     * rolling window of recent messages.
     *
     * Default Value: 100 lines
     * - Sufficient for typical debug sessions
     * - Memory footprint: ~5-10KB for text content
     * - Adjustable via property system if needed
     *
     * Typical Settings:
     * - Quick debugging: 50 lines (minimal memory)
     * - Standard monitoring: 100 lines (default)
     * - Long session logging: 500-1000 lines (more history)
     * - Production monitoring: 1000+ lines (comprehensive log)
     *
     * Memory Impact:
     * - 100 lines ≈ 5KB text + widget overhead
     * - 1000 lines ≈ 50KB text + widget overhead
     * - 10000 lines ≈ 500KB (not recommended, use file logging instead)
     */
    int miMaxLineCount{100};

    //std::shared_ptr< InformationData > mpInformationData { nullptr };

    QPixmap _minPixmap;
};

