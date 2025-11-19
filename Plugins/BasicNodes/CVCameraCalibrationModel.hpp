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
 * @file CVCameraCalibrationModel.hpp
 * @brief Camera calibration node for computing intrinsic/extrinsic parameters.
 *
 * This file defines a comprehensive camera calibration system supporting multiple
 * calibration patterns (chessboard, circles, ChArUco) with automatic corner detection,
 * multi-image accumulation, and parameter estimation.
 *
 * **Calibration Process:**
 * 1. Configure pattern type (chessboard/circles/ChArUco) and dimensions
 * 2. Capture/import multiple images of calibration pattern from different angles
 * 3. Automatic corner detection and accumulation
 * 4. Compute camera matrix, distortion coefficients, and optionally extrinsics
 * 5. Export parameters to YAML file for undistortion/3D reconstruction
 *
 * **Supported Calibration Patterns:**
 * - **Chessboard**: Classic black/white grid (e.g., 11×7 squares)
 * - **Circles Grid**: Symmetric circular pattern
 * - **Asymmetric Circles**: Offset circle pattern
 * - **ChArUco Board**: Hybrid chessboard+ArUco markers (robust to partial occlusion)
 *
 * **Output Parameters:**
 * - Camera matrix (fx, fy, cx, cy) - intrinsic parameters
 * - Distortion coefficients (k1, k2, p1, p2, k3) - radial and tangential distortion
 * - Rotation/translation vectors (per image) - extrinsic parameters (optional)
 * - Reprojection error - calibration quality metric (pixels RMS)
 *
 * **Typical Calibration Workflow:**
 * ```
 * CVCamera → CVCameraCalibration → [Auto-Capture 15-20 images] → Calibrate → Save YAML
 * CVImageLoader → CVCameraCalibration → [Manual image selection] → Calibrate → Save YAML
 * ```
 *
 * **Common Applications:**
 * - Fisheye lens correction
 * - Stereo vision setup
 * - 3D reconstruction preparation
 * - Augmented reality (AR) systems
 * - Robot vision calibration
 * - Measurement accuracy improvement
 *
 * @see cv::calibrateCamera for underlying calibration algorithm
 * @see cv::findChessboardCorners, cv::findCirclesGrid for corner detection
 * @see cv::aruco::CharucoDetector for ChArUco detection
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "CVCameraCalibrationEmbeddedWidget.hpp"
#include <opencv2/aruco.hpp>
#include <opencv2/objdetect/charuco_detector.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @enum CameraCalibPattern
 * @brief Supported calibration pattern types.
 */
enum CameraCalibPattern { 
    CHESSBOARD = 0,             ///< Classic chessboard (e.g., 11×7 squares)
    CIRCLES_GRID,               ///< Symmetric circles grid
    ASYMMETRIC_CIRCLES_GRID,    ///< Asymmetric circles (offset rows)
    CHARUCOBOARD                ///< Hybrid ChArUco board (chessboard + ArUco markers)
};

/**
 * @enum ArucoDict
 * @brief ArUco dictionary types for ChArUco boards.
 *
 * Different dictionaries offer trade-offs between number of unique markers
 * and detection robustness. Higher bit counts (6×6, 7×7) are more robust but
 * have fewer unique IDs.
 */
enum ArucoDict { 
    DICT4x4_50 = 0, DICT4x4_100, DICT4x4_250, DICT4x4_1000,     ///< 4×4 bit markers
    DICT5x5_50, DICT5x5_100, DICT5x5_250, DICT5x5_1000,         ///< 5×5 bit markers
    DICT6x6_50, DICT6x6_100, DICT6x6_250, DICT6x6_1000,         ///< 6×6 bit markers
    DICT7x7_50, DICT7x7_100, DICT7x7_250, DICT7x7_1000,         ///< 7×7 bit markers
    DICTOriginal, DICTApriltag_16h5, DICTApriltag_25h9,         ///< AprilTag families
    DICTApriltag_36h10, DICTApriltag_36h11                      ///< AprilTag families
};

/**
 * @enum ThreadState
 * @brief Calibration thread processing states.
 */
