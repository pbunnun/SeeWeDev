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
 * @file ScalarOperationModel.hpp
 * @brief Model for performing binary operations on scalar values.
 *
 * This file defines the ScalarOperationModel class for arithmetic, comparison, and logical
 * operations on scalar data types (integers, floats, doubles, booleans). It supports a
 * comprehensive set of operators including mathematical operations, relational comparisons,
 * and bitwise/logical operations, with automatic type handling and result type inference.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"
#include "IntegerData.hpp"
#include "FloatData.hpp"
#include "DoubleData.hpp"
#include "BoolData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct SclOps
 * @brief Enumeration of scalar operation types.
 *
 * Defines all supported binary operations for scalar values, categorized into
 * arithmetic, relational, and logical operators.
 */
struct SclOps
{
    /**
     * @enum Operators
     * @brief Supported scalar operation types.
     *
     * **Arithmetic Operations:**
     * - PLUS: Addition (a + b)
     * - MINUS: Subtraction (a - b)
     * - MULTIPLY: Multiplication (a × b)
     * - DIVIDE: Division (a ÷ b)
     *
     * **Relational Operations (return boolean):**
     * - GREATER_THAN: a > b
     * - GREATER_THAN_OR_EQUAL: a >= b
     * - LESSER_THAN: a < b
     * - LESSER_THAN_OR_EQUAL: a <= b
     * - EQUAL: a == b
     *
     * **Min/Max Operations:**
     * - MAXIMUM: max(a, b)
     * - MINIMUM: min(a, b)
     *
     * **Logical Operations (boolean inputs):**
     * - AND: a && b
     * - OR: a || b
     * - XOR: a ^ b (exclusive OR)
     * - NOR: !(a || b) (NOT OR)
     * - NAND: !(a && b) (NOT AND)
     */
    enum Operators
    {
        PLUS = 0,                     ///< Addition: a + b
        MINUS = 1,                    ///< Subtraction: a - b
        GREATER_THAN = 2,             ///< Comparison: a > b
        GREATER_THAN_OR_EQUAL = 3,    ///< Comparison: a >= b
        LESSER_THAN = 4,              ///< Comparison: a < b
        LESSER_THAN_OR_EQUAL = 5,     ///< Comparison: a <= b
        MULTIPLY = 6,                 ///< Multiplication: a × b
        DIVIDE = 7,                   ///< Division: a ÷ b
        MAXIMUM = 8,                  ///< Maximum: max(a, b)
        MINIMUM = 9,                  ///< Minimum: min(a, b)
        EQUAL = 10,                   ///< Equality: a == b
        AND = 11,                     ///< Logical AND: a && b
        OR = 12,                      ///< Logical OR: a || b
        XOR = 13,                     ///< Logical XOR: a ^ b
        NOR = 14,                     ///< Logical NOR: !(a || b)
        NAND = 15                     ///< Logical NAND: !(a && b)
    };
};

/**
 * @struct ScalarOperationParameters
 * @brief Configuration parameters for scalar operations.
 *
 * Stores the selected operation type from SclOps::Operators enum.
 */
typedef struct ScalarOperationParameters{
    int miOperator;  ///< Selected operator (SclOps::Operators value)
    
    /**
     * @brief Default constructor.
     *
     * Initializes with PLUS (addition) as default operation.
     */
    ScalarOperationParameters()
        : miOperator(SclOps::PLUS)
    {
    }
} ScalarOperationParameters;

