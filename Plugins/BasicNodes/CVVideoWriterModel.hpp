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
 * @file CVVideoWriterModel.hpp
 * @brief Model for video file recording with threading and file segmentation.
 *
 * This file defines the CVVideoWriterModel and VideoWriterThread classes for recording
 * video streams to files with support for automatic file segmentation, configurable
 * frame rates, and threaded I/O to prevent pipeline blocking. Essential for video
 * capture, processing result recording, and surveillance applications.
 */

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QQueue>
#include <QtWidgets/QPushButton>

#include "PBNodeDelegateModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"
#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class VideoWriterThread
 * @brief Worker thread for asynchronous video file writing.
 *
 * This QThread subclass handles video encoding and file I/O operations in a separate
 * thread, preventing disk writes from blocking the main processing pipeline. It manages
 * cv::VideoWriter lifecycle, frame queueing, and automatic file segmentation.
 *
 * **Key Features:**
 * - Asynchronous queue-based frame writing
 * - Automatic file segmentation (split into multiple files after N frames)
 * - Configurable FPS and output filename
 * - Thread-safe frame enqueueing
 * - Error signaling to main thread
 *
 * **File Segmentation:**
 * When max_frame_per_video is reached:
 * - Current file closed
 * - New file opened with incremented counter
 * - Example: video_0000.avi, video_0001.avi, video_0002.avi
 *
 * @see CVVideoWriterModel
 * @see cv::VideoWriter
 */
class VideoWriterThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a VideoWriterThread.
     * @param parent Parent QObject (typically the model).
     */
    explicit
    VideoWriterThread( QObject *parent = nullptr );

    /**
     * @brief Destructor.
     *
     * Stops recording, closes video writer, and waits for thread completion.
     */
    ~VideoWriterThread() override;

    /**
     * @brief Adds a frame to the writing queue.
     * @param Image frame to write (copied into queue).
     *
     * Thread-safe method to enqueue frames. Frames are written in order received.
     */
    void
    add_image( const cv::Mat & );

    /**
     * @brief Starts video recording with specified parameters.
     * @param filename Output filename (with or without counter suffix).
     * @param max_frame_per_video Maximum frames per file before segmentation.
     * @param fps Frame rate for video encoding.
     *
     * Initializes recording parameters. Actual file creation happens when first
     * frame is written (to determine frame size and format).
     */
    void
    start_writer( QString filename, int max_frame_per_video, int fps );

    /**
     * @brief Stops video recording.
     *
     * Closes current video file and stops accepting new frames.
     */
    void
    stop_writer();

Q_SIGNALS:
    /**
     * @brief Signal emitted when video writer encounters an error.
     * @param Error code.
     *
     * Allows worker thread to report errors (file creation failure, codec issues)
     * to main thread for user notification.
     */
    void
    video_writer_error_signal(int);

protected:
    /**
     * @brief Thread execution loop.
     *
     * Continuously processes frame queue, writing each frame to video file.
     * Handles file segmentation when frame limit reached.
     */
    void
    run() override;

private:
    /**
     * @brief Opens a new video writer with parameters from first frame.
     * @param image First frame (used to determine size, channels).
     * @return true if successful, false on error.
     *
     * Creates cv::VideoWriter with appropriate codec, FPS, and frame size.
     */
    bool open_writer( const cv::Mat & image );

    QSemaphore mWaitingSemaphore;          ///< Synchronization semaphore

    QString msFilename;                    ///< Base output filename
    int miFPS {10};                        ///< Frame rate for encoding
    int miRecordingStatus {0};             ///< Recording state (0=stopped, 1=recording)
    cv::Size mSize;                        ///< Frame size
    int miChannels {0};                    ///< Number of color channels

    QQueue< cv::Mat > mqCVImage;           ///< Frame queue
    cv::VideoWriter mVideoWriter;          ///< OpenCV video writer
    bool mbWriterReady {false};            ///< Writer initialization status
    bool mbAbort {false};                  ///< Abort flag for graceful shutdown

    int miFrameCounter {0};                ///< Current file frame count
    int miFilenameCounter {0};             ///< File segmentation counter
    int miFramePerVideo {1000};            ///< Max frames before creating new file
};