enum ThreadState { 
    THREAD_INIT = 0,            ///< Initialized, waiting for images
    THREAD_DETECT_CORNERS = 1,  ///< Detecting corners in new image
    THREAD_CALIBRATE = 2,       ///< Computing calibration parameters
    THREAD_STOP = 3             ///< Shutting down thread
};

/**
 * @struct _CameraCalibrationParams
 * @brief Comprehensive calibration configuration parameters.
 *
 * This structure contains all settings for pattern detection, corner refinement,
 * and calibration computation.
 *
 * **Pattern Configuration:**
 * - **Chessboard**: Set miChessBoard_Cols/Rows (interior corners, not squares)
 *   - Example: 11×7 board has 10×6 interior corners
 * - **ChArUco**: Set board size + ArUco dictionary + marker/square sizes
 * - **Circles**: Set grid dimensions and spacing
 *
 * **Calibration Flags (miFlags):**
 * - CV_CALIB_FIX_ASPECT_RATIO: Force fx = fy * mfFixAspectRation
 * - CV_CALIB_FIX_PRINCIPAL_POINT: Fix cx, cy at image center
 * - CV_CALIB_ZERO_TANGENTIAL_DIST: Set p1=p2=0 (no tangential distortion)
 * - CV_CALIB_FIX_K3: Don't estimate k3 (only k1, k2)
 * - CV_CALIB_RATIONAL_MODEL: Use 6-coefficient model (k1-k6)
 *
 * **Corner Refinement:**
 * - Search window size (miSearchWindow_Width/Height) affects sub-pixel accuracy
 * - Larger windows more robust but slower
 * - Typical: 5×5 to 11×11 pixels
 *
 * @note mfSquare_Size and mfMarker_Size are in real-world units (mm, cm, inches).
 *       Consistent units required for 3D reconstruction.
 */
typedef struct _CameraCalibrationParams{
    int miPattern { CHESSBOARD };                   ///< Pattern type (CHESSBOARD, CIRCLES_GRID, etc.)
    int miArucoDict { DICT4x4_50 };                 ///< ArUco dictionary for ChArUco boards
    int miChessBoard_Cols{11};                      ///< Chessboard columns (interior corners)
    int miChessBoard_Rows{7};                       ///< Chessboard rows (interior corners)
    int miSearchWindow_Width{11};                   ///< Corner refinement window width (pixels)
    int miSearchWindow_Height{11};                  ///< Corner refinement window height (pixels)
    float mfSquare_Size{30};                        ///< Chessboard square size (mm, cm, or inches)
    float mfMarker_Size{20};                        ///< ArUco marker size for ChArUco (same units as square)
    float mfActualDistanceTopLeftRightCorner{180};  ///< Optional: actual distance for scale validation (mm)
    float mfFixAspectRation{0};                     ///< Fixed aspect ratio fx/fy (0=compute both independently)
    bool mbEnableK3{false};                         ///< Estimate k3 distortion coefficient
    bool mbWriteDetectedFeatures{false};            ///< Save detected corners to file (debug)
    bool mbWriteExtrinsicParams{false};             ///< Save rotation/translation vectors
    bool mbWriteRefined3DPoints{false};             ///< Save 3D object points after refinement
    bool mbAssumeZeroTangentialDistortion{false};   ///< Force p1=p2=0 (radial distortion only)
    bool mbFixPrincipalPointAtCenter{false};        ///< Fix cx=width/2, cy=height/2
    bool mbFlipImages{false};                       ///< Flip images vertically before processing
    bool mbSaveUndistortedImages{false};            ///< Save undistorted versions of calibration images
    int miFlags{0};                                 ///< cv::calibrateCamera flags (computed from booleans)
} CameraCalibrationParams;

