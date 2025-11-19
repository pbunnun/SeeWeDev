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
 * @file StdVectorNumberData.hpp
 * @brief Template class for transferring numeric vector data in dataflow graphs.
 *
 * This file defines the StdVectorNumberData template class for transmitting
 * std::vector<T> of numeric types through QtNodes connections. Supports integer,
 * float, and double vectors with automatic type aliasing.
 *
 * **Key Features:**
 * - **Generic Numeric Vectors:** Template-based for int, float, double
 * - **Type Aliases:** Convenient typedefs for common types
 * - **Information Display:** Human-readable vector content
 * - **QtNodes Integration:** Compatible with dataflow graph system
 *
 * **Common Use Cases:**
 * - Feature vectors (machine learning inputs)
 * - Histogram data (image analysis)
 * - Time series data (sensor readings)
 * - Coordinate lists (polygon points)
 * - Statistical data (distributions, samples)
 *
 * **Supported Vector Types:**
 * - **StdVectorIntData:** std::vector<int> for integer arrays
 * - **StdVectorFloatData:** std::vector<float> for single-precision
 * - **StdVectorDoubleData:** std::vector<double> for double-precision
 *
 * **Usage Pattern:**
 * @code
 * // Producer node
 * auto data = std::make_shared<StdVectorIntData>();
 * data->data() = {1, 2, 3, 4, 5};  // Set vector contents
 * data->set_information();          // Update display text
 * emit dataUpdated(0);              // Send to output port
 * 
 * // Consumer node
 * auto input = std::dynamic_pointer_cast<StdVectorIntData>(nodeData);
 * if (input) {
 *     std::vector<int>& values = input->data();
 *     int sum = std::accumulate(values.begin(), values.end(), 0);
 * }
 * @endcode
 *
 * **Data Transfer Examples:**
 * @code
 * // Feature extraction: Image → Feature vector
 * ImageProcessor → [features: std::vector<float>] → Classifier
 * 
 * // Histogram: Image → Intensity distribution
 * ImageLoader → [histogram: std::vector<int>] → HistogramDisplay
 * 
 * // Time series: Sensor → Sample buffer
 * SensorReader → [samples: std::vector<double>] → SignalAnalyzer
 * @endcode
 *
 * **Information Display:**
 * The set_information() method formats vector contents for property browser:
 * @code
 * Data Type : std::vector
 * 10.5
 * 20.3
 * 15.7
 * ...
 * @endcode
 *
 * @see InformationData for base class with display functionality
 * @see NodeData for QtNodes data interface
 */

#pragma once

#include <QtNodes/NodeData>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class StdVectorNumberData
 * @brief Template class for transmitting numeric vector data through dataflow connections.
 *
 * Provides a type-safe container for std::vector<T> of numeric types (int, float, double)
 * with QtNodes integration and information display capabilities.
 *
 * **Template Parameter:**
 * @tparam T Numeric type (int, float, double) for vector elements
 *
 * **Core Functionality:**
 * - **Vector Storage:** Holds std::vector<T> data
 * - **Type Identification:** Returns "Numbers"/"Nbs" type descriptor
 * - **Data Access:** Direct reference to internal vector
 * - **Information Display:** Formatted vector content for UI
 *
 * **Inheritance:**
 * @code
 * NodeData
 *   └── InformationData
 *         └── StdVectorNumberData<T>
 * @endcode
 *
 * **Typical Usage:**
 * @code
 * // Create and populate vector data
 * auto vecData = std::make_shared<StdVectorFloatData>();
 * vecData->data() = {1.5f, 2.7f, 3.2f, 4.1f};
 * vecData->set_information();
 * 
 * // Send through connection
 * setOutData(portIndex, vecData);
 * 
 * // Receive and process
 * auto input = std::dynamic_pointer_cast<StdVectorDoubleData>(inData);
 * if (input) {
 *     for (double val : input->data()) {
 *         processValue(val);
 *     }
 * }
 * @endcode
 *
 * **Use Case Examples:**
 *
 * **1. Feature Vectors (Machine Learning):**
 * @code
 * // Extract features from image
 * auto features = std::make_shared<StdVectorFloatData>();
 * features->data() = extractHOGFeatures(image);  // [0.2, 0.5, 0.1, ...]
 * features->set_information();
 * 
 * // Feed to classifier
 * ClassifierNode receives features->data()
 * @endcode
 *
 * **2. Histogram Data:**
 * @code
 * // Calculate intensity histogram
 * auto histogram = std::make_shared<StdVectorIntData>();
 * histogram->data().resize(256, 0);  // 256 bins for 8-bit image
 * 
 * for (int pixel : imagePixels) {
 *     histogram->data()[pixel]++;
 * }
 * histogram->set_information();
 * @endcode
 *
 * **3. Coordinate Lists:**
 * @code
 * // Store polygon vertices
 * auto xCoords = std::make_shared<StdVectorDoubleData>();
 * auto yCoords = std::make_shared<StdVectorDoubleData>();
 * 
 * xCoords->data() = {10.5, 20.3, 15.7, 8.2};
 * yCoords->data() = {5.1, 18.9, 25.3, 12.0};
 * @endcode
 *
 * **4. Time Series Data:**
 * @code
 * // Buffer sensor readings
 * auto samples = std::make_shared<StdVectorDoubleData>();
 * samples->data().reserve(1000);  // Pre-allocate for efficiency
 * 
 * while (sensorActive) {
 *     samples->data().push_back(readSensor());
 * }
 * samples->set_information();
 * @endcode
 *
 * **Type Aliases:**
 * - StdVectorIntData: For integer vectors (counts, indices, labels)
 * - StdVectorFloatData: For single-precision (graphics, ML features)
 * - StdVectorDoubleData: For double-precision (scientific, high-accuracy)
 *
 * **Vector Modification:**
 * @code
 * auto vecData = std::make_shared<StdVectorIntData>();
 * 
 * // Direct manipulation
 * vecData->data().push_back(42);
 * vecData->data().resize(100);
 * vecData->data().clear();
 * 
 * // STL algorithms
 * std::sort(vecData->data().begin(), vecData->data().end());
 * auto maxIt = std::max_element(vecData->data().begin(), vecData->data().end());
 * @endcode
 *
 * **Information Display Format:**
 * When set_information() is called, the property browser shows:
 * @code
 * Data Type : std::vector
 * 1.5
 * 2.7
 * 3.2
 * 4.1
 * @endcode
 *
 * @note Vector size is not automatically limited - consider truncating display for large vectors
 * @note set_information() must be called to update display after data changes
 *
 * @see InformationData::set_information() for display update mechanism
 * @see InformationData::information() for retrieving formatted string
 */