/**
 * @class CVVideoWriterModel
 * @brief Node model for recording video streams to files.
 *
 * This model provides comprehensive video recording capabilities with threaded I/O,
 * automatic file segmentation, configurable frame rates, and start/stop control via
 * embedded button. Supports multiple video codecs and formats through OpenCV.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Frame stream to record
 *
 * **Output Ports:**
 * None (sink node)
 *
 * **Key Features:**
 * - Start/Stop button in embedded widget
 * - Threaded writing (non-blocking)
 * - Automatic file segmentation (prevents huge files)
 * - Configurable FPS (independent of input rate)
 * - Multiple codec support (via OpenCV)
 *
 * **Recording Control:**
 * - **Start Button:** Begins recording to file
 * - **Stop Button:** Ends recording, closes current file
 * - Button state toggles (only one active at a time)
 *
 * **File Segmentation:**
 * Automatically splits recording into multiple files:
 * - Prevents single huge video files
 * - Easier to manage and playback
 * - Example: output_0000.avi (1000 frames), output_0001.avi (1000 frames), etc.
 * - Counter increments automatically
 *
 * **Frame Rate Handling:**
 * - Output FPS set via fps property
 * - Independent of input frame rate
 * - If input faster than output: frames may be dropped
 * - If input slower than output: video may play back faster than real-time
 *
 * **Codec Support:**
 * Depends on OpenCV build and system codecs:
 * - **MJPEG:** Universal, good quality, large files
 * - **H264/MPEG4:** Compressed, smaller files, may require codec
 * - **XVID:** Common, good compression
 * - Platform-specific codecs (varies by OS)
 *
 * **Properties (Configurable):**
 * - **output_filename:** Base filename (e.g., "recording.avi")
 * - **fps:** Output frame rate (default: 10)
 * - **frame_per_video:** Frames per file segment (default: 1000)
 *
 * **Use Cases:**
 * - Record camera stream to disk
 * - Save processed video for later review
 * - Create video datasets
 * - Surveillance recording
 * - Time-lapse video creation
 * - Processing result archival
 *
 * **Example Workflows:**
 * @code
 * // Simple recording
 * [Camera] -> [VideoWriter: Start] -> recording_0000.avi
 * 
 * // Record processed output
 * [Camera] -> [ProcessingNode] -> [VideoWriter] -> results.avi
 * 
 * // Conditional recording (save only interesting frames)
 * [Camera] -> [QualityFilter] -> [SyncGate] -> [VideoWriter]
 * @endcode
 *
 * **File Naming:**
 * - Base: "output.avi"
 * - Files: "output_0000.avi", "output_0001.avi", ...
 * - Counter added before extension
 * - Increments automatically on segmentation
 *
 * **Performance Considerations:**
 * - Threading prevents blocking, but queue can grow with fast input
 * - Disk write speed limits practical frame rate
 * - Compression codec affects CPU usage
 * - Consider SSD for high-speed recording (>60 fps)
 * - MJPEG faster to encode but larger files
 * - H264 slower encoding but better compression
 *
 * **Enable/Disable Behavior:**
 * - Disabling node stops recording (same as Stop button)
 * - Enabling node does NOT auto-start (user must click Start)
 *
 * **Error Handling:**
 * - Thread emits video_writer_error_signal() on failure
 * - Common errors: codec not available, disk full, invalid filename
 * - User notified via slot
 *
 * **Best Practices:**
 * 1. Set FPS to match input rate for real-time recording
 * 2. Use frame_per_video to limit file sizes (e.g., 1000-3000 frames)
 * 3. Ensure sufficient disk space before long recordings
 * 4. Test codec availability before deployment
 * 5. Stop recording before closing application
 * 6. Use absolute paths for output files
 *
 * @see VideoWriterThread
 * @see cv::VideoWriter
 * @see SaveImageModel (for single frames)
 */
class CVVideoWriterModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVVideoWriterModel.
     *
     * Initializes with default parameters and creates the Start/Stop button widget.
     */
    CVVideoWriterModel();

    /**
     * @brief Destructor.
     *
     * Stops recording and deletes the writer thread.
     */
    virtual
    ~CVVideoWriterModel() override
    {
        if( mpVideoWriterThread )
            delete mpVideoWriterThread;
    }

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing filename, FPS, and segmentation settings.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved parameters.
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 1 for input (frame stream), 0 for output (sink node).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return CVImageData for input port.
     */
    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Sets input frame data and enqueues for writing if recording.
     * @param nodeData Input CVImageData (video frame).
     * @param Port index (0).
     *
     * If recording is active, adds frame to writer thread's queue.
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    /**
     * @brief Returns the embedded Start/Stop button widget.
     * @return Pointer to QPushButton.
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    /**
     * @brief Sets a model property.
     * @param Property name ("output_filename", "fps", "frame_per_video").
     * @param QVariant value.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Late constructor for thread initialization.
     *
     * Creates and connects the video writer thread.
     */
    void
    late_constructor() override;

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private Q_SLOTS:

    /**
     * @brief Slot for Start/Stop button clicks.
     * @param Button checked state (true=Start, false=Stop).
     *
     * Starts or stops video recording based on button state.
     */
    void
    em_button_clicked(bool);

    /**
     * @brief Slot for node enable/disable state changes.
     * @param Enabled state.
     *
     * Stops recording when node is disabled.
     */
    void
    enable_changed(bool) override;

    /**
     * @brief Slot to handle errors from writer thread.
     * @param Error code.
     *
     * Called when video writer encounters an error.
     */
    void
    video_writer_error_occured(int);

private:
    QPushButton * mpEmbeddedWidget;                ///< Start/Stop button widget
    bool mbRecording { false };                    ///< Current recording state

    VideoWriterThread * mpVideoWriterThread { nullptr };  ///< Worker thread for writing

    QString msOutput_Filename;                     ///< Output filename template
    int miFPS {10};                                ///< Output video frame rate
    int miFramePerVideo {1000};                    ///< Frames per file segment

    /**
     * @brief Processes incoming frame data.
     * @param in Input CVImageData.
     *
     * If recording, enqueues frame to writer thread.
     */
    void processData(const std::shared_ptr< CVImageData > & in);

    QPixmap _minPixmap;
};
