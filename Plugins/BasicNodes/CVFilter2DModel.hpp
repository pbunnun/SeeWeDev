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
 * @file CVFilter2DModel.hpp
 * @brief Node model for custom 2D convolution filtering
 * 
 * This file defines a node that applies custom convolution kernels to images.
 * CVFilter2D is a fundamental image processing operation that enables custom
 * linear filtering using user-defined or predefined kernels for various effects.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct MatKernel
 * @brief Kernel (convolution matrix) definition for filtering operations
 * 
 * Encapsulates predefined and custom convolution kernels. Convolution kernels
 * are small matrices that define how each pixel's value is calculated from
 * its neighbors.
 * 
 * Predefined kernel types:
 * - **KERNEL_NULL**: Identity or custom kernel
 * - **KERNEL_LAPLACIAN**: Edge detection (second derivative)
 * - **KERNEL_AVERAGE**: Box blur (averaging neighbors)
 */
typedef struct MatKernel
{
    /**
     * @enum KernelType
     * @brief Predefined kernel types for common operations
     */
    enum KernelType {
        KERNEL_NULL = 0,        ///< No predefined kernel (custom or identity)
        KERNEL_LAPLACIAN = 1,   ///< Laplacian edge detection kernel
        KERNEL_AVERAGE = 2      ///< Averaging (box blur) kernel
    };

    /** @brief Type of kernel to use */
    int miKernelType;
    
    /** @brief Size of the kernel (e.g., 3 for 3×3 kernel) */
    int miKernelSize;

    /**
     * @brief Constructs a kernel definition
     * 
     * @param kernelType The predefined kernel type
     * @param size Kernel dimension (generates size×size matrix)
     */
    explicit MatKernel(const KernelType kernelType, const int size)
        : miKernelType(kernelType),
          miKernelSize(size)
    {
    }

    /**
     * @brief Generates the actual cv::Mat kernel matrix
     * 
     * Creates the convolution kernel based on type and size:
     * - Laplacian: Standard Laplacian operator for edge detection
     * - Average: Normalized averaging kernel (all 1s / kernel_area)
     * - Null: Identity or custom matrix
     * 
     * @return cv::Mat containing the kernel coefficients
     */
    const cv::Mat image() const;

} MatKernel;

/**
 * @struct CVFilter2DParameters
 * @brief Parameter structure for 2D filtering operations
 * 
 * Configures custom convolution filtering with kernel selection and output options.
 */
typedef struct CVFilter2DParameters{
    /** 
     * @brief Desired depth of output image (CV_8U, CV_16S, CV_32F, etc.)
     * @note CV_8U = 8-bit unsigned, CV_16S = 16-bit signed, CV_32F = 32-bit float
     */
    int miImageDepth;
    
    /** @brief Convolution kernel definition */
    MatKernel mMKKernel;
    
    /** 
     * @brief Value added to filtered results before storing
     * @note Useful for brightening or offsetting filtered values
     */
    double mdDelta;
    
    /** 
     * @brief Border extrapolation method for edge pixels
     * @see cv::BorderTypes
     */
    int miBorderType;
    
    /**
     * @brief Default constructor with 3×3 null kernel
     */
    CVFilter2DParameters()
        : miImageDepth(CV_8U),
          mMKKernel(MatKernel(MatKernel::KERNEL_NULL, 3)),
          mdDelta(0),
          miBorderType(cv::BORDER_DEFAULT)
    {
    }
} CVFilter2DParameters;