template <class T>
class StdVectorNumberData : public InformationData
{
public:
    /**
     * @brief Default constructor - creates empty vector.
     *
     * Initializes the data object with an empty std::vector<T>.
     *
     * **Example:**
     * @code
     * auto data = std::make_shared<StdVectorIntData>();
     * // data->data() is empty vector
     * data->data().push_back(10);
     * data->data().push_back(20);
     * @endcode
     */
    StdVectorNumberData()
        : mvData()
    {}

    /**
     * @brief Constructs with initial vector data.
     *
     * Initializes the data object with a copy of the provided vector.
     *
     * @param data Initial std::vector<T> to copy into this data object
     *
     * **Example:**
     * @code
     * std::vector<float> features = {0.5f, 1.2f, 0.8f};
     * auto data = std::make_shared<StdVectorFloatData>(features);
     * // data->data() contains copy of features
     * @endcode
     */
    StdVectorNumberData(const std::vector<T> & data )
        : mvData( data )
    {}

    /**
     * @brief Returns the data type identifier for QtNodes.
     *
     * Identifies this data as "Numbers" type with "Nbs" abbreviation,
     * enabling type-compatible connections in the dataflow graph.
     *
     * @return NodeDataType with id "Numbers" and name "Nbs"
     *
     * **Type Compatibility:**
     * @code
     * // All StdVectorNumberData variants share same type
     * StdVectorIntData::type()    -> {"Numbers", "Nbs"}
     * StdVectorFloatData::type()  -> {"Numbers", "Nbs"}
     * StdVectorDoubleData::type() -> {"Numbers", "Nbs"}
     * 
     * // Can connect any numeric vector to any numeric vector input
     * // Runtime casting determines actual template type
     * @endcode
     *
     * @note All instantiations (int, float, double) share the same type identifier
     * @note Use dynamic_pointer_cast to verify template parameter at runtime
     *
     * @see QtNodes::NodeDataType for type system details
     */
    NodeDataType
    type() const override
    {
        return { "Numbers", "Nbs" };
    }

    /**
     * @brief Returns a mutable reference to the internal vector.
     *
     * Provides direct access to the std::vector<T> for reading or modification.
     *
     * @return std::vector<T>& Mutable reference to internal vector
     *
     * **Example Usage:**
     * @code
     * auto data = std::make_shared<StdVectorIntData>();
     * 
     * // Add elements
     * data->data().push_back(10);
     * data->data().insert(data->data().end(), {20, 30, 40});
     * 
     * // Modify elements
     * data->data()[0] = 15;
     * 
     * // Use STL algorithms
     * std::sort(data->data().begin(), data->data().end());
     * std::reverse(data->data().begin(), data->data().end());
     * 
     * // Query
     * size_t count = data->data().size();
     * bool empty = data->data().empty();
     * 
     * // Update display after modifications
     * data->set_information();
     * @endcode
     *
     * **Common Operations:**
     * @code
     * // Resize and initialize
     * data->data().resize(100, 0);  // 100 elements, all zeros
     * 
     * // Reserve capacity
     * data->data().reserve(1000);  // Avoid reallocations
     * 
     * // Clear
     * data->data().clear();
     * 
     * // Range-based iteration
     * for (T& val : data->data()) {
     *     val *= 2;  // Double all values
     * }
     * @endcode
     *
     * @warning Modifications are not automatically reflected in UI - call set_information()
     * @note Returns reference - changes directly affect stored data
     */
    std::vector<T> &
    data()
    {
        return mvData;
    }