/**
 * @class CameraCalibrationThread
 * @brief Background thread for corner detection and calibration computation.
 *
 * This thread handles CPU-intensive operations:
 * - Corner detection in new calibration images (findChessboardCorners, etc.)
 * - Calibration parameter estimation (calibrateCamera)
 * - Reprojection error computation
 *
 * **Thread Workflow:**
 * ```
 * while (!abort) {
 *     wait_for_command();
 *     
 *     if (command == DETECT_CORNERS) {
 *         corners = detect_pattern(image);
 *         if (corners.valid) {
 *             accumulate_image_and_corners();
 *             emit result_image_ready(annotated_image);
 *         }
 *     }
 *     else if (command == CALIBRATE) {
 *         [cameraMatrix, distCoeffs, rvecs, tvecs] = cv::calibrateCamera(...);
 *         reprojection_error = computeReprojectionErrors(...);
 *         save_to_yaml();
 *         emit result_image_ready(undistorted_sample);
 *     }
 * }
 * ```
 *
 * **Corner Detection Methods:**
 * - **Chessboard**: cv::findChessboardCorners + cv::cornerSubPix (sub-pixel refinement)
 * - **Circles**: cv::findCirclesGrid
 * - **ChArUco**: cv::aruco::CharucoDetector (robust to partial occlusion)
 *
 * **Calibration Quality Metrics:**
 * - **Reprojection Error**: RMS pixel distance between detected and projected corners
 *   - Good: < 0.5 pixels
 *   - Acceptable: 0.5-1.0 pixels
 *   - Poor: > 1.0 pixels (recalibrate or add more images)
 * - **Per-Image Errors**: Identify outlier images for removal
 *
 * @note Thread safety: Uses QSemaphore for command synchronization.
 */
class CameraCalibrationThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Constructor initializes thread.
     * @param parent Parent QObject for memory management
     */
    explicit
    CameraCalibrationThread( QObject *parent = nullptr );

    ~CameraCalibrationThread() override;

    /**
     * @brief Sets calibration parameters (pattern type, dimensions, flags).
     * @param params CameraCalibrationParams struct
     */
    void
    set_params( CameraCalibrationParams & params );

    /**
     * @brief Queues corner detection for a new calibration image.
     * @param image Calibration image containing pattern
     * @note Emits result_image_ready() with annotated image after corner detection.
     */
    void
    detect_corners( const cv::Mat & );

    /**
     * @brief Triggers calibration computation using all accumulated images.
     * Computes camera matrix, distortion coefficients, and saves to YAML file.
     * @note Emits result_image_ready() with undistorted sample image.
     */
    void
    calibrate();

    /**
     * @brief Starts the background thread.
     */
    void
    start_thread( );

    /**
     * @brief Stops the thread gracefully.
     */
    void
    stop_thread();

    /**
     * @brief Gets accumulated calibration images.
     * @return Vector of cv::Mat images used for calibration
     */
    std::vector< cv::Mat > &
    get_images() { return mvCVImages; };

Q_SIGNALS:
    /**
     * @brief Emitted when an error occurs (pattern not found, calibration failed, etc.).
     * @param error_code Error identifier
     */
    void
    error_signal(int);

    /**
     * @brief Emitted when result image is ready (annotated corners or undistorted image).
     * @param image Result cv::Mat (for display or saving)
     */
    void
    result_image_ready( cv::Mat & image );

protected:
    /**
     * @brief Main thread loop executing corner detection and calibration commands.
     */
    void
    run() override;

