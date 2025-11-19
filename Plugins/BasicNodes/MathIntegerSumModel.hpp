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
 * @file MathIntegerSumModel.hpp
 * @brief Adds two integer values.
 *
 * This simple arithmetic node computes the sum of two integer inputs and outputs
 * the result. It's used for basic mathematical operations in pipelines, such as
 * accumulating counts, combining measurements, or offset calculations.
 *
 * **Key Features:**
 * - Two integer inputs (addends)
 * - Single integer output (sum)
 * - No overflow checking (wraps at INT_MAX/INT_MIN)
 * - Minimal latency (direct addition)
 *
 * **Typical Use Cases:**
 * - Accumulate object counts from multiple sources
 * - Combine measurements or metrics
 * - Calculate total quantities
 * - Offset or bias adjustments
 *
 * @see IntegerData for input/output type
 * @see MathConvertToIntModel for type conversion
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPlainTextEdit>

#include "PBNodeDelegateModel.hpp"
#include "IntegerData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class MathIntegerSumModel
 * @brief Computes the sum of two integer inputs.
 *
 * MathIntegerSumModel performs simple integer addition: output = input1 + input2.
 * It's a basic arithmetic building block for mathematical pipelines.
 *
 * **Port Configuration:**
 * - **Inputs:**
 *   - Port 0: IntegerData - First addend
 *   - Port 1: IntegerData - Second addend
 * - **Output:**
 *   - Port 0: IntegerData - Sum (input1 + input2)
 *
 * **Operation:**
 * ```cpp
 * int a = input_port_0->data();  // First integer
 * int b = input_port_1->data();  // Second integer
 * int sum = a + b;               // Compute sum
 * output->data() = sum;
 * ```
 *
 * **Common Use Cases:**
 *
 * 1. **Multi-Source Counting:**
 *    ```
 *    ObjectCount1 → MathIntegerSum ← ObjectCount2
 *                        ↓
 *                   Total Count
 *    ```
 *
 * 2. **Accumulation:**
 *    ```
 *    CurrentTotal → MathIntegerSum ← NewValue
 *                        ↓
 *                   Updated Total
 *    ```
 *
 * 3. **Offset Calculation:**
 *    ```
 *    BaseValue → MathIntegerSum ← Offset
 *                     ↓
 *               Adjusted Value
 *    ```
 *
 * 4. **Combining Measurements:**
 *    ```
 *    AreaRegion1 → MathIntegerSum ← AreaRegion2
 *                       ↓
 *                  Total Area
 *    ```
 *
 * 5. **Counter Increment:**
 *    ```
 *    Counter → MathIntegerSum ← Constant(1)
 *                   ↓
 *              Counter + 1
 *    ```
 *
 * **Behavior Details:**
 * - **Overflow**: Standard C++ integer overflow (wraps at INT_MAX/INT_MIN)
 *   - Example: 2147483647 + 1 = -2147483648 (wraps to INT_MIN)
 * - **Null Inputs**: If either input is null, output is null (no partial sum)
 * - **Both Inputs Required**: Waits for both inputs before computing
 * - **No Saturation**: Does not clamp at INT_MAX (use separate clamping node if needed)
 *
 * **Performance:**
 * - Computation: O(1) - single integer addition
 * - Latency: < 1μs (negligible)
 * - No state or buffering
 *
 * **Limitations:**
 * - Integer overflow wraps (no overflow detection)
 * - No support for floating-point addition (use DoubleData nodes instead)
 * - No multi-input accumulation (chain multiple MathIntegerSum nodes for >2 inputs)
 *
 * **Extensions:**
 * For more complex arithmetic, chain multiple nodes:
 * ```
 * // Compute: (A + B) + C
 * A → Sum1 ← B
 *      ↓
 *    Sum2 ← C
 *      ↓
 *   Result
 * ```
 *
 * **Design Rationale:**
 * - Simple, single-purpose node (Unix philosophy)
 * - No embedded widget (parameters would be redundant)
 * - Minimal overhead for performance-critical pipelines
 * - Type-safe (only accepts IntegerData)
 *
 * @note For overflow-safe addition, check input ranges before summing
 * @note For floating-point math, use DoubleData instead of IntegerData
 * @see IntegerData for input/output data type
 */
class MathIntegerSumModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    MathIntegerSumModel();

    virtual
    ~MathIntegerSumModel() override {}

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    bool
    resizable() const override { return false; }

    static const QString _category;

    static const QString _model_name;

private:

    std::shared_ptr< IntegerData > mpIntegerData_1; ///< First input (addend 1)
    std::shared_ptr< IntegerData > mpIntegerData_2; ///< Second input (addend 2)
    std::shared_ptr< IntegerData > mpIntegerData;   ///< Output (sum)
};

