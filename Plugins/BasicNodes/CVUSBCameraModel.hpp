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
 * @file CVUSBCameraModel.hpp
 * @brief Camera capture node for real-time image acquisition from USB/V4L2 cameras.
 *
 * This file defines the CVUSBCameraModel node which provides live camera feed integration
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
 * CVUSBCamera → CVImageDisplay        (Live preview)
 * CVUSBCamera → FaceDetection → Draw  (Real-time detection)
 * CVUSBCamera → Threshold → FindContour (Object tracking)
 * Timer → CVUSBCamera → Processing     (Triggered capture at fixed rate)
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
#include <QtCore/QTimer>

#include "PBAsyncDataModel.hpp"

#include "SyncData.hpp"
#include "CVImageData.hpp"
#include "CVImagePool.hpp"
#include "InformationData.hpp"

#include "CVUSBCameraEmbeddedWidget.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;
using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;
using CVDevLibrary::FrameMetadata;

// Forward declaration
class CVUSBCameraModel;

/**
 * @struct CVUSBCameraParameters
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
typedef struct CVUSBCameraParameters{
    int miFourCC {cv::VideoWriter::fourcc('M','J','P','G')}; ///< Codec (MJPG, YUYV, etc.) - cv::VideoWriter::fourcc('Y','U','Y','V')
    int miFps {25};                 ///< Frames per second (typical: 15, 25, 30, 60)
    int miWidth {2592};             ///< Frame width in pixels (must match camera capability)
    int miHeight {1944};            ///< Frame height in pixels (must match camera capability)
    int miAutoWB {1};               ///< Auto white balance (1=auto, 0=manual)
    int miBrightness {-10};         ///< Brightness adjustment (-64 to 64, camera-dependent)
    int miGain {70};                ///< Gain/ISO (0-100, camera-dependent)
    int miAutoExposure {1};         ///< Auto exposure (1=auto, 0=manual)
    int miExposure {2000};          ///< Exposure time in μs when AutoExposure=0 (camera-dependent range)
    int miAutoFocus {1};            ///< Auto focus (1=auto, 0=manual) - not all cameras support this
} CVUSBCameraParameters;

/**
 * @class CVUSBCameraWorker
 * @brief Async worker (QObject) for USB camera capture inside PBAsyncDataModel worker thread.
 */
class CVUSBCameraWorker : public QObject
{
    Q_OBJECT
public:
    explicit CVUSBCameraWorker(QObject *parent = nullptr);

public Q_SLOTS:
    void setCameraId(int cameraId);
    void setParams(CVUSBCameraParameters params);
    void setSingleShotMode(bool enabled);
    void fireSingleShot();

Q_SIGNALS:
    void frameCaptured(cv::Mat frame);
    void cameraReady(bool status);
    void fpsUpdated(double fps);
    void capabilitiesDetected(bool autoFocusSupported,
                              bool autoExposureSupported,
                              bool autoWbSupported,
                              bool brightnessSupported,
                              bool gainSupported,
                              bool exposureSupported);

private Q_SLOTS:
    void captureFrame();

private:
    void checkCamera();

    CVUSBCameraModel *mpModel{nullptr};
    QTimer *mpTimer{nullptr};
    int miCameraID{-1};
    bool mbConnected{false};
    bool mbSingleShotMode{false};
    unsigned long miDelayTime{10};
    double mdFPS{0};
    CVUSBCameraParameters mUSBCameraParams;
    cv::VideoCapture mCVVideoCapture;
};