private:
    /**
     * @brief Computes RMS reprojection error for calibration quality assessment.
     *
     * Reprojection error measures how well the calibrated camera model explains
     * observed corner positions. Lower error indicates better calibration.
     *
     * **Calculation:**
     * ```
     * for each image:
     *     projected_points = project(object_points, rvec, tvec, camera_matrix, dist_coeffs)
     *     error = norm(detected_points - projected_points)
     * total_error = sqrt(sum(error^2) / total_points)
     * ```
     *
     * @param objectPoints 3D object points (chessboard corners in world coordinates)
     * @param imagePoints 2D detected corners (pixel coordinates)
     * @param rvecs Rotation vectors per image
     * @param tvecs Translation vectors per image
     * @param cameraMatrix Camera intrinsic matrix [fx 0 cx; 0 fy cy; 0 0 1]
     * @param distCoeffs Distortion coefficients [k1 k2 p1 p2 k3]
     * @param perViewErrors Output: RMS error per image (for outlier detection)
     * @return Total RMS reprojection error (pixels)
     */
    double computeReprojectionErrors(
        const std::vector< std::vector<cv::Point3f> >& objectPoints,
        const std::vector< std::vector<cv::Point2f> >& imagePoints,
        const std::vector< cv::Mat >& rvecs, const std::vector< cv::Mat >& tvecs,
        const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs,
        std::vector< float >& perViewErrors );

    /**
     * @brief Generates 3D object points for calibration pattern.
     *
     * Creates 3D coordinates of pattern corners in pattern coordinate system
     * (typically Z=0 plane, with X-Y grid based on square size).
     *
     * **Example (11×7 chessboard, 30mm squares):**
     * ```
     * corners = [(0,0,0), (30,0,0), (60,0,0), ..., (300,180,0)]
     * ```
     *
     * @param boardSize Pattern dimensions (cols × rows interior corners)
     * @param squareSize Physical size of one square (mm, cm, inches)
     * @param corners Output: 3D points vector
     * @param patternType Pattern type (affects point generation for asymmetric grids)
     */
    void
    calcChessboardCorners(cv::Size boardSize, float squareSize, std::vector< cv::Point3f >& corners, CameraCalibPattern patternType = CHESSBOARD);

    /**
     * @brief Executes full calibration computation.
     *
     * Wraps cv::calibrateCamera with additional logic for:
     * - Flag configuration (aspect ratio, principal point, distortion model)
     * - Object point generation
     * - Reprojection error calculation
     * - Per-image error reporting
     *
     * @param imagePoints Detected 2D corners for all images
     * @param imageSize Image dimensions (width × height)
     * @param boardSize Pattern dimensions
     * @param patternType Pattern type
     * @param squareSize Physical square size
     * @param aspectRatio Fixed aspect ratio (0=compute independently)
     * @param grid_width ChArUco grid width (for scale validation)
     * @param release_object Enable object point refinement
     * @param flags cv::calibrateCamera flags
     * @param cameraMatrix Output: 3×3 camera matrix
     * @param distCoeffs Output: distortion coefficients
     * @param rvecs Output: rotation vectors per image
     * @param tvecs Output: translation vectors per image
     * @param reprojErrs Output: per-image reprojection errors
     * @param newObjPoints Output: refined 3D object points (if release_object=true)
     * @param totalAvgErr Output: total RMS error
     * @return true if calibration successful, false otherwise
     */
    bool
    runCalibration( std::vector< std::vector<cv::Point2f> > imagePoints, cv::Size imageSize, cv::Size boardSize, CameraCalibPattern patternType,
                   float squareSize, float aspectRatio, float grid_width, bool release_object, int flags, cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
                   std::vector< cv::Mat >& rvecs, std::vector< cv::Mat >& tvecs, std::vector< float >& reprojErrs, std::vector< cv::Point3f >& newObjPoints,
                   double& totalAvgErr );

    QSemaphore mWaitingSemaphore;               ///< Command synchronization
    cv::Mat mCVImage;                           ///< Current image being processed
    CameraCalibrationParams mCameraCalibrationParams; ///< Calibration config

    cv::Size mBoardSize;                        ///< Pattern dimensions (cols × rows)
    cv::Size mSearchWindowSize;                 ///< Corner refinement window size
    int miThreadState { THREAD_INIT };          ///< Current thread state
    bool mbThreadReady {false};                 ///< Thread initialized and running
    bool mbAbort {false};                       ///< Abort flag for clean shutdown
    std::vector< cv::Mat > mvCVImages;          ///< Accumulated calibration images

    cv::aruco::CharucoDetector * mpCharucoDetector{ nullptr }; ///< ChArUco detector instance
};