/**
 * @class ScalarOperationModel
 * @brief Node model for binary operations on scalar data.
 *
 * This model performs binary operations on two scalar input values, supporting arithmetic,
 * relational, and logical operations. It handles type inference automatically, producing
 * integer, float, double, or boolean results based on the operation and input types.
 *
 * **Input Ports:**
 * 1. **InformationData** - First operand (can be IntegerData, FloatData, DoubleData, BoolData)
 * 2. **InformationData** - Second operand (can be IntegerData, FloatData, DoubleData, BoolData)
 *
 * **Output Ports:**
 * 1. **InformationData** - Result (type depends on operation):
 *    - Arithmetic (PLUS, MINUS, MULTIPLY, DIVIDE, MAX, MIN): Same type as inputs
 *    - Relational (GT, GTE, LT, LTE, EQUAL): BoolData
 *    - Logical (AND, OR, XOR, NOR, NAND): BoolData
 *
 * **Operation Categories:**
 *
 * 1. **Arithmetic Operations:**
 *    - PLUS: result = a + b
 *    - MINUS: result = a - b
 *    - MULTIPLY: result = a × b
 *    - DIVIDE: result = a ÷ b (returns 0 if b == 0)
 *
 * 2. **Comparison Operations (return boolean):**
 *    - GREATER_THAN: result = (a > b)
 *    - GREATER_THAN_OR_EQUAL: result = (a >= b)
 *    - LESSER_THAN: result = (a < b)
 *    - LESSER_THAN_OR_EQUAL: result = (a <= b)
 *    - EQUAL: result = (a == b)
 *
 * 3. **Min/Max Operations:**
 *    - MAXIMUM: result = max(a, b)
 *    - MINIMUM: result = min(a, b)
 *
 * 4. **Logical Operations (boolean inputs):**
 *    - AND: result = a && b (true if both true)
 *    - OR: result = a || b (true if either true)
 *    - XOR: result = a ^ b (true if exactly one true)
 *    - NOR: result = !(a || b) (true if both false)
 *    - NAND: result = !(a && b) (false if both true)
 *
 * **Type Handling:**
 * The model uses template-based type casting via info_pointer_cast() to automatically
 * convert results to the appropriate InformationData subtype (IntegerData, FloatData,
 * DoubleData, BoolData) based on the operation result type.
 *
 * **Properties (Configurable):**
 * - **operator:** Selected operation (SclOps::Operators enum value)
 *
 * **Use Cases:**
 * - Add constant offset to sensor values
 * - Calculate ratios and percentages
 * - Threshold detection (value > limit)
 * - Range validation (min <= value <= max with two nodes)
 * - Boolean logic for multi-condition triggers
 * - Statistical calculations (average = (a + b) / 2)
 * - Unit conversions (Celsius to Fahrenheit, etc.)
 *
 * **Example Workflows:**
 * @code
 * // Temperature threshold detection
 * [Sensor (FloatData: 25.5)] ----\
 *                                 [ScalarOperation: GREATER_THAN] -> [BoolData: false]
 * [Threshold (FloatData: 30.0)] -/
 * 
 * // Calculate average of two values
 * [Value1 (DoubleData: 10.0)] ---\
 *                                [ScalarOperation: PLUS] -> [Sum: 22.0]
 * [Value2 (DoubleData: 12.0)] --/                               |
 *                                                                v
 *                                                    [ScalarOperation: DIVIDE]
 *                                                                |
 *                                        [Constant: 2.0] -------/
 *                                                                |
 *                                                                v
 *                                                        [Average: 11.0]
 * 
 * // Boolean logic: Trigger only if both conditions met
 * [Temp > 25] (BoolData) ----\
 *                            [ScalarOperation: AND] -> [BoolData: true/false]
 * [Pressure < 1000] (Bool) -/
 * @endcode
 *
 * **Special Cases:**
 * - Division by zero: Returns 0
 * - Logical operations on non-boolean: Interprets 0 as false, non-zero as true
 * - Type promotion: Mixing int and float promotes to float
 *
 * @see InformationData
 * @see IntegerData
 * @see FloatData
 * @see DoubleData
 * @see BoolData
 */
class ScalarOperationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a ScalarOperationModel.
     *
     * Initializes with default PLUS operation.
     */
    ScalarOperationModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~ScalarOperationModel() override {}

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing operator selection.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved operator.
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 2 for input (two operands), 1 for output (result).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return InformationData for all ports.
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the operation result.
     * @param port Port index (0).
     * @return Shared pointer to result InformationData (type depends on operation).
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and triggers calculation.
     * @param nodeData Input operand (InformationData subtype).
     * @param Port index (0 or 1).
     *
     * When both inputs are available, performs the selected operation and
     * updates the output data.
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief Returns nullptr (no embedded widget).
     * @return nullptr.
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets a model property.
     * @param Property name ("operator").
     * @param QVariant value (int from SclOps::Operators).
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Returns the minimum node icon.
     * @return QPixmap icon.
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private:
    ScalarOperationParameters mParams;                            ///< Operation configuration
    std::shared_ptr<InformationData> mpInformationData { nullptr }; ///< Output result
    std::shared_ptr<InformationData> mapInformationInData[2] {{ nullptr }}; ///< Input operands
    QPixmap _minPixmap;                                           ///< Node icon

    /**
     * @brief Processes input data and performs the operation.
     * @param in Array of two input InformationData pointers.
     * @param out Output InformationData pointer (result).
     * @param params Operation parameters (selected operator).
     *
     * Extracts numeric/boolean values from inputs, applies the operation,
     * and creates the appropriate output data type.
     */
    void processData(std::shared_ptr<InformationData> (&in)[2], std::shared_ptr< InformationData > & out,
                      const ScalarOperationParameters & params );

    /**
     * @brief Template function for casting results to appropriate InformationData type.
     * @tparam T Result type (int, float, double, bool).
     * @param result Computed result value.
     * @param info Output InformationData shared pointer (set to correct subtype).
     *
     * Uses typeid to determine result type and creates the corresponding
     * InformationData subclass (IntegerData, FloatData, DoubleData, BoolData).
     *
     * **Example:**
     * @code
     * double result = a + b;
     * info_pointer_cast(result, mpInformationData); // Creates DoubleData
     * @endcode
     */
    template<typename T>
    void info_pointer_cast(const T result, std::shared_ptr<InformationData>& info)
    {
        const std::string type = typeid(result).name();
        if(type == "int")
        {
            auto d = std::make_shared<IntegerData>(result);
            info = std::static_pointer_cast<InformationData>(d);
        }
        else if(type == "float")
        {
            auto d = std::make_shared<FloatData>(result);
            info = std::static_pointer_cast<InformationData>(d);
        }
        else if(type == "double")
        {
            auto d = std::make_shared<DoubleData>(result);
            info = std::static_pointer_cast<InformationData>(d);
        }
        else if(type == "bool")
        {
            auto d = std::make_shared<BoolData>(result);
            info = std::static_pointer_cast<InformationData>(d);
        }
        return;
    }

};

