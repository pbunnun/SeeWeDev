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
 * @file CVHoughCircleTransfromModel.hpp
 * @brief Provides circle detection using the Hough Circle Transform algorithm.
 *
 * This file implements a node that detects circular shapes in grayscale or edge-detected images
 * using OpenCV's cv::CVHoughCircles function. The Hough Circle Transform is a feature extraction
 * technique used to identify circles in images, particularly useful for detecting round objects
 * like coins, eyes, wheels, or cells in microscopy images.
 *
 * The algorithm works by accumulating evidence for circles of various sizes and positions in
 * a parameter space (Hough space). It uses gradient information to vote for potential circle
 * centers and radii, making it robust to partial occlusions and noise.
 *
 * The node outputs both a visualization image with detected circles drawn and the count of
 * detected circles, enabling both visual inspection and automated counting applications.
 *
 * Key Features:
 * - Multiple Hough methods (currently HOUGH_GRADIENT)
 * - Configurable circle size range (min/max radius)
 * - Adjustable detection sensitivity (threshold parameters)
 * - Customizable visualization (center points and/or circles)
 * - Circle count output for automated analysis
 *
 * Typical Applications:
 * - Coin detection and counting
 * - Eye tracking and pupil detection
 * - Wheel or circular part inspection
 * - Cell counting in microscopy
 * - Round object quality control
 * - Traffic sign detection (circular signs)
 *
 * @see CVHoughCircleTransformModel, cv::CVHoughCircles
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"
#include "CVImagePool.hpp"
#include "SyncData.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;
using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;

/**
 * @struct CVHoughCircleTransformParameters
 * @brief Configuration parameters for the Hough Circle Transform algorithm.
 *
 * This structure contains all parameters controlling circle detection behavior and visualization.
 * The parameters are divided into three categories:
 * 1. Detection parameters: Control the algorithm's sensitivity and search space
 * 2. Visualization parameters: Control how detected circles are drawn
 * 3. Algorithm flags: Enable/disable specific detection modes
 *
 * Detection Parameters:
 * - miHoughMethod: Detection algorithm variant (currently only HOUGH_GRADIENT supported)
 *   * HOUGH_GRADIENT: Uses Sobel derivatives to compute edge gradients
 *   * More methods may be added in future OpenCV versions
 *
 * - mdInverseRatio: Inverse ratio of accumulator resolution to image resolution
 *   * Value of 1: Accumulator has same resolution as input image (most accurate)
 *   * Value of 2: Accumulator has half the resolution (2x faster, less accurate)
 *   * Typical range: 1.0 to 2.0
 *   * Higher values increase speed but reduce precision
 *
 * - mdCenterDistance: Minimum distance between detected circle centers (in pixels)
 *   * Prevents detecting the same circle multiple times
 *   * Too small: Multiple detections of same circle
 *   * Too large: Misses circles that are close together
 *   * Typical value: Circle diameter or larger
 *
 * - mdThresholdU (Upper Threshold): First pass threshold for center detection
 *   * Higher values: Fewer circles detected (more conservative)
 *   * Lower values: More circles detected (may include false positives)
 *   * Typical range: 100-300 (Canny edge detector upper threshold)
 *
 * - mdThresholdL (Lower Threshold): Second pass threshold for circle accumulation
 *   * Controls sensitivity in accumulator voting
 *   * Lower values: More circles detected
 *   * Typical range: 50-200 (should be less than upper threshold)
 *   * Rule of thumb: ThresholdL ≈ ThresholdU / 2
 *
 * - miRadiusMin/miRadiusMax: Search range for circle radii (in pixels)
 *   * Only circles within [RadiusMin, RadiusMax] will be detected
 *   * Setting these correctly dramatically improves performance and accuracy
 *   * Too wide: Slower detection, more false positives
 *   * Too narrow: May miss target circles
 *
 * Visualization Parameters:
 * - mbDisplayPoint: Whether to draw circle centers as points
 * - mucPointColor: RGB color for center points [0-255]
 * - miPointSize: Radius of center point marker (pixels)
 *
 * - mbDisplayCircle: Whether to draw full circles
 * - mucCircleColor: RGB color for circles [0-255]
 * - miCircleThickness: Circle line width (pixels, -1 for filled)
 * - miCircleType: Line rendering type (LINE_AA for anti-aliased, LINE_8 for 8-connected)
 *
 * Parameter Tuning Tips:
 * 1. Start with default values and adjust one parameter at a time
 * 2. Set RadiusMin/Max as tight as possible for your application
 * 3. If too many circles: Increase ThresholdL or CenterDistance
 * 4. If too few circles: Decrease ThresholdL or ThresholdU
 * 5. Use edge-detected or preprocessed images for best results
 */
