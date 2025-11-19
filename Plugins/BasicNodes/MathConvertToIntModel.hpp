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
 * @file MathConvertToIntModel.hpp
 * @brief Converts floating-point or text data to integer values.
 *
 * This utility node extracts numeric values from InformationData (text strings)
 * and converts them to integer format (IntegerData). It's used for type conversion
 * in pipelines where downstream nodes require integer input.
 *
 * **Key Features:**
 * - Parses numeric strings to integers
 * - Handles InformationData input (text-based data)
 * - Outputs IntegerData (int type)
 * - Automatic type conversion and rounding
 *
 * **Typical Use Cases:**
 * - Convert text-based measurements to integers
 * - Extract numeric values from information displays
 * - Type conversion for math operations
 * - Prepare data for integer-based nodes (counters, indices)
 *
 * @see IntegerData for output type
 * @see InformationData for input type
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPlainTextEdit>

#include "PBNodeDelegateModel.hpp"
#include "IntegerData.hpp"
#include "InformationData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class MathConvertToIntModel
 * @brief Converts InformationData (text) to IntegerData.
 *
 * This simple converter node takes text-based numeric data (InformationData)
 * and outputs it as integer type (IntegerData). It performs string parsing
 * and type conversion automatically.
 *
 * **Port Configuration:**
 * - **Input:** InformationData - Text containing numeric value (e.g., "42", "123.7")
 * - **Output:** IntegerData - Parsed integer value
 *
 * **Conversion Logic:**
 * ```cpp
 * QString text = input_information->data();  // e.g., "42.8"
 * int value = text.toInt();                   // Result: 42 (truncates decimal)
 * // Alternative for rounding:
 * // int value = qRound(text.toDouble());     // Result: 43 (rounds)
 * 
 * output_integer->data() = value;
 * ```
 *
 * **Common Use Cases:**
 *
 * 1. **Counter Display to Integer:**
 *    ```
 *    ObjectCount → InformationDisplay → MathConvertToInt → Comparison
 *    ```
 *
 * 2. **User Input Conversion:**
 *    ```
 *    TextInput("42") → MathConvertToInt → MathOperation
 *    ```
 *
 * 3. **Pipeline Index Control:**
 *    ```
 *    FrameNumber (string) → MathConvertToInt → ArrayIndex
 *    ```
 *
 * 4. **Type Compatibility:**
 *    ```
 *    InformationData (area="250") → MathConvertToInt → MathCondition(> 200)
 *    ```
 *
 * **Conversion Behavior:**
 * - **Integer Strings**: "42" → 42 (direct conversion)
 * - **Floating-Point Strings**: "42.7" → 42 (truncates, does NOT round)
 * - **Invalid Strings**: "abc" → 0 (Qt default behavior)
 * - **Mixed Strings**: "42abc" → 42 (parses leading digits, stops at non-digit)
 * - **Empty Strings**: "" → 0
 * - **Negative Values**: "-15" → -15 (preserves sign)
 *
 * **Rounding vs Truncation:**
 * Current implementation uses truncation (floor for positive, ceil for negative).
 * For rounding behavior:
 * ```cpp
 * // Truncation (current):
 * "42.7" → 42
 * "42.3" → 42
 * "-42.7" → -42
 *
 * // If rounding implemented:
 * "42.7" → 43
 * "42.3" → 42
 * "-42.7" → -43
 * ```
 *
 * **Performance:**
 * - Conversion time: O(n) where n = string length (typically < 1μs)
 * - No buffering or state
 * - Negligible overhead
 *
 * **Limitations:**
 * - Truncates decimals (no rounding)
 * - Limited error handling for invalid input
 * - No output range clamping (can overflow for very large strings)
 * - No indication of conversion failure
 *
 * **Design Rationale:**
 * - Simple passthrough conversion for type compatibility
 * - Minimal node complexity (no embedded widget)
 * - Relies on Qt's robust QString::toInt() implementation
 *
 * @note For floating-point preservation, use DoubleData instead of IntegerData
 * @note Invalid inputs silently convert to 0 (Qt default behavior)
 * @see InformationData for input text data type
 * @see IntegerData for output integer data type
 */
class MathConvertToIntModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    MathConvertToIntModel();

    virtual
    ~MathConvertToIntModel() override {}

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

    std::shared_ptr< InformationData > mpInformationData; ///< Input text data
    std::shared_ptr< IntegerData > mpIntegerData;         ///< Output integer data
};

