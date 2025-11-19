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
 * @file CVCameraModel.hpp
 * @brief Camera capture node for real-time image acquisition from USB/V4L2 cameras.
 *
 * This file defines the CVCameraModel node which provides live camera feed integration
 * for computer vision pipelines. It wraps OpenCV's VideoCapture interface with threaded
 * capture, parameter control, and both continuous and triggered acquisition modes.
 *
 * **Key Features:**
 * - Multi-camera support with device ID selection (0, 1, 2, ...)
 * - Configurable capture parameters (resolution, FPS, codec, exposure, gain, white balance)
 * - Threaded capture to prevent UI blocking
 * - Dual operating modes:
 *   * **Continuous Mode**: Stream at configured FPS when no sync input connected
 *   * **Single-Shot Mode**: Capture triggered by sync signal input
 * - Embedded widget for camera control (start/stop, device selection)
 * - Real-time FPS monitoring
 * - Camera status feedback (connected/disconnected)
 *
 * **Typical Camera Pipeline:**
 * ```
 * CVCamera → CVImageDisplay        (Live preview)
 * CVCamera → FaceDetection → Draw  (Real-time detection)
 * CVCamera → Threshold → FindContour (Object tracking)
 * Timer → CVCamera → Processing     (Triggered capture at fixed rate)
 * ```
 *
 * **Common Applications:**
 * - Live video processing and analysis
 * - Object detection and tracking
 * - Quality inspection systems
 * - Barcode/QR code scanning
 * - Motion detection
 * - Time-lapse capture
 * - Multi-camera surveillance
 *
 * @see CVVDOLoaderModel for pre-recorded video playback
 * @see CVCameraCalibrationModel for camera calibration workflows
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include "PBNodeDelegateModel.hpp"

#include "SyncData.hpp"
#include "CVImageData.hpp"
#include "InformationData.hpp"

#include "CVCameraEmbeddedWidget.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVCameraParameters
 * @brief Camera capture configuration parameters.
 *
 * This structure encapsulates all configurable camera properties for VideoCapture.
 * Parameters map to cv::VideoCapture properties via CAP_PROP_* constants.
 *
 * **Parameter Details:**
 * - **FourCC Codec**: Determines compression format
 *   - MJPG: Motion JPEG (widely supported, good quality)
 *   - YUYV: Uncompressed (high bandwidth, maximum quality)
 *   - H264: Hardware compression (lower bandwidth, higher latency)
 * - **Resolution**: Must match camera's supported modes
 *   - Common: 640×480, 1280×720, 1920×1080, 2592×1944
 *   - Query camera specs for available resolutions
 * - **Auto vs Manual Control**:
 *   - AutoWB/AutoExposure=1: Camera handles automatically
 *   - AutoWB/AutoExposure=0: Use manual Brightness/Gain/Exposure values
 *
 * **Default Configuration:**
 * - High-resolution mode (2592×1944) with MJPEG compression
 * - Auto white balance and exposure enabled
 * - 25 FPS for smooth playback
 *
 * @note Not all cameras support all parameters. Check cv::VideoCapture::get() return values.
 */
typedef struct CVCameraParameters{
    int miFourCC {cv::VideoWriter::fourcc('M','J','P','G')}; ///< Codec (MJPG, YUYV, etc.) - cv::VideoWriter::fourcc('Y','U','Y','V')
    int miFps {25};                 ///< Frames per second (typical: 15, 25, 30, 60)
    int miWidth {2592};             ///< Frame width in pixels (must match camera capability)
    int miHeight {1944};            ///< Frame height in pixels (must match camera capability)
    int miAutoWB {1};               ///< Auto white balance (1=auto, 0=manual)
    int miBrightness {-10};         ///< Brightness adjustment (-64 to 64, camera-dependent)
    int miGain {70};                ///< Gain/ISO (0-100, camera-dependent)
    int miAutoExposure {1};         ///< Auto exposure (1=auto, 0=manual)
    int miExposure {2000};          ///< Exposure time in μs when AutoExposure=0 (camera-dependent range)
} CVCameraParameters;