typedef struct CVHoughCircleTransformParameters{
    int miHoughMethod;          ///< Hough detection method (cv::HOUGH_GRADIENT)
    double mdInverseRatio;      ///< Inverse accumulator resolution ratio (1.0 = full resolution)
    double mdCenterDistance;    ///< Minimum distance between circle centers (pixels)
    double mdThresholdU;        ///< Upper threshold for center detection (Canny high threshold)
    double mdThresholdL;        ///< Lower threshold for accumulator voting (detection sensitivity)
    int miRadiusMin;            ///< Minimum circle radius to search for (pixels)
    int miRadiusMax;            ///< Maximum circle radius to search for (pixels)
    bool mbDisplayPoint;        ///< Whether to draw circle center points
    int mucPointColor[3];       ///< RGB color for center point markers [0-255]
    int miPointSize;            ///< Radius of center point marker (pixels)
    bool mbDisplayCircle;       ///< Whether to draw full circles
    int mucCircleColor[3];      ///< RGB color for circle outlines [0-255]
    int miCircleThickness;      ///< Circle line thickness (pixels, -1 for filled)
    int miCircleType;           ///< Circle line type (cv::LINE_AA, LINE_8, LINE_4)

    bool mbEnableGradient;      ///< Enable gradient-based detection refinement
    /**
     * @brief Default constructor initializing all parameters to recommended values.
     *
     * Default Configuration:
     * - Method: HOUGH_GRADIENT (the only currently supported method)
     * - Inverse Ratio: 1.0 (full resolution accumulator for maximum accuracy)
     * - Center Distance: 10 pixels (minimum separation between circles)
     * - Thresholds: 200 (upper), 100 (lower) - moderate sensitivity
     * - Radius Range: 5-20 pixels (suitable for small-to-medium circles)
     * - Visualization: Both points (blue) and circles (blue) enabled
     * - Point/Circle Size: 3 pixels thickness
     * - Line Type: LINE_AA (anti-aliased for smooth rendering)
     *
     * These defaults work well for general-purpose circle detection, but should be
     * tuned for specific applications by adjusting radius range and thresholds.
     */
    CVHoughCircleTransformParameters()
        : miHoughMethod(cv::HOUGH_GRADIENT),
          mdInverseRatio(1),
          mdCenterDistance(10),
          mdThresholdU(200),
          mdThresholdL(100),
          miRadiusMin(5),
          miRadiusMax(20),
          mbDisplayPoint(true),
          mucPointColor{0},
          miPointSize(3),
          mbDisplayCircle(true),
          mucCircleColor{0},
          miCircleThickness(3),
          miCircleType(cv::LINE_AA)
    {
    }
} CVHoughCircleTransformParameters;