/**
 * @class CVFilter2DModel
 * @brief Node model for custom 2D convolution filtering
 * 
 * This model applies arbitrary linear filters using OpenCV's cv::filter2D().
 * Convolution filtering is a fundamental operation where each output pixel
 * is a weighted sum of input pixels in a neighborhood defined by the kernel.
 * 
 * How 2D convolution works:
 * 1. Place kernel over each pixel in the image
 * 2. Multiply overlapping values element-wise
 * 3. Sum all products
 * 4. Add delta value
 * 5. Store result in output image
 * 
 * Mathematical formulation:
 * ```
 * dst(x,y) = Σ kernel(i,j) * src(x+i-anchor_x, y+j-anchor_y) + delta
 * ```
 * 
 * Predefined kernels:
 * - **Laplacian**: Detects edges using second derivative
 *   ```
 *   [ 0 -1  0]
 *   [-1  4 -1]
 *   [ 0 -1  0]
 *   ```
 * - **Average**: Simple blur by averaging neighbors
 *   ```
 *   [1/9 1/9 1/9]
 *   [1/9 1/9 1/9]
 *   [1/9 1/9 1/9]
 *   ```
 * 
 * Common use cases:
 * - Custom edge detection filters (Sobel, Prewitt, Roberts)
 * - Sharpening filters
 * - Embossing effects
 * - Directional derivative estimation
 * - Custom blur kernels
 * - Feature extraction filters
 * 
 * Input:
 * - Port 0: CVImageData - Source image to filter
 * 
 * Output:
 * - Port 0: CVImageData - Filtered image
 * 
 * Design Note: The output depth can differ from input, allowing accumulation
 * in higher precision formats (e.g., CV_32F) to prevent overflow/underflow.
 * 
 * @note Larger kernels = slower processing but can capture wider patterns
 * @see cv::filter2D for the underlying OpenCV operation
 */
class CVFilter2DModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new 2D filter node
     * 
     * Initializes with default 3×3 null kernel and 8-bit output depth.
     */
    CVFilter2DModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVFilter2DModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the current filter parameters for project persistence.
     * 
     * @return QJsonObject containing kernel type, size, depth, delta, border type
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved filter parameters.
     * 
     * @param p QJsonObject containing the saved node configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 1 input port (source image)
     * - 1 output port (filtered image)
     * 
     * @param portType The type of port (In or Out)
     * @return Number of ports of the specified type
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port
     * 
     * All ports use CVImageData type.
     * 
     * @param portType The type of port (In or Out)
     * @param portIndex The index of the port
     * @return NodeDataType describing CVImageData
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Provides the filtered image output
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the filtered CVImageData
     * @note Returns nullptr if no input has been processed
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input image data
     * 
     * When image data arrives, this method:
     * 1. Validates the input data
     * 2. Generates kernel matrix from type and size
     * 3. Applies cv::filter2D() with current parameters
     * 4. Stores the result for output
     * 5. Notifies connected nodes
     * 
     * @param nodeData The input CVImageData
     * @param portIndex The input port index (must be 0)
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief No embedded widget for this node
     * 
     * @return nullptr - Parameters are set via property browser
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets model properties from the property browser
     * 
     * Handles property changes for:
     * - "kernel_type": Predefined kernel selection (enumeration)
     * - "kernel_size": Kernel dimension (odd int, >= 3)
     * - "output_depth": Output image depth (CV_8U, CV_16S, CV_32F, etc.)
     * - "delta": Value added to result (double)
     * - "border_type": Edge pixel handling method (enumeration)
     * 
     * When properties change, the node automatically reprocesses current input.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Provides a thumbnail preview pixmap
     * 
     * @return QPixmap for node list/palette preview
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    /** @brief Category name for node organization */
    static const QString _category;

    /** @brief Display name for the node type */
    static const QString _model_name;

private:
    /**
     * @brief Internal helper to perform 2D convolution
     * 
     * Executes the filtering operation:
     * 1. Generates kernel cv::Mat from MatKernel definition
     * 2. Calls cv::filter2D() to convolve kernel with image
     * 3. Applies delta offset if specified
     * 4. Converts to specified output depth
     * 
     * Why different output depths matter:
     * - CV_8U: Standard 8-bit (0-255), suitable for display
     * - CV_16S: 16-bit signed, prevents overflow in edge detection
     * - CV_32F: Floating point, preserves precision for further processing
     * 
     * @param in The input CVImageData to filter
     * @param out The output CVImageData to populate with filtered result
     * @param params The filtering parameters (kernel, depth, delta, border)
     * @note Uses cv::filter2D() internally
     * @see cv::filter2D
     */
    void processData( const std::shared_ptr< CVImageData> & in, std::shared_ptr< CVImageData > & out,
                      const CVFilter2DParameters & params );

    /** @brief Current filter parameters */
    CVFilter2DParameters mParams;
    
    /** @brief Cached filtered output image */
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    
    /** @brief Cached input image data */
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    
    /** @brief Preview pixmap for node palette */
    QPixmap _minPixmap;
};