/**
 * @class CVCameraThread
 * @brief Background thread for camera capture to prevent UI blocking.
 *
 * This thread manages the VideoCapture instance and handles frame grabbing independently
 * of the main UI thread. It supports both continuous streaming and single-shot triggered modes.
 *
 * **Operating Modes:**
 * 1. **Continuous Mode** (default):
 *    - Grabs frames at configured FPS
 *    - Emits image_ready() signal after each capture
 *    - Used for live streaming pipelines
 *
 * 2. **Single-Shot Mode** (sync input connected):
 *    - Waits for fire_single_shot() trigger
 *    - Captures one frame per trigger
 *    - Used for timer-driven or event-driven capture
 *
 * **Thread Lifecycle:**
 * ```cpp
 * CVCameraThread *thread = new CVCameraThread(parent, imageData);
 * thread->set_camera_id(0);
 * thread->set_params(params);
 * thread->start();  // Begin capture loop
 * // ...
 * thread->requestInterruption();  // Signal stop
 * thread->wait();   // Wait for clean exit
 * ```
 *
 * **Camera Setup Sequence:**
 * 1. Check camera availability (check_camera)
 * 2. Open VideoCapture with device ID
 * 3. Set codec, resolution, FPS
 * 4. Set manual parameters (exposure, gain, brightness) if auto mode disabled
 * 5. Start capture loop
 *
 * **Performance:**
 * - Decouples capture from processing to maintain frame rate
 * - Shared image buffer protected by semaphore
 * - FPS measurement via frame timestamps
 *
 * @note Camera parameters are applied in specific order due to driver dependencies
 *       (codec → resolution → FPS → manual settings).
 */
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class CVCameraThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Constructor initializes thread with shared image buffer.
     * @param parent Parent QObject for Qt memory management
     * @param pCVImageData Shared pointer to image data (written by thread, read by model)
     */
    explicit
    CVCameraThread( QObject *parent, std::shared_ptr< CVImageData > pCVImageData );

    ~CVCameraThread() override;

    /**
     * @brief Sets the camera device ID to open.
     * @param camera_id Device index (0 for default camera, 1, 2, ... for additional cameras)
     */
    void
    set_camera_id( int camera_id );

    /**
     * @brief Updates camera capture parameters (resolution, FPS, codec, exposure, etc.).
     * @param params CVCameraParameters struct with new settings
     * @note Triggers camera_ready() signal with status after applying parameters.
     */
    void
    set_params( CVCameraParameters & );

    /**
     * @brief Gets current camera parameters.
     * @return Reference to CVCameraParameters struct
     */
    CVCameraParameters &
    get_params( ) { return mCameraParams; }

    /**
     * @brief Switches between continuous streaming and single-shot triggered mode.
     * @param mode true=single-shot (wait for triggers), false=continuous streaming
     */
    void
    set_single_shot_mode( const bool mode ) { mSingleShotSemaphore.release(); mbSingleShotMode.store(mode); };

    /**
     * @brief Triggers a single frame capture in single-shot mode.
     * @note No effect if not in single-shot mode.
     */
    void
    fire_single_shot( ) { mSingleShotSemaphore.release(); };

    /**
     * @brief Gets actual measured frame rate.
     * @return Frames per second (may differ from configured FPS due to camera/system limits)
     */
    double
    get_fps( ) { return mdFPS; };

Q_SIGNALS:
    /**
     * @brief Emitted when a new frame is captured and ready in mpCVImageData.
     */
    void
    image_ready( );

    /**
     * @brief Emitted when camera connection status changes.
     * @param status true if camera successfully opened, false if disconnected/failed
     */
    bool
    camera_ready( bool status );

protected:
    /**
     * @brief Main capture loop executed in background thread.
     *
     * **Capture Loop:**
     * ```cpp
     * while (!mbAbort) {
     *     if (single_shot_mode)
     *         wait_for_trigger();
     *     
     *     if (camera.grab() && camera.retrieve(frame)) {
     *         mpCVImageData->data() = frame.clone();
     *         emit image_ready();
     *     }
     *     
     *     msleep(delay_for_fps);
     * }
     * ```
     */
    void
    run() override;

