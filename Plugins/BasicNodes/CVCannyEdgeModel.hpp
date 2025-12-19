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
 * @file CVCannyEdgeModel.hpp
 * @brief Node model for Canny edge detection
 * 
 * This file defines a node that performs edge detection using the Canny algorithm.
 * The Canny edge detector is one of the most popular edge detection techniques,
 * known for its optimal edge detection capabilities with good localization and
 * minimal false responses.
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
 * @struct CVCannyEdgeParameters
 * @brief Parameter structure for Canny edge detection
 * 
 * Configures the Canny edge detection algorithm with the following components:
 * 
 * The Canny algorithm workflow:
 * 1. Gaussian blur with kernel size miSizeKernel (noise reduction)
 * 2. Gradient calculation (Sobel operators)
 * 3. Non-maximum suppression (thin edges)
 * 4. Double thresholding with miThresholdL (low) and miThresholdU (upper)
 * 5. Edge tracking by hysteresis
 * 
 * Threshold selection guidelines:
 * - Upper threshold (miThresholdU): Strong edges above this are always included
 * - Lower threshold (miThresholdL): Weak edges between thresholds are included only if connected to strong edges
 * - Typical ratio: upper = 2x to 3x lower
 */
typedef struct CVCannyEdgeParameters{
    /** 
     * @brief Aperture size for Sobel operator (3, 5, or 7)
     * @note Larger kernels detect smoother, larger-scale edges
     */
    int miSizeKernel {3};
    
    /** 
     * @brief Upper threshold for hysteresis
     * @note Gradients above this are considered strong edges
     */
    int miThresholdU {90};
    
    /** 
     * @brief Lower threshold for hysteresis
     * @note Gradients between lower and upper are weak edges (kept if connected to strong)
     */
    int miThresholdL {30};
    
    /** 
     * @brief Enable L2 gradient calculation (more accurate but slower)
     * - false: L1 gradient (|dx| + |dy|) - faster
     * - true: L2 gradient (sqrt(dx² + dy²)) - more accurate
     */
    bool mbEnableGradient {false};
} CVCannyEdgeParameters;

/**
 * @class CVCannyEdgeWorker
 * @brief Worker class for asynchronous Canny edge detection
 */
class CVCannyEdgeWorker : public QObject
{
    Q_OBJECT
public:
    CVCannyEdgeWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     int thresholdL,
                     int thresholdU,
                     int kernelSize,
                     bool enableGradient,
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
 * @class CVCannyEdgeModel
 * @brief Node model for Canny edge detection algorithm
 * 
 * This model implements the Canny edge detection algorithm using OpenCV's cv::Canny().
 * The Canny detector is widely regarded as one of the best edge detectors due to:
 * - Good detection: Finds true edges with minimal false positives
 * - Good localization: Detected edges are close to actual edges
 * - Single response: Each edge is marked once (thin edges)
 * 
 * Algorithm stages:
 * 1. **Noise reduction**: Gaussian blur to suppress noise
 * 2. **Gradient computation**: Sobel operators find edge strength and direction
 * 3. **Non-maximum suppression**: Thin thick edges to single-pixel width
 * 4. **Double thresholding**: Classify edges as strong, weak, or non-edges
 * 5. **Edge tracking**: Connect weak edges to strong edges (hysteresis)
 * 
 * Common use cases:
 * - Object boundary detection
 * - Feature extraction for vision algorithms
 * - Image segmentation preprocessing
 * - Shape analysis and recognition
 * - Lane detection in autonomous vehicles
 * 
 * Input:
 * - Port 0: CVImageData - Source image (will be converted to grayscale if needed)
 * - Port 1: SyncData - Optional synchronization signal
 * 
 * Output:
 * - Port 0: CVImageData - Binary edge map (white edges on black background)
 * 
 * Design Note: Input is automatically converted to grayscale since Canny operates
 * on intensity gradients. The output is a single-channel binary image where
 * edge pixels have value 255 and non-edge pixels have value 0.
 * 
 * @note For best results, adjust thresholds based on image characteristics
 * @see cv::Canny for the underlying OpenCV implementation
 */
class CVCannyEdgeModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new Canny edge detection node
     * 
     * Initializes with default parameters:
     * - Kernel size: 3
     * - Upper threshold: 90
     * - Lower threshold: 30
     * - L1 gradient (faster)
     */
    CVCannyEdgeModel();

    /**
     * @brief Destructor - ensures safe cleanup of async operations
     */
    ~CVCannyEdgeModel() override = default;

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the current Canny parameters for project persistence.
     * 
     * @return QJsonObject containing kernel size and threshold values
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved Canny parameters.
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
     * - "kernel_size": Sobel aperture size (3, 5, or 7)
     * - "threshold_upper": Upper hysteresis threshold (0-255)
     * - "threshold_lower": Lower hysteresis threshold (0-255)
     * - "enable_l2_gradient": Use L2 norm for gradient (bool)
     * 
     * When properties change, the node automatically reprocesses current input.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property
     * @note Ensure threshold_upper > threshold_lower for best results
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

    /** @brief Current Canny parameters */
    CVCannyEdgeParameters mParams;
    
    /** @brief Preview pixmap for node palette */
    QPixmap _minPixmap;

    // Pending data for backpressure
    cv::Mat mPendingFrame;
    CVCannyEdgeParameters mPendingParams;
    QPixmap _minpixmap;
};