/**
 * @class CVCameraCalibrationModel
 * @brief Camera calibration node with multi-pattern support and threaded processing.
 *
 * CVCameraCalibrationModel provides a complete camera calibration workflow from
 * image capture through parameter estimation and YAML export. It supports multiple
 * calibration patterns, automatic corner detection, quality metrics, and visualization.
 *
 * **Port Configuration:**
 * - **Input:** CVImageData - Calibration images (from camera or file loader)
 * - **Output:** CVImageData - Annotated images (detected corners or undistorted result)
 *
 * **Embedded Widget Controls:**
 * - Pattern type selection (Chessboard/Circles/ChArUco)
 * - Pattern dimensions (rows, columns, square size)
 * - Capture/Add image button
 * - Calibrate button (computes parameters)
 * - Image gallery (review captured images)
 * - Delete image button (remove outliers)
 *
 * **Calibration Workflow:**
 *
 * 1. **Setup Configuration:**
 *    ```
 *    - Select pattern type (Chessboard recommended for beginners)
 *    - Set pattern dimensions (e.g., 11×7 for 11 columns, 7 rows)
 *    - Set square size in real-world units (e.g., 30mm)
 *    - Connect CVCamera or CVImageLoader to input
 *    ```
 *
 * 2. **Capture Calibration Images (15-20 recommended):**
 *    ```
 *    - Cover different regions of image (center, corners, edges)
 *    - Vary pattern orientation (tilted, rotated)
 *    - Vary distance (close and far)
 *    - Click "Capture" for each good image
 *    - Node shows detected corners overlay (green if detected, red if failed)
 *    ```
 *
 * 3. **Compute Calibration:**
 *    ```
 *    - Click "Calibrate" button
 *    - Wait for computation (5-30 seconds depending on image count)
 *    - Check reprojection error (should be < 1.0 pixels)
 *    - Review undistorted sample output
 *    ```
 *
 * 4. **Export Parameters:**
 *    ```
 *    - YAML file saved to specified path (default: camera_params.yaml)
 *    - File contains:
 *      * Camera matrix (fx, fy, cx, cy)
 *      * Distortion coefficients (k1, k2, p1, p2, k3)
 *      * Image dimensions
 *      * Reprojection error
 *      * Optional: Rotation/translation vectors per image
 *    ```
 *
 * **Pattern Selection Guide:**
 * - **Chessboard**: Most common, easy to print, sub-pixel accuracy
 *   - Pros: High accuracy, widely supported
 *   - Cons: Requires full pattern visibility
 * - **Circles Grid**: Robust to lighting, simpler detection
 *   - Pros: Faster detection, works in varied lighting
 *   - Cons: Lower sub-pixel accuracy
 * - **ChArUco**: Best for partial occlusion scenarios
 *   - Pros: Robust to occlusion, each marker identifiable
 *   - Cons: Requires ArUco library, more complex setup
 *
 * **Common Use Cases:**
 *
 * 1. **Fisheye Lens Correction:**
 *    ```
 *    CVCamera → CVCameraCalibration → [Calibrate] → Save YAML
 *    CVCamera → UndistortImage(YAML) → CVImageDisplay
 *    ```
 *
 * 2. **Stereo Vision Setup:**
 *    ```
 *    LeftCamera → CVCameraCalibration → left_params.yaml
 *    RightCamera → CVCameraCalibration → right_params.yaml
 *    StereoCalibration(left_params, right_params) → stereo_params.yaml
 *    ```
 *
 * 3. **3D Reconstruction Preparation:**
 *    ```
 *    CVCamera → CVCameraCalibration → camera_params.yaml
 *    CVCamera → FeatureDetect → StereoMatch(camera_params) → PointCloud
 *    ```
 *
 * 4. **AR Marker Tracking:**
 *    ```
 *    CVCamera → CVCameraCalibration → params.yaml
 *    CVCamera → ArUcoDetect(params) → PoseEstimation → Render3DObject
 *    ```
 *
 * **Calibration Quality Tips:**
 * - **Image Count**: 15-20 images minimum, 30-50 for high accuracy
 * - **Coverage**: Ensure pattern appears in all image regions
 * - **Orientation**: Include tilted views (not just frontal)
 * - **Distance Variation**: Capture at multiple depths
 * - **Focus**: Ensure pattern is in focus (no motion blur)
 * - **Lighting**: Even illumination (avoid glare on pattern)
 * - **Resolution**: Use camera's native resolution
 *
 * **Troubleshooting:**
 * - **Pattern Not Detected**:
 *   - Check pattern dimensions match configuration
 *   - Improve lighting (reduce shadows/glare)
 *   - Ensure pattern is fully visible and flat
 *   - Try different distance or angle
 * - **High Reprojection Error (>1.0 pixels)**:
 *   - Remove outlier images (check per-image errors)
 *   - Add more images with better coverage
 *   - Verify pattern square size is correct
 *   - Check for motion blur or focus issues
 * - **Asymmetric Distortion**:
 *   - Disable "Fix Aspect Ratio" constraint
 *   - Enable tangential distortion (disable "Zero Tangential Distortion")
 *
 * **Advanced Calibration Flags:**
 * - **Fix Aspect Ratio**: Force fx = fy (for square pixels)
 * - **Fix Principal Point**: Assume optical center at image center
 * - **Zero Tangential Distortion**: Only radial distortion (lens decentering negligible)
 * - **Enable K3**: Include third radial distortion term (for strong fisheye)
 * - **Rational Model**: Use 6-coefficient distortion (k1-k6 for extreme distortion)
 *
 * **YAML Output Format:**
 * ```yaml
 * camera_matrix:
 *   rows: 3
 *   cols: 3
 *   data: [fx, 0, cx, 0, fy, cy, 0, 0, 1]
 * distortion_coefficients:
 *   rows: 1
 *   cols: 5
 *   data: [k1, k2, p1, p2, k3]
 * image_width: 1920
 * image_height: 1080
 * avg_reprojection_error: 0.42
 * ```
 *
 * **Performance Characteristics:**
 * - Corner detection: ~50-200ms per image (resolution-dependent)
 * - Calibration computation: ~5-30 seconds (image count and resolution)
 * - Threaded processing: UI remains responsive during computation
 *
 * **Design Rationale:**
 * - Threaded detection/calibration prevents UI freezing during long computations
 * - Multiple pattern support accommodates different application needs
 * - Image gallery allows quality review and outlier removal
 * - Real-time corner overlay provides immediate feedback
 * - YAML export ensures compatibility with OpenCV standard format
 *
 * @note Physical square size units (mm, cm, inches) must be consistent for 3D reconstruction.
 * @see cv::calibrateCamera for calibration algorithm details
 * @see cv::findChessboardCorners, cv::findCirclesGrid, cv::aruco::CharucoDetector
 * @see CVCamera for live image capture
 * @see CVImageLoader for loading pre-captured calibration images
 */
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class CVCameraCalibrationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVCameraCalibrationModel();

    virtual
    ~CVCameraCalibrationModel() override
    {
        if( mpCameraCalibrationThread )
            delete mpCameraCalibrationThread;
    }

    QJsonObject
    save() const override;

    void
    load(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr< NodeData >
    outData( PortIndex port ) override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    void
    setModelProperty( QString &, const QVariant & ) override;

    void
    late_constructor() override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:
    /**
     * @brief Handles calibration thread errors (pattern not found, calibration failure, etc.).
     * @param error_code Error identifier
     */
    void
    thread_error_occured(int);
    
    /**
     * @brief Receives result image from thread (annotated corners or undistorted).
     * @param image Result cv::Mat for display
     */
    void
    received_result( cv::Mat & );
    
    /**
     * @brief Processes embedded widget button clicks (Capture, Calibrate, Delete, etc.).
     * @param button Button identifier
     */
    void
    em_button_clicked( int button );

private:
    /**
     * @brief Constructs cv::calibrateCamera flags from boolean parameters.
     * Translates user-friendly checkboxes into OpenCV flag bitmask.
     */
    void
    set_flags();

    CVCameraCalibrationEmbeddedWidget * mpEmbeddedWidget; ///< UI controls

    std::shared_ptr< CVImageData > mpCVImageData { nullptr }; ///< Output image data
    CameraCalibrationThread * mpCameraCalibrationThread { nullptr }; ///< Background thread
    bool mbAutoCapture{false};                  ///< Auto-capture mode (capture every input frame)
    cv::Mat mOrgCVImage;                        ///< Original input image (before annotation)
    bool mbInMemoryImage{false};                ///< Image stored in memory (vs. file path)
    int miCurrentDisplayImage{-1};              ///< Currently displayed image index in gallery

    EnumPropertyType mEnumPattern;              ///< Pattern type property (UI)
    EnumPropertyType mEnumArucoDict;            ///< ArUco dictionary property (UI)

    CameraCalibrationParams mCameraCalibrationParams; ///< Calibration configuration

    QString msOutputFilename {"camera_params.yaml"}; ///< YAML output file path
    QString msWorkingDirname {""};              ///< Working directory for image/file operations

    /**
     * @brief Processes input calibration image (triggers corner detection).
     * @param in Input CVImageData containing calibration pattern
     */
    void processData( const std::shared_ptr< CVImageData > & in );
};