    /**
     * @brief Updates the information string with current vector contents.
     *
     * Formats the vector data into a human-readable QString for display
     * in the property browser. Shows data type header and all elements.
     *
     * **Display Format:**
     * @code
     * Data Type : std::vector
     * 10.5
     * 20.3
     * 15.7
     * 8.9
     * @endcode
     *
     * **Example:**
     * @code
     * auto data = std::make_shared<StdVectorDoubleData>();
     * data->data() = {1.1, 2.2, 3.3, 4.4, 5.5};
     * data->set_information();  // Updates display string
     * 
     * // Retrieve formatted string
     * QString info = data->information();
     * qDebug() << info;
     * // Output:
     * // Data Type : std::vector
     * // 1.1
     * // 2.2
     * // 3.3
     * // 4.4
     * // 5.5
     * @endcode
     *
     * **Large Vector Consideration:**
     * @code
     * // For very large vectors, consider truncating display
     * // Current implementation shows ALL elements
     * auto largeData = std::make_shared<StdVectorIntData>();
     * largeData->data().resize(10000);  // 10000 elements
     * largeData->set_information();      // May create very long string
     * @endcode
     *
     * @note Call this after modifying data() to update property browser display
     * @note QString::number() handles type-specific formatting (int vs float precision)
     * @note All elements are included - no automatic truncation for large vectors
     *
     * @see InformationData::information() to retrieve the formatted string
     * @see InformationData::mQSData for the internal QString storage
     */
    void set_information() override
    {
        mQSData  = QString("Data Type : std::vector \n");
        for (T value : mvData)
        {
            mQSData += QString::number(value);
            mQSData += "\n";
        }
    }

private:
    /**
     * @brief Internal storage for the numeric vector.
     *
     * Stores the actual std::vector<T> data transmitted through connections.
     */
    std::vector<T> mvData;
};

/**
 * @def StdVectorIntData
 * @brief Type alias for std::vector<int> data transfer.
 *
 * Convenient typedef for integer vector data, commonly used for:
 * - Histograms (pixel intensity distributions)
 * - Label arrays (classification outputs)
 * - Index lists (selected items, sorted indices)
 * - Count data (object detections per frame)
 *
 * **Example:**
 * @code
 * auto histogram = std::make_shared<StdVectorIntData>();
 * histogram->data().resize(256, 0);  // 256 bins
 * 
 * for (uchar pixel : imageData) {
 *     histogram->data()[pixel]++;
 * }
 * @endcode
 */
#define StdVectorIntData StdVectorNumberData<int>

/**
 * @def StdVectorFloatData
 * @brief Type alias for std::vector<float> data transfer.
 *
 * Convenient typedef for single-precision float vectors, commonly used for:
 * - Machine learning features (HOG, SIFT descriptors)
 * - Graphics coordinates (normalized positions)
 * - Probability distributions (classifier outputs)
 * - Lightweight numeric arrays
 *
 * **Example:**
 * @code
 * auto features = std::make_shared<StdVectorFloatData>();
 * features->data() = extractFeatures(image);  // ML feature vector
 * 
 * // Feed to neural network
 * auto output = neuralNet.predict(features->data());
 * @endcode
 */
#define StdVectorFloatData StdVectorNumberData<float>

/**
 * @def StdVectorDoubleData
 * @brief Type alias for std::vector<double> data transfer.
 *
 * Convenient typedef for double-precision float vectors, commonly used for:
 * - Scientific computations (high accuracy required)
 * - Statistical analysis (mean, variance, correlation)
 * - Coordinate transformations (matrix operations)
 * - Sensor measurements (calibrated values)
 *
 * **Example:**
 * @code
 * auto samples = std::make_shared<StdVectorDoubleData>();
 * samples->data().reserve(1000);
 * 
 * // Collect high-precision sensor readings
 * for (int i = 0; i < 1000; i++) {
 *     samples->data().push_back(readHighPrecisionSensor());
 * }
 * 
 * // Statistical analysis
 * double mean = std::accumulate(samples->data().begin(), 
 *                                samples->data().end(), 0.0) / samples->data().size();
 * @endcode
 */
#define StdVectorDoubleData StdVectorNumberData<double>
