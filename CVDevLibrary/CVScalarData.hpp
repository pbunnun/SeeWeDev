/**
 * @file CVScalarData.hpp
 * @brief OpenCV Scalar data type for multi-channel color/value representation.
 *
 * This file defines the CVScalarData class, which encapsulates cv::Scalar
 * values for transmission between nodes in the CVDev dataflow graph system.
 *
 * **Key Features:**
 * - **Multi-Channel:** Stores up to 4 values (cv::Scalar[0-3])
 * - **OpenCV Integration:** Direct cv::Scalar compatibility
 * - **Color Representation:** Perfect for BGR, BGRA, HSV colors
 * - **General Purpose:** Can represent any 1-4 component vector
 *
 * **Common Use Cases:**
 * - Color values (BGR, BGRA, RGB, RGBA, HSV, etc.)
 * - Fill colors for drawing operations
 * - Threshold ranges (min/max pairs)
 * - 3D/4D vectors in image processing
 * - Mean color calculations
 * - Statistical values (mean, stddev, etc.)
 *
 * **Dataflow Patterns:**
 * @code
 * // Color selection
 * ColorPickerNode → [CVScalarData] → DrawRectangleNode → [Image]
 * 
 * // Threshold range
 * RangeSliderNode → [CVScalarData(min,max)] → InRangeNode → [Mask]
 * 
 * // Mean color extraction
 * ImageAnalysisNode → [CVScalarData(mean)] → DisplayNode
 * @endcode
 *
 * **OpenCV Scalar Structure:**
 * - cv::Scalar[0]: First component (Blue in BGR, Hue in HSV)
 * - cv::Scalar[1]: Second component (Green in BGR, Saturation in HSV)
 * - cv::Scalar[2]: Third component (Red in BGR, Value in HSV)
 * - cv::Scalar[3]: Fourth component (Alpha in BGRA)
 *
 * **Example Values:**
 * @code
 * // BGR Colors
 * cv::Scalar black(0, 0, 0);
 * cv::Scalar white(255, 255, 255);
 * cv::Scalar red(0, 0, 255);
 * cv::Scalar green(0, 255, 0);
 * cv::Scalar blue(255, 0, 0);
 * 
 * // HSV Range
 * cv::Scalar hsvLower(0, 50, 50);
 * cv::Scalar hsvUpper(10, 255, 255);
 * @endcode
 *
 * @see InformationData
 * @see cv::Scalar
 * @see CVImageData
 */

#pragma once

#include <opencv2/core.hpp>

#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class CVScalarData
 * @brief OpenCV Scalar data container for dataflow graph nodes.
 *
 * Encapsulates a cv::Scalar value with type identification for use in
 * the CVDev node-based visual programming system. Scalar represents
 * a 4-element vector commonly used for colors and multi-channel values.
 *
 * **Data Properties:**
 * - **Type Name:** "information"
 * - **Display Name:** "Scl"
 * - **Storage:** cv::Scalar (4 doubles)
 * - **Components:** [0], [1], [2], [3]
 *
 * **Construction Examples:**
 * @code
 * // Default constructor (0, 0, 0, 0)
 * auto data1 = std::make_shared<CVScalarData>();
 * 
 * // Initialize with cv::Scalar
 * auto data2 = std::make_shared<CVScalarData>(cv::Scalar(255, 0, 0)); // Blue
 * 
 * // From individual values
 * cv::Scalar red(0, 0, 255);
 * auto data3 = std::make_shared<CVScalarData>(red);
 * @endcode
 *
 * **Access Patterns:**
 * @code
 * // Get reference to cv::Scalar
 * cv::Scalar& color = data->scalar();
 * 
 * // Access individual components
 * double blue = data->scalar()[0];
 * double green = data->scalar()[1];
 * double red = data->scalar()[2];
 * double alpha = data->scalar()[3];
 * 
 * // Modify components
 * data->scalar()[0] = 128;  // Set blue channel
 * @endcode
 *
 * **OpenCV Drawing Usage:**
 * @code
 * // Rectangle with color
 * cv::Mat image;
 * cv::Scalar color = colorData->scalar();
 * cv::rectangle(image, rect, color, thickness);
 * 
 * // Circle
 * cv::circle(image, center, radius, data->scalar(), -1);
 * 
 * // Text
 * cv::putText(image, text, point, font, size, data->scalar());
 * @endcode
 *
 * **Color Space Operations:**
 * @code
 * // BGR color
 * auto bgrData = std::make_shared<CVScalarData>(cv::Scalar(255, 0, 0));
 * 
 * // HSV range for color filtering
 * cv::Scalar lowerBound(0, 50, 50);
 * cv::Scalar upperBound(10, 255, 255);
 * cv::inRange(hsvImage, lowerBound, upperBound, mask);
 * @endcode
 *
 * **Information Display:**
 * The set_information() method generates:
 * @code
 * (255.0 , 0.0 , 0.0 , 0.0)
 * @endcode
 *
 * This shows all four components in parentheses, separated by commas.
 *
 * **Common Color Values:**
 * @code
 * // BGR Colors (B, G, R)
 * cv::Scalar black(0, 0, 0);
 * cv::Scalar white(255, 255, 255);
 * cv::Scalar red(0, 0, 255);
 * cv::Scalar green(0, 255, 0);
 * cv::Scalar blue(255, 0, 0);
 * cv::Scalar yellow(0, 255, 255);
 * cv::Scalar magenta(255, 0, 255);
 * cv::Scalar cyan(255, 255, 0);
 * @endcode
 *
 * **Statistical Usage:**
 * @code
 * // Mean and standard deviation
 * cv::Scalar mean, stddev;
 * cv::meanStdDev(image, mean, stddev);
 * 
 * auto meanData = std::make_shared<CVScalarData>(mean);
 * auto stddevData = std::make_shared<CVScalarData>(stddev);
 * @endcode
 *
 * **Best Practices:**
 * - Remember BGR ordering in OpenCV (not RGB!)
 * - Use all 4 components for BGRA images
 * - Scalar values are doubles internally
 * - Components default to 0 if not specified
 *
 * @note cv::Scalar stores doubles, not ints (auto-converts).
 * @note Type name is "information" (generic category).
 * @note Direct reference access via scalar() method.
 *
 * @see InformationData for base class functionality
 * @see cv::Scalar for OpenCV documentation
 * @see CVImageData for image data type
 */
