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
#include <atomic>
#include <QtCore/QMutex>
#include <QtCore/QThread>

#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"
#include "CVImagePool.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;

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

class CVFilter2DWorker : public QObject
{
    Q_OBJECT
public:
    explicit CVFilter2DWorker(QObject *parent = nullptr) : QObject(parent) {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     CVFilter2DParameters params,
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);
};

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
class CVFilter2DModel : public PBAsyncDataModel
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
    ~CVFilter2DModel() override = default;

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

protected:
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;
    void process_cached_input() override;

private:

    /** @brief Current filter parameters */
    CVFilter2DParameters mParams;
    
    /** @brief Preview pixmap for node palette */
    QPixmap _minPixmap;

    // Pending work for backpressure handling
    cv::Mat mPendingFrame;
    CVFilter2DParameters mPendingParams;
};


