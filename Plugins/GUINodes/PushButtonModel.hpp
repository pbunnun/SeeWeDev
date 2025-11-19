/**
 * @file PushButtonModel.hpp
 * @brief Interactive button node for manual triggering and control.
 *
 * This file defines the PushButtonModel class, which embeds a QPushButton widget
 * in a dataflow graph node. It provides manual control capabilities by generating
 * synchronization triggers and click count events when the button is pressed.
 *
 * **Key Features:**
 * - **Manual Triggering:** User-initiated dataflow execution
 * - **Click Counting:** Tracks total button presses (via IntegerData output)
 * - **Dual Outputs:** SyncData trigger + IntegerData count
 * - **Source Node:** No inputs, generates events
 * - **Interactive Control:** Direct user interaction in graph
 * - **Testing Tool:** Manual step-through for debugging
 *
 * **Common Use Cases:**
 * - Manual start/stop control for pipelines
 * - Step-by-step debugging of workflows
 * - Testing individual processing branches
 * - Interactive demonstrations
 * - Trigger capture/save operations manually
 * - Reset or initialize operations
 *
 * **Typical Workflow:**
 * @code
 * // Manual image capture trigger
 * PushButton → [sync] → CameraCapture → [image] → Display
 * 
 * // Click counter for testing
 * PushButton → [int] → LCDNumber (shows click count)
 * 
 * // Manual save trigger
 * PushButton → [sync] → SaveImage (saves current frame)
 * 
 * // Step-through processing
 * PushButton → [sync] → ProcessingNode → NextNode
 * @endcode
 *
 * **Output Behavior:**
 * - **Port 0 (SyncData):** Triggers on every button click
 * - **Port 1 (IntegerData):** Cumulative click count (increments)
 *
 * **Implementation Details:**
 * - No input ports (source node)
 * - Two output ports (sync + count)
 * - Thread-safe click handling
 * - Embedded QPushButton widget
 * - Persistent click count during session
 *
 * **Design Patterns:**
 * @code
 * // Gate pattern - enable/disable processing
 * PushButton → [sync] → SyncGate → ProcessingChain
 * 
 * // Trigger pattern - one-shot operations
 * PushButton → [sync] → OneShot → Action
 * 
 * // Counter pattern - limited executions
 * PushButton → [int] → Threshold → Conditional
 * @endcode
 *
 * @note This is a source node with no input ports.
 * @note Click count persists during runtime but resets on graph reload.
 * @note Both outputs trigger simultaneously on each click.
 * @note Useful for testing and manual control workflows.
 *
 * @see PBNodeDelegateModel
 * @see SyncData
 * @see IntegerData
 * @see LCDNumberModel
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPushButton>
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "IntegerData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class PushButtonModel
 * @brief Interactive push button node for manual triggering.
 *
 * This node embeds a QPushButton widget to provide user-initiated triggers
 * and click counting within the dataflow graph. Acts as a source node for
 * manual control and testing workflows.
 *
 * **Port Configuration:**
 * - No input ports (source node)
 * - Output Port 0: SyncData (trigger signal)
 * - Output Port 1: IntegerData (click count)
 *
 * **Widget Properties:**
 * - **mpEmbeddedWidget:** Embedded QPushButton widget
 * - **mpIntData:** IntegerData storing cumulative click count
 *
 * **Event Flow:**
 * 1. User clicks button
 * 2. em_button_clicked() slot triggered
 * 3. Click count incremented in IntegerData
 * 4. New SyncData created
 * 5. Both outputs updated simultaneously
 * 6. Connected nodes receive trigger/count
 *
 * **Output Synchronization:**
 * @code
 * void em_button_clicked() {
 *   mpIntData->increment();                         // Increment counter
 *   mpSyncData = std::make_shared<SyncData>();     // Port 0: new trigger
 *   dataUpdated(0);  // Notify sync output
 *   dataUpdated(1);  // Notify count output
 * }
 * @endcode
 *
 * **Connection Examples:**
 * @code
 * // Manual capture trigger
 * PushButton → [sync] → CameraCaptureNode → [image] → Display
 * 
 * // Click counter display
 * PushButton → [int] → LCDNumberModel (shows total clicks)
 * 
 * // Conditional execution
 * PushButton → [int] → MathCondition (execute every 5 clicks)
 * 
 * // Dual control
 * PushButton → [sync] → Action1
 *           → [int]  → Counter → Threshold → Action2
 * @endcode
 *
 * **Testing Workflows:**
 * Use PushButton for step-by-step debugging:
 * @code
 * // Step through image processing
 * PushButton → [sync] → LoadImage → Filter1 → Filter2 → Display
 * 
 * // Manual frame advance
 * PushButton → [sync] → VideoCapture (advances one frame)
 * 
 * // Iterative testing
 * PushButton → [sync] → Algorithm → ResultValidator
 * @endcode
 *
 * **Click Count Persistence:**
 * - Persists during runtime (cumulative)
 * - Can be saved/restored via JSON
 * - IntegerData maintains count state
 * - Useful for session-based counting
 *
 * **Performance Characteristics:**
 * - Lightweight UI event handling
 * - No computational overhead
 * - Instant response to clicks
 * - Suitable for high-frequency manual triggering
 *
 * **Best Practices:**
 * - Use for manual control, not automated workflows
 * - Combine with SyncGate for enable/disable logic
 * - Use click count for limited-execution scenarios
 * - Label button clearly in node caption
 * - Avoid in production automated pipelines
 *
 * @note This is a source node with no inputs.
 * @note Click count can be persisted via save/load.
 * @note Both outputs trigger on same click event.
 * @note Ideal for testing and interactive control.
 *
 * @see PBNodeDelegateModel for base class functionality
 * @see SyncData for trigger signal type
 * @see IntegerData for count data type
 * @see LCDNumberModel for complementary display node
 */
class PushButtonModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs push button control node.
     *
     * Initializes the QPushButton widget and connects the clicked()
     * signal to the internal click handler.
     */
    PushButtonModel();

    /**
     * @brief Destructor.
     */
    virtual
        ~PushButtonModel() override { }

    /**
     * @brief Saves node configuration to JSON.
     * @return JSON object with button state.
     *
     * Saves the current click count for persistence.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores node configuration from JSON.
     * @param p JSON object with saved configuration.
     *
     * Restores the click count state.
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Port type (In or Out).
     * @return 0 for In (no inputs), 2 for Out (sync + count).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a port.
     * @param portType Port type (In or Out).
     * @param portIndex Port index.
     * @return SyncData for output port 0, IntegerData for port 1.
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns output data.
     * @param port Output port index (0 = sync, 1 = count).
     * @return SyncData for port 0, IntegerData for port 1.
     *
     * Provides the current trigger signal or click count to
     * connected downstream nodes.
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data (not used - no inputs).
     * @param nodeData Ignored.
     * @param portIndex Ignored.
     *
     * No-op implementation since this is a source node with no inputs.
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override { }

    /**
     * @brief Returns the embedded QPushButton widget.
     * @return Pointer to push button widget.
     *
     * Provides the QPushButton for embedding in the node's graphics item.
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets model property from property browser.
     * @param property Property name.
     * @param value New property value.
     *
     * Recieve signals back from QtPropertyBrowser and use this function to
     * set parameters/variables accordingly.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

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
     * @brief Button click event handler.
     *
     * Called when the user clicks the button. Increments the click count,
     * creates new SyncData trigger, updates IntegerData count, and notifies
     * both output ports of the data change.
     *
     * **Event Sequence:**
     * 1. Increment click count in mpIntData
     * 2. Create new SyncData (timestamp trigger)
     * 3. Emit dataUpdated(0) for sync port
     * 4. Emit dataUpdated(1) for count port
     */
    void
    em_button_clicked( );

    /**
     * @brief Handles node enable/disable state changes.
     * @param enabled True if node is enabled, false otherwise.
     */
    void
    enable_changed(bool) override;

private:
    /**
     * @brief Push button widget.
     *
     * QPushButton widget providing the interactive control element.
     * Connected to em_button_clicked() slot.
     */
    QPushButton * mpEmbeddedWidget;

    /**
     * @brief Sync trigger output data.
     *
     * Recreated on each button click to provide fresh synchronization
     * trigger to downstream nodes.
     */
    std::shared_ptr< SyncData > mpSyncData;
    
    /**
     * @brief Click count output data.
     *
     * Stores the cumulative click count, updated on each button press.
     * Can be persisted via save/load.
     */
    std::shared_ptr< IntegerData > mpIntData;
};