private:
    /**
     * @brief Verifies camera is accessible and opens VideoCapture.
     * @note Emits camera_ready(false) if camera cannot be opened.
     */
    void
    check_camera();

    QSemaphore mCameraCheckSemaphore;     ///< Synchronizes camera parameter updates
    QSemaphore mSingleShotSemaphore;      ///< Blocks thread in single-shot mode until triggered

    int miCameraID{-1};                   ///< Device ID (0, 1, 2, ...)
    bool mbAbort{false};                  ///< Set to true to stop thread
    std::atomic<bool> mbSingleShotMode {false}; ///< true=single-shot, false=continuous
    bool mbConnected{false};              ///< Current camera connection status
    unsigned long miDelayTime{10};        ///< Delay between frames in ms (1000/FPS)
    double mdFPS{0};                      ///< Measured actual frame rate
    CVCameraParameters mCameraParams;     ///< Current capture parameters
    cv::VideoCapture mCVVideoCapture;     ///< OpenCV camera interface

    std::shared_ptr< CVImageData > mpCVImageData; ///< Shared output buffer
};

/**
 * @class CVCameraModel
 * @brief Live camera capture node with parameter control and dual operating modes.
 *
 * CVCameraModel provides real-time image acquisition from USB, built-in, or V4L2 cameras.
 * It wraps cv::VideoCapture in a background thread to maintain frame rate while allowing
 * UI responsiveness. The node automatically switches between continuous streaming and
 * triggered single-shot modes based on sync input connection status.
 *
 * **Port Configuration:**
 * - **Inputs:**
 *   - Port 0: SyncData (optional) - Triggers single frame capture when connected
 * - **Outputs:**
 *   - Port 0: CVImageData - Captured frame
 *   - Port 1: InformationData - Camera status (FPS, resolution, connection state)
 *
 * **Embedded Widget Controls:**
 * - Camera ID selection dropdown (0, 1, 2, ...)
 * - Start/Stop buttons
 * - Connection status indicator (red=disconnected, green=connected)
 *
 * **Operating Mode Logic:**
 * ```
 * if (sync_input_connected) {
 *     // Single-Shot Mode
 *     wait_for_sync_signal();
 *     capture_one_frame();
 * } else {
 *     // Continuous Mode
 *     while (running) {
 *         capture_frame();
 *         emit_at_fps();
 *     }
 * }
 * ```
 *
 * **Common Use Cases:**
 *
 * 1. **Live Preview:**
 *    ```
 *    CVCamera → CVImageDisplay
 *    ```
 *
 * 2. **Real-Time Object Detection:**
 *    ```
 *    CVCamera → FaceDetection → DrawContour → CVImageDisplay
 *    CVCamera → HoughCircleTransform → DrawContour → CVImageDisplay
 *    ```
 *
 * 3. **Fixed-Rate Processing (avoid camera FPS variations):**
 *    ```
 *    Timer(10Hz) → CVCamera → Threshold → FindContour → Analysis
 *    ```
 *
 * 4. **Multi-Camera Synchronization:**
 *    ```
 *    Timer → CombineSync ┬→ CVCamera(ID=0) → Process0
 *                         ├→ CVCamera(ID=1) → Process1
 *                         └→ CVCamera(ID=2) → Process2
 *    ```
 *
 * 5. **Conditional Capture (only when object detected):**
 *    ```
 *    CVCamera(continuous) → MotionDetect → [if motion] → Trigger → CVCamera(single-shot) → SaveImage
 *    ```
 *
 * 6. **Barcode Scanner:**
 *    ```
 *    CVCamera → Threshold → FindContour → BarcodeDecoder → Display
 *    ```
 *
 * **Camera Parameter Configuration:**
 * All parameters accessible via node properties panel:
 * - Resolution (Width × Height)
 * - Frame Rate (FPS)
 * - Codec (MJPG, YUYV, H264)
 * - Exposure (Auto/Manual with μs value)
 * - Gain/ISO
 * - Brightness
 * - White Balance (Auto/Manual)
 *
 * **Performance Characteristics:**
 * - Threaded capture: No UI blocking, maintains FPS under load
 * - Measured FPS available in InformationData output
 * - Single-shot latency: ~(1/FPS) seconds from trigger to output
 * - Continuous mode latency: ~(2-3)/FPS seconds (grab + retrieve + processing)
 *
 * **Camera Compatibility:**
 * - USB webcams (UVC protocol)
 * - Built-in laptop cameras
 * - Industrial cameras (V4L2 on Linux, DirectShow on Windows)
 * - Query available devices with `v4l2-ctl --list-devices` (Linux)
 *
 * **Troubleshooting:**
 * - **Red status indicator**: Check camera connection, permissions, device ID
 * - **Low FPS**: Reduce resolution, change codec to MJPG, check CPU load
 * - **Parameter changes ignored**: Some cameras don't support all CAP_PROP_* settings
 * - **Single-shot not triggering**: Verify sync input is connected and firing
 *
 * **Design Rationale:**
 * - Thread separation prevents frame drops during heavy UI/processing load
 * - Automatic mode switching simplifies pipeline design (no manual mode toggle)
 * - Semaphore-based single-shot ensures precise timing for triggered applications
 * - Shared pointer for image data minimizes copying overhead
 *
 * @note Camera parameters must match device capabilities (use v4l2-ctl or camera specs).
 * @see CVVDOLoaderModel for video file playback
 * @see CVCameraCalibrationModel for intrinsic/extrinsic calibration
 * @see cv::VideoCapture for underlying capture interface
 */
class CVCameraModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVCameraModel();

    virtual
    ~CVCameraModel() override
    {
        if( mpCVCameraThread )
            delete mpCVCameraThread;
    }

    QJsonObject
    save() const override;

    void
    load(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    void
    late_constructor() override;

    static const QString _category;

    static const QString _model_name;

    void setSelected(bool selected) override;

private Q_SLOTS:
    /**
     * @brief Handles new frame ready signal from capture thread.
     * Triggers dataUpdated() to propagate frame through pipeline.
     */
    void
    received_image();

    /**
     * @brief Updates UI and information output when camera connection changes.
     * @param status true if connected, false if disconnected
     */
    void
    camera_status_changed( bool );

    /**
     * @brief Processes embedded widget button clicks (Start/Stop, camera ID change).
     * @param button Button identifier (0=Start, 1=Stop, etc.)
     */
    void
    em_button_clicked( int button );

    void
    enable_changed( bool ) override;

    /**
     * @brief Switches to single-shot mode when sync input is connected.
     * Triggered automatically by NodeEditor framework.
     */
    void
    inputConnectionCreated(QtNodes::ConnectionId const&) override { mpCVCameraThread->set_single_shot_mode( true ); };

    /**
     * @brief Switches to continuous mode when sync input is disconnected.
     * Triggered automatically by NodeEditor framework.
     */
    void
    inputConnectionDeleted(QtNodes::ConnectionId const&) override { mpCVCameraThread->set_single_shot_mode( false ); };

private:
    int miBrightness {-10};         ///< Current brightness setting
    int miGain {70};                ///< Current gain/ISO setting
    int miExposure {8000};          ///< Current exposure time in μs
    bool mbAutoExposure {false};    ///< Auto exposure enabled
    bool mbAutoWB {false};          ///< Auto white balance enabled

    CVCameraProperty mCameraProperty;                   ///< Camera ID and status
    CVCameraEmbeddedWidget * mpEmbeddedWidget;          ///< UI controls

    CVCameraThread * mpCVCameraThread { nullptr };      ///< Background capture thread

    std::shared_ptr< SyncData > mpSyncInData { nullptr };       ///< Trigger input (single-shot mode)
    std::shared_ptr< CVImageData > mpCVImageData;               ///< Captured frame output
    std::shared_ptr< InformationData > mpInformationData;       ///< Camera status output
};