/**
 * @class CVHoughCircleTransformModel
 * @brief Node for detecting circular shapes in images using the Hough Circle Transform.
 *
 * This model implements circle detection using OpenCV's cv::CVHoughCircles function, which
 * is based on the Hough Transform technique for identifying geometric shapes. The algorithm
 * is particularly effective at detecting circles even when they are partially occluded or
 * have slight irregularities in their shape.
 *
 * Algorithm Overview:
 * The Hough Circle Transform operates in three main stages:
 *
 * 1. Edge Detection:
 *    - Computes image gradients using Sobel operator
 *    - Applies Canny edge detection with ThresholdU/ThresholdL
 *    - Edge pixels become candidate points for circle boundaries
 *
 * 2. Center Detection (Accumulator Voting):
 *    - For each edge pixel, vote for potential circle centers
 *    - Votes accumulate in a 2D accumulator array (scaled by InverseRatio)
 *    - Local maxima in accumulator indicate likely circle centers
 *    - Centers closer than CenterDistance are merged
 *
 * 3. Radius Estimation:
 *    - For each detected center, search for best-fitting radius
 *    - Test all radii in range [RadiusMin, RadiusMax]
 *    - Count edge pixels at each radius distance from center
 *    - Radius with maximum edge pixel support is selected
 *
 * Mathematical Foundation:
 * A circle is defined by three parameters: center (x₀, y₀) and radius r.
 * For a point (x, y) on the circle:
 *   (x - x₀)² + (y - y₀)² = r²
 *
 * The Hough Transform maps this to a 3D parameter space (x₀, y₀, r).
 * Each edge pixel votes for all possible circles passing through it.
 *
 * Input Requirements:
 * - Single-channel 8-bit grayscale image
 * - Edges can be pre-detected (recommended) or algorithm will detect them
 * - Image should have good contrast between circles and background
 * - Preprocessing (blur, threshold) often improves results
 *
 * Outputs:
 * - Port 0 (CVImageData): Visualization image with detected circles drawn
 *   * Original image converted to BGR (if grayscale)
 *   * Circle centers marked as colored points (if enabled)
 *   * Circle outlines drawn as colored arcs (if enabled)
 *   * Can overlay both points and circles simultaneously
 *
 * - Port 1 (IntegerData): Count of detected circles
 *   * Useful for automated counting applications
 *   * Enables threshold-based decisions in workflows
 *   * Can trigger alerts when count exceeds limits
 *
 * Common Use Cases:
 *
 * 1. Coin Counting:
 *    ```
 *    ImageLoader → GaussianBlur → CVHoughCircleTransform → Display
 *                                         ↓
 *                                    Circle Count → Information
 *    ```
 *    Settings: RadiusMin/Max based on coin size, moderate thresholds
 *
 * 2. Eye/Pupil Tracking:
 *    ```
 *    Camera → ROI (face region) → InRange (dark pixels) → CVHoughCircleTransform
 *    ```
 *    Settings: Small radius range (5-15px), high sensitivity (low ThresholdL)
 *
 * 3. Industrial Part Inspection:
 *    ```
 *    Camera → Threshold → CVHoughCircleTransform → Quality Check
 *                              ↓
 *                        Count verification (expect N circles)
 *    ```
 *    Settings: Tight radius tolerances, high CenterDistance to avoid duplicates
 *
 * 4. Cell Counting (Microscopy):
 *    ```
 *    Microscope → Enhance Contrast → Threshold → CVHoughCircleTransform
 *    ```
 *    Settings: Biological cell size range, allow overlapping (low CenterDistance)
 *
 * 5. Traffic Sign Detection:
 *    ```
 *    Camera → ColorFilter (red/blue) → CannyEdge → CVHoughCircleTransform
 *    ```
 *    Settings: Large radius range (20-100px), conservative thresholds
 *
 * Performance Characteristics:
 * - Complexity: O(N × R × W × H) where N = edge pixels, R = radius range, W×H = image size
 * - Typical Processing Time:
 *   * 640×480 image, 10-30px radius range: 10-50ms
 *   * 1920×1080 image, full radius scan: 100-500ms
 * - Performance Optimization Tips:
 *   * Use smallest possible radius range
 *   * Increase InverseRatio (2.0) for 4x speedup at cost of precision
 *   * Preprocess with GaussianBlur to reduce edge noise
 *   * Use Canny edge detection before this node for faster processing
 *
 * Parameter Tuning Guidelines:
 *
 * Problem: Too many false circles detected
 * Solution:
 * - Increase ThresholdL (more selective accumulator voting)
 * - Increase CenterDistance (reduce duplicate detections)
 * - Narrow RadiusMin/Max range
 * - Preprocess with stronger blur or threshold
 *
 * Problem: Missing valid circles
 * Solution:
 * - Decrease ThresholdL (more sensitive detection)
 * - Decrease ThresholdU (detect weaker edges)
 * - Widen RadiusMin/Max range
 * - Reduce CenterDistance (allow closer circles)
 * - Use InverseRatio=1.0 for higher precision
 *
 * Problem: Circles detected but with wrong radii
 * Solution:
 * - Adjust RadiusMin/Max to expected size range
 * - Use preprocessing to enhance circle boundaries
 * - Increase image contrast before detection
 *
 * Limitations:
 * - Cannot detect ellipses (circles only)
 * - Struggles with heavily occluded or incomplete circles
 * - Performance degrades with wide radius ranges
 * - May miss circles on image boundaries
 * - Sensitive to noise (use GaussianBlur preprocessing)
 *
 * Best Practices:
 * 1. Always preprocess with GaussianBlur (kernel size 5 or 7)
 * 2. Set RadiusMin/Max as narrow as possible for your application
 * 3. Start with default thresholds and adjust ThresholdL first
 * 4. Use edge detection visualization to verify edge quality
 * 5. For overlapping circles, reduce CenterDistance carefully
 * 6. Consider image resolution: rescale large images for speed
 * 7. For real-time applications, use InverseRatio=2.0
 *
 * Comparison with Other Detection Methods:
 * - vs. Template Matching: Hough is rotation-invariant, works with partial circles
 * - vs. Blob Detection: Hough finds exact circles, blob finds arbitrary shapes
 * - vs. Contour Detection: Hough more robust to gaps and noise in boundaries
 *
 * @see CVHoughCircleTransformParameters, cv::CVHoughCircles, cv::HOUGH_GRADIENT
 */

