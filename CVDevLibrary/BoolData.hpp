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
 * @file BoolData.hpp
 * @brief Boolean data type for node-based dataflow communication.
 *
 * This file defines the BoolData class, which encapsulates boolean values
 * for transmission between nodes in the CVDev dataflow graph system.
 *
 * **Key Features:**
 * - **Boolean Storage:** Stores true/false values
 * - **Timestamping:** Automatic timestamp on data updates
 * - **String Conversion:** Provides "True"/"False" string representation
 * - **Type Safety:** Provides type identification for the node system
 *
 * **Common Use Cases:**
 * - Enable/disable flags for processing nodes
 * - Conditional branching in dataflow
 * - State indicators (on/off, active/inactive)
 * - Comparison results from conditional nodes
 * - Toggle switches in UI controls
 * - Trigger conditions
 *
 * **Dataflow Patterns:**
 * @code
 * // Conditional processing
 * CompareNode → [BoolData] → ConditionalNode → ProcessingBranch
 * 
 * // Enable/disable control
 * CheckboxNode → [BoolData] → EnableGateNode → [Data]
 * 
 * // State feedback
 * DetectorNode → [BoolData(detected)] → IndicatorNode
 * @endcode
 *
 * **String Representation:**
 * - true → "True"
 * - false → "False"
 *
 * @see InformationData
 * @see IntegerData
 */

#pragma once

#include <QtNodes/NodeData>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class BoolData
 * @brief Boolean data container for dataflow graph nodes.
 *
 * Encapsulates a single boolean value with automatic timestamping,
 * type identification, and string conversion for use in the CVDev
 * node-based visual programming system.
 *
 * **Data Properties:**
 * - **Type Name:** "Boolean"
 * - **Display Name:** "Bln"
 * - **Storage:** bool (true/false)
 * - **Timestamping:** Automatic on data modification
 * - **String Format:** "True" or "False"
 *
 * **Construction Examples:**
 * @code
 * // Default constructor (value = true)
 * auto data1 = std::make_shared<BoolData>();
 * 
 * // Initialize with value
 * auto data2 = std::make_shared<BoolData>(false);
 * 
 * // Set value after construction
 * auto data3 = std::make_shared<BoolData>();
 * data3->set_data(true);
 * @endcode
 *
 * **Access Patterns:**
 * @code
 * // Read value
 * bool isEnabled = data->data();
 * 
 * // Modify value (updates timestamp)
 * data->set_data(true);
 * 
 * // Get string representation
 * QString stateText = data->state_str();  // "True" or "False"
 * 
 * // Direct reference access
 * data->data() = false;  // Does NOT update timestamp
 * @endcode
 *
 * **Information Display:**
 * The set_information() method generates:
 * @code
 * Data Type: bool
 * True
 * @endcode
 *
 * **Use in Conditional Logic:**
 * @code
 * // Enable/disable processing
 * if (enableData->data()) {
 *     processImage();
 * }
 * 
 * // Toggle state
 * toggleData->set_data(!toggleData->data());
 * @endcode
 *
 * **Best Practices:**
 * - Use set_data() instead of direct reference for timestamp updates
 * - Use state_str() for UI display and logging
 * - Ideal for binary state representation
 * - Consider IntegerData for multi-state enumerations
 *
 * @note Default construction initializes to true, not false.
 * @note Direct modification via data() reference does not update timestamp.
 *
 * @see InformationData for base class functionality
 * @see IntegerData for numeric values
 */
class BoolData : public InformationData
{
public:
    /**
     * @brief Default constructor initializing value to true.
     *
     * Creates a BoolData instance with value true.
     *
     * @note Default is true, not false. Specify false explicitly if needed.
     */
    BoolData()
        : mbBool(true)
    {
    }

    /**
     * @brief Constructor with initial value.
     * @param state Initial boolean value.
     *
     * Creates a BoolData instance with the specified state.
     */
    BoolData( const bool state )
        : mbBool(state)
    {}

    /**
     * @brief Returns the data type information.
     * @return NodeDataType with name "Boolean" and display "Bln".
     *
     * Provides type identification for the node system's type checking
     * and connection validation.
     */
    NodeDataType
    type() const override
    {
        return { "Boolean", "Bln" };
    }

    /**
     * @brief Returns a reference to the boolean value.
     * @return Reference to the internal boolean data.
     *
     * Provides direct access to the boolean value. Modifying through
     * this reference does NOT update the timestamp.
     *
     * **Warning:** Direct modification bypasses timestamp updates.
     * Use set_data() when timestamp tracking is important.
     *
     * @code
     * bool val = data->data();       // Read
     * data->data() = true;           // Write (no timestamp update)
     * data->data() = !data->data();  // Toggle (no timestamp update)
     * @endcode
     */
    bool&
    data()
    {
        return mbBool;
    }

    /**
     * @brief Sets the boolean value and updates timestamp.
     * @param data New boolean value.
     *
     * Updates the stored value and automatically updates the timestamp
     * from the InformationData base class. This is the preferred method
     * for modifying the value when timestamp tracking is needed.
     *
     * @code
     * data->set_data(true);   // Sets to true with timestamp
     * data->set_data(false);  // Sets to false with timestamp
     * @endcode
     */
    void
    set_data(bool data)
    {
        mbBool = data;
        InformationData::set_timestamp();
    }

    /**
     * @brief Returns string representation of the state.
     * @return QString "True" if true, "False" if false.
     *
     * Provides a human-readable string representation suitable
     * for UI display, logging, or debugging.
     *
     * @code
     * BoolData data(true);
     * qDebug() << data.state_str();  // Outputs: "True"
     * @endcode
     */
    QString
    state_str() const
    {
        return mbBool? QString("True") : QString("False");
    }

    /**
     * @brief Generates formatted information string.
     *
     * Creates a human-readable string representation of the data
     * for display in debug views or information panels.
     *
     * **Format:**
     * @code
     * Data Type: bool
     * <True|False>
     * @endcode
     *
     * Example output for true:
     * @code
     * Data Type: bool
     * True
     * @endcode
     */
    void set_information() override
    {
        mQSData  = QString("Data Type: bool \n");
        mQSData += state_str();
        mQSData += QString("\n");
    }

private:
    /**
     * @brief The stored boolean value.
     *
     * Internal storage for the boolean data. Access through data()
     * or set_data() methods.
     */
    bool mbBool;

};
