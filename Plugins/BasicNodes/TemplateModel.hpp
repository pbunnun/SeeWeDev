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
 * @file TemplateModel.hpp
 * @brief Template node model for reference implementation and testing.
 *
 * This file defines the TemplateModel class, which serves as a reference implementation
 * for creating custom node models. It demonstrates standard patterns including embedded
 * widgets, multiple data types (image, vector, information), property management, state
 * persistence, and UI control integration.
 *
 * **Purpose:** Development template and testing framework for new node creation.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "InformationData.hpp"
#include "StdVectorNumberData.hpp"
#include "TemplateEmbeddedWidget.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class TemplateModel
 * @brief Reference template node model for development and testing.
 *
 * This model serves as a comprehensive template for creating custom node models,
 * demonstrating best practices for:
 * - Multiple output data types (CVImageData, StdVectorIntData, InformationData)
 * - Embedded widget integration (TemplateEmbeddedWidget)
 * - Property management and persistence
 * - Button-triggered actions
 * - Enable/disable state handling
 *
 * **Output Ports:**
 * 1. **CVImageData** - Example image output
 * 2. **StdVectorIntData** - Example integer vector output
 * 3. **InformationData** - Example information/text output
 *
 * **Key Features:**
 * - No input ports (data source/generator node example)
 * - Embedded widget with combo box, spin box, and buttons
 * - Property browser integration (checkbox, text, size, point)
 * - Start/Stop/Send button controls
 * - JSON save/load for state persistence
 *
 * **Demonstrated Patterns:**
 *
 * 1. **Embedded Widget Integration:**
 *    - TemplateEmbeddedWidget provides UI controls
 *    - Button signals trigger model actions
 *    - Widget state synchronized with model properties
 *
 * 2. **Property Management:**
 *    - Boolean property (mbCheckBox)
 *    - String property (msDisplayText)
 *    - Geometric properties (mSize, mPoint)
 *    - setModelProperty() handles property updates from browser
 *
 * 3. **State Persistence:**
 *    - save() serializes all properties to JSON
 *    - load() restores state from JSON
 *    - Widget state saved and restored
 *
 * 4. **Output Generation:**
 *    - Multiple output data types
 *    - Data created on demand (lazy evaluation)
 *    - outData() provides access to generated data
 *
 * 5. **Lifecycle Management:**
 *    - late_constructor() for post-construction initialization
 *    - enable_changed() for node enable/disable handling
 *
 * **Properties (Configurable):**
 * - **checkbox:** Boolean flag (mbCheckBox)
 * - **display_text:** Display string (msDisplayText)
 * - **size:** QSize dimension (mSize)
 * - **point:** QPoint coordinate (mPoint)
 *
 * **Use Cases (for Template):**
 * - Starting point for new node development
 * - Testing framework integration
 * - UI control pattern reference
 * - Property system demonstration
 * - Multi-output node example
 *
 * **Development Guide:**
 * When creating a new node based on this template:
 * 1. Copy TemplateModel and TemplateEmbeddedWidget files
 * 2. Rename class and update _category, _model_name
 * 3. Modify input/output port configuration in nPorts() and dataType()
 * 4. Implement actual processing in button handlers or setInData()
 * 5. Add/remove properties in setModelProperty() and save()/load()
 * 6. Customize embedded widget UI in .ui file
 * 7. Update processData() or equivalent for actual functionality
 *
 * **Example Customization:**
 * @code
 * // Convert to image processing node
 * class MyCustomModel : public PBNodeDelegateModel {
 *     // Add input port in nPorts()
 *     // Implement setInData() to receive image
 *     // Process in button handler or immediately
 *     // Output result via outData()
 * };
 * @endcode
 *
 * @see TemplateEmbeddedWidget
 * @see PBNodeDelegateModel
 */
class TemplateModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a TemplateModel.
     *
     * Initializes default values and creates the embedded widget.
     */
    TemplateModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~TemplateModel() override {}

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing all properties and widget state.
     *
     * Demonstrates proper state serialization including embedded widget
     * configuration.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved state.
     *
     * Restores all properties and updates embedded widget to match.
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 0 for input (no inputs), 3 for output (image, vector, info).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return CVImageData (port 0), StdVectorIntData (port 1), InformationData (port 2).
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the output data.
     * @param port Port index (0=image, 1=vector, 2=info).
     * @return Shared pointer to generated data.
     *
     * Creates and returns example data for each port type.
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data (no-op for this template).
     * @param nodeData Input data (unused).
     * @param Port index (unused).
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    /**
     * @brief Returns the embedded widget.
     * @return Pointer to TemplateEmbeddedWidget.
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets a model property from property browser.
     * @param Property name ("checkbox", "display_text", "size", "point").
     * @param QVariant value.
     *
     * Demonstrates property handling pattern for different data types.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Late constructor called after node creation.
     *
     * Used for initialization that requires the node to be fully constructed,
     * such as setting up connections or initializing resources.
     */
    void
    late_constructor() override;

    static const QString _category;   ///< Node category ("Template" or "Development")
    static const QString _model_name; ///< Node display name

private Q_SLOTS:
    
    /**
     * @brief Slot for embedded widget button clicks.
     * @param button Button identifier (0=Start, 1=Stop, 2=Send).
     *
     * Handles all button actions from the embedded widget.
     */
    void
    em_button_clicked( int button );

    /**
     * @brief Slot for node enable/disable state changes.
     * @param Enabled state.
     *
     * Allows model to respond to enable/disable actions.
     */
    void
    enable_changed(bool) override;

private:
    TemplateEmbeddedWidget * mpEmbeddedWidget;  ///< Embedded UI widget

    std::shared_ptr< CVImageData > mpCVImageData;       ///< Example image output
    std::shared_ptr< StdVectorIntData > mpStdVectorIntData; ///< Example vector output
    std::shared_ptr< InformationData > mpInformationData;   ///< Example info output

    bool mbCheckBox{ true };              ///< Example boolean property
    QString msDisplayText{ "ComboBox" };  ///< Example string property
    QSize mSize { QSize( 1, 1 ) };        ///< Example size property
    QPoint mPoint { QPoint( 7, 7 ) };     ///< Example point property
};