/**
 * @class CVHoughCircleTransformWorker
 * @brief Worker class for asynchronous circle detection
 */
class CVHoughCircleTransformWorker : public QObject
{
    Q_OBJECT
public:
    CVHoughCircleTransformWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     int houghMethod,
                     double inverseRatio,
                     double centerDistance,
                     double thresholdU,
                     double thresholdL,
                     int radiusMin,
                     int radiusMax,
                     bool displayPoint,
                     unsigned char pointColorB,
                     unsigned char pointColorG,
                     unsigned char pointColorR,
                     int pointSize,
                     bool displayCircle,
                     unsigned char circleColorB,
                     unsigned char circleColorG,
                     unsigned char circleColorR,
                     int circleThickness,
                     int circleType,
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId);

Q_SIGNALS:
    // CRITICAL: This signal MUST be declared in each worker class
    // CANNOT be inherited from base class due to Qt MOC limitation
    void frameReady(std::shared_ptr<CVImageData> img, std::shared_ptr<IntegerData> count);
};

class CVHoughCircleTransformModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVHoughCircleTransformModel();

    virtual
    ~CVHoughCircleTransformModel() override {}

    QJsonObject
    save() const override;

    void
    load(const QJsonObject &p) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    // Override base class to handle 3 outputs instead of 2
    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    static const QString _category;

    static const QString _model_name;

protected:
    // Implement PBAsyncDataModel pure virtuals
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;

private:
    void process_cached_input() override;

    CVHoughCircleTransformParameters mParams;              ///< Current detection and visualization parameters
    std::shared_ptr<IntegerData> mpIntegerData { nullptr }; ///< Output count of detected circles
    QPixmap _minPixmap;                                  ///< Node icon for visual representation

    static const std::string color[3];                   ///< Color channel names for property system

    // Pending data for backpressure
    cv::Mat mPendingFrame;
    CVHoughCircleTransformParameters mPendingParams;
};