class CVScalarData : public InformationData
{
public:
    /**
     * @brief Default constructor creating zero scalar.
     *
     * Creates a CVScalarData instance with cv::Scalar(0, 0, 0, 0).
     * All four components are initialized to zero.
     */
    CVScalarData()
        : mCVScalar()
    {}

    /**
     * @brief Constructor with initial cv::Scalar value.
     * @param scalar Initial cv::Scalar value.
     *
     * Creates a CVScalarData instance with the specified cv::Scalar.
     *
     * **Example:**
     * @code
     * // Blue color in BGR
     * auto blueData = std::make_shared<CVScalarData>(cv::Scalar(255, 0, 0));
     * @endcode
     */
    CVScalarData( const cv::Scalar & scalar )
        : mCVScalar( scalar )
    {}

    /**
     * @brief Returns the data type information.
     * @return NodeDataType with name "information" and display "Scl".
     *
     * Provides type identification for the node system's type checking
     * and connection validation.
     *
     * @note Type name is "information" (generic category), not "scalar".
     */
    NodeDataType
    type() const override
    {
        return { "information", "Scl" };
    }

    /**
     * @brief Returns a reference to the cv::Scalar value.
     * @return Reference to the internal cv::Scalar.
     *
     * Provides direct access to the cv::Scalar for reading and modification.
     *
     * **Usage Examples:**
     * @code
     * // Read components
     * double blue = data->scalar()[0];
     * double green = data->scalar()[1];
     * double red = data->scalar()[2];
     * double alpha = data->scalar()[3];
     * 
     * // Modify components
     * data->scalar()[0] = 255;  // Set blue
     * 
     * // Use in OpenCV functions
     * cv::rectangle(img, rect, data->scalar(), 2);
     * cv::inRange(img, lowerData->scalar(), upperData->scalar(), mask);
     * @endcode
     */
    cv::Scalar &
    scalar()
    {
        return mCVScalar;
    }

    /**
     * @brief Generates formatted information string.
     *
     * Creates a human-readable string representation of the scalar
     * for display in debug views or information panels.
     *
     * **Format:**
     * @code
     * (<val0> , <val1> , <val2> , <val3>)
     * @endcode
     *
     * Example outputs:
     * @code
     * (255.0 , 0.0 , 0.0 , 0.0)     // Blue in BGR
     * (0.0 , 255.0 , 0.0 , 0.0)     // Green in BGR
     * (0.0 , 0.0 , 255.0 , 0.0)     // Red in BGR
     * (128.5 , 64.3 , 200.1 , 255.0) // General values
     * @endcode
     */
    void set_information() override
    {
        mQSData = QString("(%1 , %2 , %3 , %4)")
                  .arg(mCVScalar[0]).arg(mCVScalar[1])
                  .arg(mCVScalar[2]).arg(mCVScalar[3]);
    }

private:
    /**
     * @brief The stored cv::Scalar value.
     *
     * Internal storage for the 4-component vector. Access through
     * scalar() method.
     *
     * Components:
     * - [0]: First component (Blue in BGR, Hue in HSV)
     * - [1]: Second component (Green in BGR, Saturation in HSV)
     * - [2]: Third component (Red in BGR, Value in HSV)
     * - [3]: Fourth component (Alpha in BGRA)
     */
    cv::Scalar mCVScalar;
};
