/**
 * @file DisplayTextModel.hpp
 * @brief Text display node for flowchart annotations and documentation.
 *
 * This file defines the DisplayTextModel class, which provides a simple text
 * display widget embedded in a dataflow graph node. It's designed for creating
 * flowchart annotations, documentation, and visual organization without data processing.
 *
 * **Key Features:**
 * - **Text Display:** Multi-line text with configurable styling
 * - **Editable Widget:** Direct text editing in the embedded widget
 * - **Pass-Through:** InformationData input passes directly to output
 * - **Styling Options:** Font, color, and size configuration
 * - **Text Alignment:** Justify property for text alignment
 * - **No Processing:** Pure visualization/documentation node
 * - **Clean Layout:** No caption, no connection point dots by default
 *
 * **Common Use Cases:**
 * - Flowchart annotations and labels
 * - Pipeline documentation
 * - Section headers in complex graphs
 * - Notes and comments
 * - Visual organization markers
 * - Tutorial/example explanations
 *
 * **Typical Workflow:**
 * @code
 * // Use as annotation
 * [Display Text: "Image Processing Section"]
 * 
 * // Pass-through with documentation
 * Node1 → [InformationData] → DisplayText → [InformationData] → Node2
 *                             "This stage processes frames"
 * 
 * // Section marker
 * [Display Text: "=== Preprocessing ==="]
 * @endcode
 *
 * **Properties:**
 * - **Font:** Text font family
 * - **Size:** Font size in points
 * - **Color:** Text color (RGB)
 * - **Alignment:** Text justification (left/center/right)
 *
 * **Implementation Details:**
 * - One InformationData input port (optional)
 * - One InformationData output port (pass-through)
 * - Embedded QTextEdit widget for multi-line text
 * - No caption displayed by default
 * - Connection points hidden by default
 * - Direct widget editing (no property browser for text)
 *
 * @note This is a visualization/documentation node with minimal processing.
 * @note Text content is edited directly in the widget, not property browser.
 * @note Input data passes through unchanged to output.
 *
 * @see PBNodeDelegateModel
 * @see InformationData
 * @see QTextEdit
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QTextEdit>
#include <QFont>
#include <QColor>
#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class DisplayTextModel
 * @brief Text display node for flowchart annotations.
 *
 * This node embeds a QTextEdit widget to provide multi-line text display
 * and editing capabilities within the dataflow graph. It's designed for
 * documentation and visual organization rather than data processing.
 *
 * **Port Configuration:**
 * - Input Port 0: InformationData (optional, pass-through)
 * - Output Port 0: InformationData (same as input)
 *
 * **Widget Properties:**
 * - **mpEmbeddedWidget:** Embedded QTextEdit widget
 * - **msText:** Current text content
 * - **mFont:** Font family and style
 * - **miSize:** Font size in points
 * - **mColor:** Text color
 * - **miAlignment:** Text justification (0=left, 1=center, 2=right, 3=justify)
 *
 * **Display Behavior:**
 * - Text edited directly in widget
 * - Pass-through input to output without modification
 * - Resizable for long text content
 * - No caption or connection dots by default
 *
 * **Configuration Example:**
 * @code
 * {
 *   "model-name": "DisplayTextModel",
 *   "text": "Processing Section",
 *   "font": "Arial",
 *   "size": 14,
 *   "color": "#000000",
 *   "alignment": 1  // center
 * }
 * @endcode
 *
 * **Styling:**
 * - Font configurable via property browser
 * - Size adjustable (8-72 points)
 * - Color picker for text color
 * - Alignment: Left, Center, Right, Justify
 *
 * @note This is a documentation/visualization node, not a processing node.
 * @note Edit text directly in the widget, not in property browser.
 * @note Use for flowchart annotations and visual organization.
 *
 * @see PBNodeDelegateModel for base class functionality
 * @see InformationData for pass-through data type
 * @see QTextEdit for Qt widget documentation
 */
class DisplayTextModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs display text node.
     *
     * Initializes the QTextEdit widget with default styling and
     * sets up the text display properties. Configures default state
     * with no caption and no connection point dots.
     */
    DisplayTextModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~DisplayTextModel() override { }

    /**
     * @brief Saves node configuration to JSON.
     * @return JSON object with text and styling configuration.
     *
     * Saves text content and all styling properties:
     * @code
     * {
     *   "text": "...",
     *   "font": "Arial",
     *   "size": 12,
     *   "color": "#000000",
     *   "alignment": 0
     * }
     * @endcode
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores node configuration from JSON.
     * @param p JSON object with saved configuration.
     *
     * Restores text content and styling, updates widget accordingly.
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Port type (In or Out).
     * @return 1 for both In and Out (pass-through).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a port.
     * @param portType Port type (In or Out).
     * @param portIndex Port index.
     * @return InformationData type for both input and output.
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Sets input data (pass-through).
     * @param nodeData InformationData to pass through.
     * @param portIndex Input port index (0).
     *
     * Receives InformationData and stores it for pass-through to output.
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    /**
     * @brief Returns output data (pass-through input).
     * @param port Output port index (0).
     * @return Same InformationData that was received on input.
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Returns the embedded QTextEdit widget.
     * @return Pointer to text edit widget.
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets model property from property browser.
     * @param property Property name.
     * @param value New property value.
     *
     * Handles font, size, color, and alignment properties.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Event filter to handle focus events.
     * @param obj Object that triggered the event.
     * @param event Event to filter.
     * @return bool True if event was handled, false otherwise.
     *
     * Prevents node deletion when text edit has focus.
     */
    bool
    eventFilter(QObject *obj, QEvent *event) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    /**
     * @brief Model category name.
     */
    static const QString _category;

    /**
     * @brief Unique model name identifier.
     */
    static const QString _model_name;

private Q_SLOTS:
    /**
     * @brief Handles text changes in the embedded widget.
     */
    void
    text_changed();
    
    /**
     * @brief Handles node enable/disable state changes.
     * @param enabled True if node is enabled, false otherwise.
     */
    void
    enable_changed(bool) override;

private:
    /**
     * @brief Applies current styling to the widget.
     */
    void
    apply_styling();

    /**
     * @brief Text edit widget.
     */
    QTextEdit * mpEmbeddedWidget;
    
    /**
     * @brief Cached pass-through data.
     */
    std::shared_ptr< InformationData > mpInformationData;
    
    /**
     * @brief Current text content.
     */
    QString msText {""};
    
    /**
     * @brief Font family.
     */
    QString msFontFamily {"Arial"};
    
    /**
     * @brief Font size in points.
     */
    int miFontSize {12};
    
    /**
     * @brief Text color RGB components (0-255).
     */
    int mucTextColor[3] {0, 0, 0};  // Black
    
    /**
     * @brief Background color RGB components (0-255).
     */
    int mucBackgroundColor[3] {255, 255, 255};  // White
    
    /**
     * @brief Text alignment (0=left, 1=center, 2=right, 3=justify).
     */
    int miAlignment {0};
    
    /**
     * @brief Number of input ports (configurable).
     */
    unsigned int miNumInputPorts {1};
    
    /**
     * @brief Vector to store input data from multiple ports.
     */
    std::vector<std::shared_ptr<InformationData>> mvInputData;
    QPixmap _minPixmap;
};

