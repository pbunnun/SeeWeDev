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
 * @file StdStringData.hpp
 * @brief Standard string data type for text-based dataflow communication.
 *
 * This file defines the StdStringData class, which encapsulates std::string
 * values for transmission between nodes in the CVDev dataflow graph system.
 *
 * **Key Features:**
 * - **Text Storage:** Stores UTF-8 compatible text strings
 * - **Standard C++ Type:** Uses std::string internally
 * - **Qt Integration:** Automatic conversion to/from QString
 * - **Immutable Access:** Read-only data() method
 *
 * **Common Use Cases:**
 * - File paths and filenames
 * - Configuration parameters
 * - Text labels and annotations
 * - Log messages and status text
 * - Command-line arguments
 * - Data identifiers and keys
 * - Recognized text from OCR
 *
 * **Dataflow Patterns:**
 * @code
 * // File path transmission
 * FileDialogNode → [StdStringData] → ImageLoaderNode
 * 
 * // Text annotation
 * TextInputNode → [StdStringData] → TextOverlayNode → [Image]
 * 
 * // OCR result
 * TextRecognitionNode → [StdStringData] → DisplayNode
 * @endcode
 *
 * **String Encoding:**
 * - std::string stores bytes (typically UTF-8)
 * - QString handles Unicode automatically
 * - Conversion via QString::fromStdString()
 *
 * @see InformationData
 * @see QString
 */

#pragma once

#include <QtNodes/NodeData>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class StdStringData
 * @brief Standard string data container for dataflow graph nodes.
 *
 * Encapsulates a std::string value with type identification and Qt integration
 * for use in the CVDev node-based visual programming system.
 *
 * **Data Properties:**
 * - **Type Name:** "information"
 * - **Display Name:** "Str"
 * - **Storage:** std::string
 * - **Access:** Read-only via data() method
 *
 * **Construction Examples:**
 * @code
 * // Default constructor (empty string)
 * auto data1 = std::make_shared<StdStringData>();
 * 
 * // Initialize with string
 * auto data2 = std::make_shared<StdStringData>("Hello World");
 * 
 * // From std::string variable
 * std::string path = "/path/to/file.txt";
 * auto data3 = std::make_shared<StdStringData>(path);
 * @endcode
 *
 * **Access Patterns:**
 * @code
 * // Read value
 * std::string text = data->data();
 * 
 * // Use in file operations
 * std::ifstream file(pathData->data());
 * 
 * // String operations
 * if (data->data().empty()) {
 *     // Handle empty string
 * }
 * 
 * size_t length = data->data().length();
 * @endcode
 *
 * **Qt Integration:**
 * @code
 * // Convert to QString for Qt operations
 * QString qstr = QString::fromStdString(data->data());
 * 
 * // Display in Qt widgets
 * label->setText(QString::fromStdString(data->data()));
 * 
 * // File paths in Qt
 * QFile file(QString::fromStdString(pathData->data()));
 * @endcode
 *
 * **Information Display:**
 * The set_information() method generates:
 * @code
 * Data Type : std::string
 * <string content>
 * @endcode
 *
 * Example:
 * @code
 * Data Type : std::string
 * /path/to/image.png
 * @endcode
 *
 * **File Path Usage:**
 * @code
 * // Common pattern: file path transmission
 * auto pathData = std::make_shared<StdStringData>("/home/user/image.jpg");
 * 
 * // In receiving node
 * std::string path = inputData->data();
 * cv::Mat image = cv::imread(path);
 * @endcode
 *
 * **Text Processing:**
 * @code
 * // String manipulation
 * std::string text = data->data();
 * std::transform(text.begin(), text.end(), text.begin(), ::toupper);
 * 
 * // Parsing
 * std::istringstream iss(data->data());
 * std::string token;
 * while (iss >> token) {
 *     // Process tokens
 * }
 * @endcode
 *
 * **Best Practices:**
 * - Use for text data, file paths, identifiers
 * - Consider QString directly if only using Qt APIs
 * - Check for empty strings before processing
 * - Be aware of encoding (typically UTF-8)
 *
 * @note data() method returns by value (copy), not reference.
 * @note No direct modification method - create new instance for changes.
 * @note Type name is "information" (generic) not "string" (specific).
 *
 * @see InformationData for base class functionality
 * @see QString for Qt string operations
 */
class StdStringData : public InformationData
{
public:
    /**
     * @brief Default constructor creating empty string.
     *
     * Creates a StdStringData instance with an empty string.
     */
    StdStringData()
        : msData()
    {
    }

    /**
     * @brief Constructor with initial string value.
     * @param string Initial string content.
     *
     * Creates a StdStringData instance with the specified string content.
     */
    StdStringData( std::string const & string )
        : msData( string )
    {}

    /**
     * @brief Returns the data type information.
     * @return NodeDataType with name "information" and display "Str".
     *
     * Provides type identification for the node system's type checking
     * and connection validation.
     *
     * @note Type name is "information" (generic category), not "string".
     */
    NodeDataType
    type() const override
    {
        return { "information", "Str" };
    }

    /**
     * @brief Returns a copy of the string value.
     * @return std::string containing the data.
     *
     * Returns the stored string by value (copy). Unlike other data types
     * that return references, this returns a copy.
     *
     * **Usage Examples:**
     * @code
     * std::string path = data->data();
     * std::ifstream file(data->data());
     * 
     * // Check if empty
     * if (data->data().empty()) {
     *     // Handle empty string
     * }
     * @endcode
     *
     * @note Returns by value, not reference. Each call creates a copy.
     */
    std::string
    data() const
    {
        return msData;
    }

    /**
     * @brief Generates formatted information string.
     *
     * Creates a human-readable string representation of the data
     * for display in debug views or information panels.
     *
     * **Format:**
     * @code
     * Data Type : std::string
     * <string content>
     * @endcode
     *
     * Example output:
     * @code
     * Data Type : std::string
     * /path/to/file.txt
     * @endcode
     *
     * The string content is converted from std::string to QString
     * using QString::fromStdString() for proper Unicode handling.
     */
    void set_information() override
    {
        mQSData = QString("Data Type : std::string \n");
        mQSData += QString::fromStdString(msData);
        mQSData += QString("\n");
    }

private:
    /**
     * @brief The stored string value.
     *
     * Internal storage for the text data. Access through data() method.
     */
    std::string msData;
};
