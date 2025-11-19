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
 * @file DoubleData.hpp
 * @brief Double-precision floating-point data type for dataflow communication.
 *
 * This file defines the DoubleData class, which encapsulates double-precision
 * floating-point values for transmission between nodes in the CVDev dataflow system.
 *
 * **Key Features:**
 * - **Double Precision:** 64-bit IEEE 754 floating-point storage
 * - **High Precision:** ~15-17 decimal digits of precision
 * - **Wide Range:** ±1.7E±308 (approximately)
 * - **Type Safety:** Provides type identification for the node system
 *
 * **Common Use Cases:**
 * - Scientific calculations requiring high precision
 * - Angle measurements (radians, degrees)
 * - Scale factors and transformation parameters
 * - Statistical computations (mean, variance, etc.)
 * - Physical measurements with decimal precision
 * - Mathematical constants (π, e, etc.)
 *
 * **Dataflow Patterns:**
 * @code
 * // Angle conversion
 * AngleInputNode → [DoubleData] → RotationNode → [Image]
 * 
 * // Scale factor
 * SliderNode → [DoubleData(0.5)] → ResizeNode → [Image]
 * 
 * // Calculation result
 * MathNode → [DoubleData] → DisplayNode
 * @endcode
 *
 * **Precision Comparison:**
 * - float: ~7 decimal digits
 * - double: ~15-17 decimal digits
 * - Use DoubleData when precision matters
 * - Use FloatData when memory/speed is critical
 *
 * @see InformationData
 * @see FloatData
 * @see IntegerData
 */

#pragma once

#include <QtNodes/NodeData>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class DoubleData
 * @brief Double-precision floating-point data container for dataflow nodes.
 *
 * Encapsulates a single double-precision floating-point value with
 * type identification for use in the CVDev node-based visual programming system.
 *
 * **Data Properties:**
 * - **Type Name:** "Double"
 * - **Display Name:** "Dbl"
 * - **Storage:** double (64-bit IEEE 754)
 * - **Precision:** ~15-17 decimal digits
 * - **Range:** ±1.7E±308
 *
 * **Construction Examples:**
 * @code
 * // Default constructor (value = 0.0)
 * auto data1 = std::make_shared<DoubleData>();
 * 
 * // Initialize with value
 * auto data2 = std::make_shared<DoubleData>(3.14159);
 * 
 * // Scientific notation
 * auto data3 = std::make_shared<DoubleData>(1.23e-5);
 * @endcode
 *
 * **Access Patterns:**
 * @code
 * // Read value
 * double value = data->data();
 * 
 * // Direct reference modification
 * data->data() = 2.71828;
 * data->data() += 0.5;
 * data->data() *= 1.5;
 * @endcode
 *
 * **Mathematical Operations:**
 * @code
 * // Use in calculations
 * double result = std::sin(angleData->data());
 * double scaled = imageData->width() * scaleData->data();
 * 
 * // Comparison
 * if (thresholdData->data() > 0.5) {
 *     // Process
 * }
 * @endcode
 *
 * **Information Display:**
 * The set_information() method generates:
 * @code
 * Data Type : double
 * 3.14159
 * @endcode
 *
 * **Precision Considerations:**
 * - Sufficient for most scientific computations
 * - Preferred over float for accumulation (less rounding error)
 * - Exact representation for integers up to 2^53
 * - Use with care for equality comparisons (epsilon tolerance)
 *
 * **Best Practices:**
 * @code
 * // Good: Epsilon comparison for equality
 * const double EPSILON = 1e-9;
 * if (std::abs(data1->data() - data2->data()) < EPSILON) {
 *     // Values are "equal"
 * }
 * 
 * // Avoid: Direct equality comparison
 * if (data1->data() == data2->data()) {  // Risky with floating-point
 *     // May fail due to rounding
 * }
 * @endcode
 *
 * @note No timestamp tracking (unlike IntegerData/BoolData).
 * @note Direct reference access is the primary access pattern.
 *
 * @see InformationData for base class functionality
 * @see FloatData for single-precision alternative
 * @see IntegerData for integer values
 */
class DoubleData : public InformationData
{
public:
    /**
     * @brief Default constructor initializing value to zero.
     *
     * Creates a DoubleData instance with value 0.0.
     */
    DoubleData()
        : mdData( 0 )
    {}

    /**
     * @brief Constructor with initial value.
     * @param data Initial double-precision value.
     *
     * Creates a DoubleData instance with the specified value.
     */
    DoubleData( const double data )
        : mdData( data )
    {}

    /**
     * @brief Returns the data type information.
     * @return NodeDataType with name "Double" and display "Dbl".
     *
     * Provides type identification for the node system's type checking
     * and connection validation.
     */
    NodeDataType
    type() const override
    {
        return { "Double", "Dbl" };
    }

    /**
     * @brief Returns a reference to the double value.
     * @return Reference to the internal double data.
     *
     * Provides direct access to the double-precision value for
     * reading and modification.
     *
     * **Usage Examples:**
     * @code
     * double val = data->data();     // Read
     * data->data() = 3.14;           // Write
     * data->data() += 1.0;           // Increment
     * data->data() *= 2.0;           // Scale
     * 
     * // Use in calculations
     * double area = data->data() * data->data();
     * @endcode
     */
    double &
    data()
    {
        return mdData;
    }

    /**
     * @brief Generates formatted information string.
     *
     * Creates a human-readable string representation of the data
     * for display in debug views or information panels.
     *
     * **Format:**
     * @code
     * Data Type : double
     * <value>
     * @endcode
     *
     * Example output:
     * @code
     * Data Type : double
     * 3.14159
     * @endcode
     *
     * @note Qt's QString::number() determines the precision automatically.
     */
    void set_information() override
    {
        mQSData = QString("Data Type : double \n");
        mQSData += QString::number(mdData);
        mQSData += QString("\n");
    }

private:
    /**
     * @brief The stored double-precision value.
     *
     * Internal storage for the floating-point data. Access through
     * data() method.
     */
    double mdData;
};