/**
 * @class CVUSBCameraModel
 * @brief Live camera capture node with parameter control and dual operating modes.
 *
 * CVUSBCameraModel provides real-time image acquisition from USB, built-in, or V4L2 cameras.
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
 *    CVUSBCamera → CVImageDisplay
 *    ```
 *
 * 2. **Real-Time Object Detection:**
 *    ```
 *    CVUSBCamera → FaceDetection → DrawContour → CVImageDisplay
 *    CVUSBCamera → HoughCircleTransform → DrawContour → CVImageDisplay
 *    ```
 *
 * 3. **Fixed-Rate Processing (avoid camera FPS variations):**
 *    ```
 *    Timer(10Hz) → CVUSBCamera → Threshold → FindContour → Analysis
 *    ```
 *
 * 4. **Multi-Camera Synchronization:**
 *    ```
 *    Timer → CombineSync ┬→ CVUSBCamera(ID=0) → Process0
 *                         ├→ CVUSBCamera(ID=1) → Process1
 *                         └→ CVUSBCamera(ID=2) → Process2
 *    ```
 *
 * 5. **Conditional Capture (only when object detected):**
 *    ```
 *    CVUSBCamera(continuous) → MotionDetect → [if motion] → Trigger → CVUSBCamera(single-shot) → SaveImage
 *    ```
 *
 * 6. **Barcode Scanner:**
 *    ```
 *    CVUSBCamera → Threshold → FindContour → BarcodeDecoder → Display
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
class CVUSBCameraModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVUSBCameraModel();

    ~CVUSBCameraModel() override;

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

protected:
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;

private Q_SLOTS:
    /**
     * @brief Handles new frame ready signal from capture thread.
     * Triggers dataUpdated() to propagate frame through pipeline.
     */
    /**
     * @brief Process captured frame from camera thread.
     * @param frame Captured frame to process with pool
     */
    void
    process_captured_frame( cv::Mat frame );

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
     * @brief Refresh available camera IDs for embedded widget and property browser.
     */
    void
    refresh_camera_list();

    /**
     * @brief Refresh available resolutions and supported formats for the current camera and update properties.
     */
    void
    refresh_camera_capabilities();

    /**
     * @brief (Deprecated, use refresh_camera_capabilities) Refresh available resolutions for the current camera.
     */
    void
    refresh_resolutions();

    /**
     * @brief Updates Property Browser editability based on camera capability detection.
     */
    void
    update_camera_capabilities( bool autoFocusSupported,
                                bool autoExposureSupported,
                                bool autoWbSupported,
                                bool brightnessSupported,
                                bool gainSupported,
                                bool exposureSupported );

    /**
     * @brief Switches to single-shot mode when sync input is connected.
     * Triggered automatically by NodeEditor framework.
     */
    void
    inputConnectionCreated(QtNodes::ConnectionId const&) override;

    /**
     * @brief Switches to continuous mode when sync input is disconnected.
     * Triggered automatically by NodeEditor framework.
     */
    void
    inputConnectionDeleted(QtNodes::ConnectionId const&) override;

    QPixmap
    minPixmap() const override { return mMinPixmap; }
private:
private:
    int miBrightness {-10};         ///< Current brightness setting
    int miGain {70};                ///< Current gain/ISO setting
    int miExposure {8000};          ///< Current exposure time in μs
    bool mbAutoExposure {false};    ///< Auto exposure enabled
    bool mbAutoWB {false};          ///< Auto white balance enabled
    bool mbAutoFocus {true};        ///< Auto focus enabled
    QString mCurrentResolution;     ///< Cached resolution string (e.g., "1920x1080")
    CVUSBCameraParameters mUSBCameraParams; ///< Cached camera parameters for resolution probing
    bool mAutoRefreshAllowed {true}; ///< Allow automatic camera/resolution enumeration

    struct CVUSBCameraCapabilities
    {
        bool mbAutoFocusSupported {true};
        bool mbAutoExposureSupported {true};
        bool mbAutoWbSupported {true};
        bool mbBrightnessSupported {true};
        bool mbGainSupported {true};
        bool mbExposureSupported {true};
    } mCVUSBCameraCapabilities;

    CVUSBCameraProperty mCVUSBCameraProperty;                   ///< Camera ID and status
    CVUSBCameraEmbeddedWidget * mpEmbeddedWidget;          ///< UI controls

    CVUSBCameraWorker * mpCameraWorker { nullptr };      ///< Async capture worker (in PBAsync thread)

    std::shared_ptr< SyncData > mpSyncInData { nullptr };       ///< Trigger input (single-shot mode)
    std::shared_ptr< CVImageData > mpCVImageData;               ///< Captured frame output
    std::shared_ptr< InformationData > mpInformationData;       ///< Camera status output
    double mdLastFps{0.0};

    QPixmap mMinPixmap;
    int miFrameMatType{CV_8UC3};

    template <typename T>
    void set_property_read_only(const QString &id, bool readOnly);

    QStringList enumerate_camera_ids(int maxCandidates = 4) const;
    QStringList enumerate_resolutions_for_camera(int cameraId) const;
    QStringList enumerate_formats_for_camera(int cameraId) const;
    static QString make_resolution_string(int width, int height);
    static bool parse_resolution_string(const QString &text, int &width, int &height);
};

