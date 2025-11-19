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
 * @file IntegerData.hpp
 * @brief Integer data type for node-based dataflow communication.
 *
 * This file defines the IntegerData class, which encapsulates integer values
 * for transmission between nodes in the CVDev dataflow graph system.
 *
 * **Key Features:**
 * - **Integer Storage:** Stores 32-bit signed integer values
 * - **Timestamping:** Automatic timestamp on data updates
 * - **Type Safety:** Provides type identification for the node system
 * - **Information Display:** Formatted text representation for debugging
 *
 * **Common Use Cases:**
 * - Counter values and iteration indices
 * - Image dimensions (width, height)
 * - Threshold values for image processing
 * - Boolean flags represented as integers (0/1)
 * - Enumeration values
 * - Click counts from user interactions
 *
 * **Dataflow Patterns:**
 * @code
 * // Counter pattern
 * TimerNode → CounterNode → [IntegerData] → DisplayNode
 * 
 * // Threshold control
 * SliderNode → [IntegerData] → ThresholdingNode → [Image]
 * 
 * // Dimension extraction
 * ImageNode → PropertiesNode → [IntegerData(width)] → ProcessingNode
 * @endcode
 *
 * **Value Range:**
 * - Type: int (typically 32-bit signed)
 * - Range: -2,147,483,648 to 2,147,483,647
 *
 * @see InformationData
 * @see DoubleData
 * @see BoolData
 */

#pragma once

#include <QtNodes/NodeData>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class IntegerData
 * @brief Integer data container for dataflow graph nodes.
 *
 * Encapsulates a single integer value with automatic timestamping and
 * type identification for use in the CVDev node-based visual programming system.
 *
 * **Data Properties:**
 * - **Type Name:** "Integer"
 * - **Display Name:** "Int"
 * - **Storage:** 32-bit signed integer (int)
 * - **Timestamping:** Automatic on data modification
 *
 * **Construction Examples:**
 * @code
 * // Default constructor (value = 0)
 * auto data1 = std::make_shared<IntegerData>();
 * 
 * // Initialize with value
 * auto data2 = std::make_shared<IntegerData>(42);
 * 
 * // Set value after construction
 * auto data3 = std::make_shared<IntegerData>();
 * data3->set_data(100);
 * @endcode
 *
 * **Access Patterns:**
 * @code
 * // Read value
 * int value = data->data();
 * 
 * // Modify value (updates timestamp)
 * data->set_data(newValue);
 * 
 * // Direct reference access
 * data->data() = 42;  // Does NOT update timestamp
 * @endcode
 *
 * **Information Display:**
 * The set_information() method generates a formatted string:
 * @code
 * Data Type : int
 * 42
 * @endcode
 *
 * **Timestamp Behavior:**
 * - Updated automatically when set_data() is called
 * - Inherited from InformationData base class
 * - Useful for detecting data freshness in dataflow
 *
 * **Best Practices:**
 * - Use set_data() instead of direct reference to ensure timestamp updates
 * - Check timestamp when data freshness matters
 * - Use for discrete values, not continuous floating-point
 * - Consider DoubleData for non-integer numeric values
 *
 * @note Direct modification via data() reference does not update timestamp.
 * @note Thread safety depends on usage context - use appropriate locking if needed.
 *
 * @see InformationData for base class functionality
 * @see DoubleData for floating-point values
 * @see BoolData for boolean values
 */
class IntegerData : public InformationData
{
public:
    /**
     * @brief Default constructor initializing value to zero.
     *
     * Creates an IntegerData instance with value 0.
     */
    IntegerData()
        : miData( 0 )
    {}

    /**
     * @brief Constructor with initial value.
     * @param data Initial integer value.
     *
     * Creates an IntegerData instance with the specified value.
     */
    IntegerData( const int data )
        : miData( data )
    {}

    /**
     * @brief Returns the data type information.
     * @return NodeDataType with name "Integer" and display "Int".
     *
     * Provides type identification for the node system's type checking
     * and connection validation.
     */
    NodeDataType
    type() const override
    {
        return { "Integer", "Int" };
    }

    /**
     * @brief Returns a reference to the integer value.
     * @return Reference to the internal integer data.
     *
     * Provides direct access to the integer value. Modifying through
     * this reference does NOT update the timestamp.
     *
     * **Warning:** Direct modification bypasses timestamp updates.
     * Use set_data() when timestamp tracking is important.
     *
     * @code
     * int val = data->data();        // Read
     * data->data() = 42;             // Write (no timestamp update)
     * data->data() += 1;             // Increment (no timestamp update)
     * @endcode
     */
    int &
    data()
    {
        return miData;
    }

    /**
     * @brief Sets the integer value and updates timestamp.
     * @param data New integer value.
     *
     * Updates the stored value and automatically updates the timestamp
     * from the InformationData base class. This is the preferred method
     * for modifying the value when timestamp tracking is needed.
     *
     * @code
     * data->set_data(100);  // Sets value and updates timestamp
     * @endcode
     */
    void
    set_data( int data )
    {
        miData = data;
        InformationData::set_timestamp();
    }

    /**
     * @brief Generates formatted information string.
     *
     * Creates a human-readable string representation of the data
     * for display in debug views or information panels.
     *
     * **Format:**
     * @code
     * Data Type : int
     * <value>
     * @endcode
     *
     * Example output:
     * @code
     * Data Type : int
     * 42
     * @endcode
     */
    void set_information() override
    {
        mQSData = QString("Data Type : int \n");
        mQSData += QString::number(miData);
        mQSData += QString("\n");
    }

private:
    /**
     * @brief The stored integer value.
     *
     * Internal storage for the integer data. Access through data()
     * or set_data() methods.
     */
    int miData;
};
