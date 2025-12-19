/**
 * @file LCDNumberModel.hpp
 * @brief LCD-style numeric display node for visualizing integer values.
 *
 * This file defines the LCDNumberModel class, which embeds a QLCDNumber widget
 * in a dataflow graph node. It provides a classic 7-segment LCD-style display
 * for real-time visualization of numeric data streams without external windows.
 *
 * **Key Features:**
 * - **LCD Display:** Classic 7-segment digit visualization
 * - **IntegerData Input:** Displays incoming integer values
 * - **Configurable Digits:** Adjustable digit count (default: 5)
 * - **Multiple Display Modes:** Supports decimal, hexadecimal, binary
 * - **Embedded Visualization:** No external UI required
 *
 * **Common Use Cases:**
 * - Counter displays for iteration counts
 * - Frame number monitoring in video streams
 * - Numeric sensor readouts
 * - Debugging numeric pipelines
 * - Dashboard visualizations
 * - Real-time metrics display
 *
 * **Typical Workflow:**
 * @code
 * // Connect counter to LCD display
 * Counter Node → [int] → LCDNumber Node (displays count)
 * 
 * // Monitor frame numbers
 * VideoCapture → FrameCounter → [int] → LCDNumber (shows frame #)
 * 
 * // Display computation results
 * ImageProcessing → PixelCount → [int] → LCDNumber
 * @endcode
 *
 * **Properties:**
 * - **Digit Count:** Number of digits to display (affects size)
 *
 * **Display Modes:**
 * QLCDNumber supports multiple numeral systems:
 * - Decimal (default)
 * - Hexadecimal
 * - Binary
 * - Octal
 *
 * **Implementation Details:**
 * - No output ports (visualization sink node)
 * - Single IntegerData input port
 * - Thread-safe display updates
 * - Persistent digit count configuration
 *
 * @note This is a visualization-only node with no output ports.
 * @note The widget can be resized by dragging node corners.
 * @note Digit count affects minimum widget size.
 *
 * @see PBNodeDelegateModel
 * @see IntegerData
 * @see QLCDNumber
 * @see PushButtonModel
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLCDNumber>
#include "PBNodeDelegateModel.hpp"
#include "IntegerData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class LCDNumberModel
 * @brief LCD-style numeric display node for integer visualization.
 *
 * This node embeds a QLCDNumber widget to provide a classic 7-segment LCD
 * display for real-time integer value monitoring within the dataflow graph.
 *
 * **Port Configuration:**
 * - Input Port 0: IntegerData (value to display)
 * - No output ports
 *
 * **Widget Properties:**
 * - **mpEmbeddedWidget:** Embedded QLCDNumber widget
 * - **miDigitCount:** Number of digits (default: 5, configurable)
 *
 * **Display Behavior:**
 * - Automatically updates when new IntegerData arrives
 * - Shows "-" for overflow/underflow conditions
 * - Supports negative numbers with leading "-" digit
 * - Clear visibility with high-contrast LCD segments
 *
 * **Configuration Example:**
 * @code
 * {
 *   "model-name": "LCDNumberModel",
 *   "digit-count": 8  // 8-digit display for larger numbers
 * }
 * @endcode
 *
 * **Connection Examples:**
 * @code
 * // Basic counter display
 * TimerModel → CounterNode → [int] → LCDNumberModel
 * 
 * // Frame counter for video
 * VideoCapture → [frame] → FrameNumberExtractor → [int] → LCDNumberModel
 * 
 * // Measurement display
 * ImageAnalysis → PixelCounter → [int] → LCDNumberModel
 * @endcode
 *
 * **Display Modes (QLCDNumber features):**
 * - **Decimal:** Standard base-10 display (default)
 * - **Hexadecimal:** Base-16 for debugging (0x...)
 * - **Binary:** Base-2 for bit analysis
 * - **Octal:** Base-8 for special cases
 *
 * **Performance Considerations:**
 * - Lightweight widget with minimal CPU usage
 * - No buffering or memory overhead
 * - Suitable for high-frequency updates
 * - GUI updates on Qt event loop
 *
 * **Persistence:**
 * - Saves digit count to JSON
 * - Restores widget configuration on load
 * - No data value persistence (display-only)
 *
 * @note This is a sink node with no outputs (visualization only).
 * @note Digit count affects minimum widget size constraints.
 * @note Use for monitoring, not for data routing.
 *
 * @see PBNodeDelegateModel for base class functionality
 * @see IntegerData for input data type
 * @see QLCDNumber for Qt widget documentation
 * @see PushButtonModel for complementary control node
 */
class LCDNumberModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs LCD number display node.
     *
     * Initializes the QLCDNumber widget with default 5-digit display
     * and sets up the visualization properties.
     */
    LCDNumberModel();

    /**
     * @brief Destructor.
     */
    /**
     * @brief Destructor.
     */
    virtual
        ~LCDNumberModel() override { }

    /**
     * @brief Saves node configuration to JSON.
     * @return JSON object with digit count configuration.
     *
     * Saves the digit count property for persistence:
     * @code
     * {
     *   "digit-count": 5
     * }
     * @endcode
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores node configuration from JSON.
     * @param p JSON object with saved configuration.
     *
     * Restores the digit count and updates the LCD widget accordingly.
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Port type (In or Out).
     * @return 1 for In (IntegerData input), 0 for Out (no outputs).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a port.
     * @param portType Port type (In or Out).
     * @param portIndex Port index.
     * @return IntegerData type for input port 0.
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Sets input data and updates LCD display.
     * @param nodeData IntegerData to display.
     * @param portIndex Input port index (0).
     *
     * Receives IntegerData and updates the QLCDNumber widget to show
     * the value. Handles null data gracefully.
     *
     * **Update Behavior:**
     * @code
     * if (integerData) {
     *   mpEmbeddedWidget->display(integerData->value());
     * }
     * @endcode
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    /**
     * @brief Returns the embedded QLCDNumber widget.
     * @return Pointer to LCD display widget.
     *
     * Provides the QLCDNumber widget for embedding in the node's
     * graphics item.
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets model property from property browser.
     * @param property Property name (e.g., "digit-count").
     * @param value New property value.
     *
     * Recieve signals back from QtPropertyBrowser and use this function to
     * set parameters/variables accordingly.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    /**
     * @brief Model category name.
     *
     * These two static members must be defined for every models. _category can be duplicate with existing categories.
     * However, _model_name has to be a unique name.
     */
    static const QString _category;

    /**
     * @brief Unique model name identifier.
     */
    static const QString _model_name;

private Q_SLOTS:
    
    /**
     * @brief Handles node enable/disable state changes.
     * @param enabled True if node is enabled, false otherwise.
     */
    void
    enable_changed(bool) override;

private:
    /**
     * @brief LCD display widget.
     *
     * QLCDNumber widget showing the current integer value with
     * 7-segment LCD-style digits.
     */
    QLCDNumber * mpEmbeddedWidget;
    
    /**
     * @brief Cached input integer data.
     *
     * Stores the last received IntegerData for potential reuse
     * (currently used for display updates).
     */
    std::shared_ptr< IntegerData > mpIntData;
    
    /**
     * @brief Number of digits to display.
     *
     * Configurable digit count affecting LCD widget size.
     * Default: 5 digits (supports -9999 to 99999).
     * Can be adjusted for larger/smaller number ranges.
     */
    int miDigitCount {5};

    QPixmap _minPixmap;
};
