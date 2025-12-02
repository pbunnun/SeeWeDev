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
 * @file CVGaussianBlurModel.hpp
 * @brief Node model for Gaussian blur filtering
 * 
 * This file defines a node that applies Gaussian blur to images for noise reduction
 * and smoothing. Gaussian blur is one of the most important preprocessing operations
 * in computer vision, using a Gaussian kernel to create a weighted average of pixels.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

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
 * @struct CVGaussianBlurParameters
 * @brief Parameter structure for Gaussian blur operation
 * 
 * Configures the Gaussian blur filter with kernel size and standard deviation.
 * 
 * The Gaussian kernel is defined by:
 * - Kernel size (width × height): Must be odd numbers (3, 5, 7, etc.)
 * - Sigma X/Y: Standard deviation in X and Y directions
 * 
 * Sigma selection guidelines:
 * - If sigma = 0, it's automatically calculated from kernel size
 * - Larger sigma = more blur (wider Gaussian distribution)
 * - Different sigmaX and sigmaY create directional blur
 * - Typical values: sigma = 0.3*((kernelSize-1)*0.5 - 1) + 0.8
 */
typedef struct CVGaussianBlurParameters{
    /** 
     * @brief Size of the Gaussian kernel (must be odd × odd)
     * @note Common values: (3,3), (5,5), (7,7), (9,9)
     */
    cv::Size mCVSizeKernel;
    
    /** 
     * @brief Gaussian kernel standard deviation in X direction
     * @note If 0, calculated automatically from kernel width
     */
    double mdSigmaX;
    
    /** 
     * @brief Gaussian kernel standard deviation in Y direction
     * @note If 0, set equal to sigmaX (isotropic blur)
     */
    double mdSigmaY;
    
    /** 
     * @brief Border extrapolation method for edge pixels
     * @see cv::BorderTypes
     */
    int miBorderType;
    
    /**
     * @brief Default constructor with standard 5×5 kernel
     */
    CVGaussianBlurParameters()
    {
        mCVSizeKernel = cv::Size(5,5);
        mdSigmaX = 0;  // Auto-calculate
        mdSigmaY = 0;  // Auto-calculate
        miBorderType = cv::BORDER_DEFAULT;
    }
} CVGaussianBlurParameters;

class CVGaussianBlurWorker : public QObject
{
    Q_OBJECT
public:
    explicit CVGaussianBlurWorker(QObject *parent = nullptr) : QObject(parent) {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     CVGaussianBlurParameters params,
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId);

Q_SIGNALS:
    // CRITICAL: This signal MUST be declared in each worker class
    // CANNOT be inherited from base class due to Qt MOC limitation
    void frameReady(std::shared_ptr<CVImageData> img);
};

/**
 * @class CVGaussianBlurModel
 * @brief Node model for Gaussian blur image smoothing
 * 
 * This model applies Gaussian blur using OpenCV's cv::CVGaussianBlur().
 * Gaussian blur is a crucial operation in computer vision for:
 * - **Noise reduction**: Suppresses high-frequency noise while preserving edges better than box filters
 * - **Preprocessing**: Prepares images for edge detection, feature extraction, segmentation
 * - **Scale-space analysis**: Creates image pyramids for multi-scale processing
 * - **Anti-aliasing**: Reduces aliasing artifacts before downsampling
 * - **Depth of field simulation**: Creates bokeh-like blur effects
 * 
 * How Gaussian blur works:
 * 1. Creates a 2D Gaussian kernel based on kernel size and sigma
 * 2. Convolves the kernel with the image (weighted average)
 * 3. Each output pixel is a weighted sum of surrounding pixels
 * 4. Weights follow Gaussian distribution (closer pixels have higher weights)
 * 
 * Advantages over other blur methods:
 * - Isotropic (rotationally symmetric) - no directional bias
 * - Better edge preservation than box/average blur
 * - Mathematically well-defined (separable, associative)
 * - Models natural optical blur (lens defocus)
 * 
 * Common use cases:
 * - Preprocessing for Canny edge detection
 * - Noise reduction in low-quality images
 * - Creating image pyramids (Gaussian pyramid)
 * - Background blurring for privacy/aesthetics
 * - Preparing images for feature detection (SIFT, SURF)
 * 
 * Input:
 * - Port 0: CVImageData - Source image to blur
 * 
 * Output:
 * - Port 0: CVImageData - Blurred image
 * 
 * @note Larger kernels = more blur but slower processing
 * @note For edge-preserving smoothing, consider bilateral filter instead
 * @see cv::CVGaussianBlur for the underlying OpenCV operation
 */
class CVGaussianBlurModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new Gaussian blur node
     * 
     * Initializes with default 5×5 kernel and auto-calculated sigma.
     */
    CVGaussianBlurModel();

    /**
     * @brief Destructor
     */
    ~CVGaussianBlurModel() override = default;

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the current blur parameters for project persistence.
     * 
     * @return QJsonObject containing kernel size, sigma values, and border type
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved blur parameters.
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
     * - "kernel_width": Width of Gaussian kernel (odd int, >= 3)
     * - "kernel_height": Height of Gaussian kernel (odd int, >= 3)
     * - "sigma_x": Standard deviation in X direction (double, >= 0)
     * - "sigma_y": Standard deviation in Y direction (double, >= 0)
     * - "border_type": Edge pixel handling method (enumeration)
     * 
     * When properties change, the node automatically reprocesses current input.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property
     * @note Kernel dimensions must be odd numbers
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
    // Implement PBAsyncDataModel pure virtuals
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;

private:
    void process_cached_input() override;

    /** @brief Current blur parameters */
    CVGaussianBlurParameters mParams;
    
    /** @brief Preview pixmap for node palette */
    QPixmap _minPixmap;

    // Pending data for backpressure
    cv::Mat mPendingFrame;
    CVGaussianBlurParameters mPendingParams;
};

