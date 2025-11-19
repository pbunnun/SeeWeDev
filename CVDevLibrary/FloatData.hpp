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
 * @file FloatData.hpp
 * @brief Single-precision floating-point data type for dataflow communication.
 *
 * This file defines the FloatData class, which encapsulates single-precision
 * floating-point values for transmission between nodes in the CVDev dataflow system.
 *
 * **Key Features:**
 * - **Single Precision:** 32-bit IEEE 754 floating-point storage
 * - **Moderate Precision:** ~6-9 decimal digits of precision
 * - **Memory Efficient:** Half the size of double (32-bit vs 64-bit)
 * - **Faster Processing:** Better cache performance, faster in some operations
 *
 * **Common Use Cases:**
 * - Graphics and rendering calculations
 * - Real-time processing where speed matters
 * - Color values and pixel manipulations
 * - OpenCV operations (many use float internally)
 * - Neural network weights and activations
 * - Memory-constrained applications
 *
 * **Dataflow Patterns:**
 * @code
 * // Color channel value (0.0 - 1.0)
 * ColorPickerNode → [FloatData] → ColorAdjustNode → [Image]
 * 
 * // Confidence score
 * DetectorNode → [FloatData(confidence)] → ThresholdNode
 * 
 * // Fast calculations
 * VideoProcessing → [FloatData] → RealtimeFilter → [Output]
 * @endcode
 *
 * **Float vs Double Trade-offs:**
 * - **Float:** Faster, less memory, sufficient for graphics
 * - **Double:** More precise, better for accumulation, scientific computing
 * - Use FloatData when: Speed matters, memory is limited, ~7 digits enough
 * - Use DoubleData when: Precision critical, accumulation needed
 *
 * @see InformationData
 * @see DoubleData
 * @see IntegerData
 */

#pragma once

#include <QtNodes/NodeData>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class FloatData
 * @brief Single-precision floating-point data container for dataflow nodes.
 *
 * Encapsulates a single single-precision floating-point value with
 * type identification for use in the CVDev node-based visual programming system.
 *
 * **Data Properties:**
 * - **Type Name:** "Float"
 * - **Display Name:** "Flt"
 * - **Storage:** float (32-bit IEEE 754)
 * - **Precision:** ~6-9 decimal digits
 * - **Range:** ±3.4E±38
 * - **Memory:** 4 bytes (half of double)
 *
 * **Construction Examples:**
 * @code
 * // Default constructor (value = 0.0f)
 * auto data1 = std::make_shared<FloatData>();
 * 
 * // Initialize with value
 * auto data2 = std::make_shared<FloatData>(1.5f);
 * 
 * // From double (implicit conversion)
 * auto data3 = std::make_shared<FloatData>(3.14);  // Converts to float
 * @endcode
 *
 * **Access Patterns:**
 * @code
 * // Read value
 * float value = data->data();
 * 
 * // Direct reference modification
 * data->data() = 0.5f;
 * data->data() += 0.1f;
 * data->data() *= 2.0f;
 * @endcode
 *
 * **OpenCV Integration:**
 * @code
 * // Many OpenCV operations use float
 * cv::Mat floatMat(100, 100, CV_32F);
 * floatMat.at<float>(0, 0) = scaleData->data();
 * 
 * // Normalization (0.0 - 1.0 range)
 * cv::normalize(input, output, 0, 1, cv::NORM_MINMAX, CV_32F);
 * float normalizedValue = data->data();
 * @endcode
 *
 * **Information Display:**
 * The set_information() method generates:
 * @code
 * Data Type : float
 * 1.5
 * @endcode
 *
 * **Performance Considerations:**
 * - Faster than double on many architectures
 * - Better cache utilization (smaller size)
 * - Preferred for GPU operations
 * - Sufficient precision for most graphics tasks
 * - May accumulate rounding errors faster than double
 *
 * **Best Practices:**
 * @code
 * // Use 'f' suffix for float literals
 * data->data() = 1.5f;     // Good: float literal
 * data->data() = 1.5;      // OK: implicit conversion from double
 * 
 * // Epsilon comparison for equality
 * const float EPSILON = 1e-6f;
 * if (std::abs(data1->data() - data2->data()) < EPSILON) {
 *     // Values are "equal"
 * }
 * @endcode
 *
 * **When to Use Float vs Double:**
 * - **Use Float when:** Speed critical, memory limited, graphics/rendering
 * - **Use Double when:** Precision critical, scientific computing, accumulation
 *
 * @note No timestamp tracking (unlike IntegerData/BoolData).
 * @note Direct reference access is the primary access pattern.
 *
 * @see InformationData for base class functionality
 * @see DoubleData for double-precision alternative
 * @see IntegerData for integer values
 */
class FloatData : public InformationData
{
public:
    /**
     * @brief Default constructor initializing value to zero.
     *
     * Creates a FloatData instance with value 0.0f.
     */
    FloatData()
        : mfData( 0 )
    {}

    /**
     * @brief Constructor with initial value.
     * @param data Initial single-precision value.
     *
     * Creates a FloatData instance with the specified value.
     */
    FloatData( const float data )
        : mfData( data )
    {}

    /**
     * @brief Returns the data type information.
     * @return NodeDataType with name "Float" and display "Flt".
     *
     * Provides type identification for the node system's type checking
     * and connection validation.
     */
    NodeDataType
    type() const override
    {
        return { "Float", "Flt" };
    }

    /**
     * @brief Returns a reference to the float value.
     * @return Reference to the internal float data.
     *
     * Provides direct access to the single-precision value for
     * reading and modification.
     *
     * **Usage Examples:**
     * @code
     * float val = data->data();      // Read
     * data->data() = 1.5f;           // Write
     * data->data() += 0.1f;          // Increment
     * data->data() *= 2.0f;          // Scale
     * 
     * // Use with OpenCV
     * cv::Mat mat(10, 10, CV_32F, cv::Scalar(data->data()));
     * @endcode
     */
    float &
    data()
    {
        return mfData;
    }

    /**
     * @brief Generates formatted information string.
     *
     * Creates a human-readable string representation of the data
     * for display in debug views or information panels.
     *
     * **Format:**
     * @code
     * Data Type : float
     * <value>
     * @endcode
     *
     * Example output:
     * @code
     * Data Type : float
     * 1.5
     * @endcode
     *
     * @note Qt's QString::number() determines the precision automatically.
     */
    void set_information() override
    {
        mQSData = QString("Data Type : float \n");
        mQSData += QString::number(mfData);
        mQSData += QString("\n");
    }

private:
    /**
     * @brief The stored single-precision value.
     *
     * Internal storage for the floating-point data. Access through
     * data() method.
     */
    float mfData;
};
