//Copyright © 2020 - 2026, NECTEC, all rights reserved

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
 * @file CVVideoLoaderModel.hpp
 * @brief Video file loader node with frame-by-frame playback and seek control.
 *
 * Provides CVVideoLoaderThread (dedicated decode thread) and CVVideoLoaderModel
 * (node model with embedded playback widget) for video processing pipelines.
 *
 * **Architecture:**
 * - CVVideoLoaderThread runs a cv::VideoCapture loop in its own QThread
 * - Frames are decoded and delivered via frame_decoded() signal (QueuedConnection)
 * - CVVideoLoaderModel adopts frames into CVImagePool (zero-copy sharing) and
 *   emits them on output port 0
 * - Sync signal input (port 1) can trigger single-frame advances from upstream
 *
 * **Output Ports:**
 * - Port 0: CVImageData — current decoded frame
 * - Port 1: CVSizeData  — video frame dimensions
 * - Port 2: InformationData — OpenCV image type string (e.g. "CV_8UC3")
 *
 * **Input Ports:**
 * - Port 0: SyncData — optional trigger for frame advance (when sync mode enabled)
 *
 * **Playback Controls (via embedded widget):**
 * - Play/Pause, Step Forward/Backward, Seek by slider or frame number, Open file
 *
 * **Frame Pool:**
 * Pool is lazily created on first frame and re-created if video dimensions change.
 * Protected by QMutex; `mShuttingDown` atomic flag guards destructor races.
 *
 * @see CVVideoLoaderThread, CVVideoLoaderEmbeddedWidget, CVImagePool
 */

#pragma once

#include <memory>
#include <atomic>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QElapsedTimer>
#include "PBNodeDelegateModel.hpp"

#include "CVImagePool.hpp"
#include "CVImageData.hpp"
#include <opencv2/videoio.hpp>
#include "InformationData.hpp"
#include "CVSizeData.hpp"
#include "CVVideoLoaderEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;
using CVDevLibrary::FrameSharingMode;

class CVVideoLoaderModel;

/**
 * @class CVVideoLoaderThread
 * @brief Dedicated decode thread for video file playback.
 *
 * Manages cv::VideoCapture lifecycle and provides frame-level playback control
 * (play, pause, step, seek, loop) without blocking the main thread.
 *
 * **Synchronization:**
 * - `mFrameRequestSemaphore`: released by `advance_frame()` to request one frame
 * - `mSeekSemaphore`: released by `seek_to_frame()` to trigger a seek
 * - `request_abort()`: sets mbAbort and releases both semaphores for clean exit
 *
 * **Playback Loop (run()):**
 * 1. Block on mFrameRequestSemaphore
 * 2. If mbAbort → exit
 * 3. If seek pending → seek cv::VideoCapture to miSeekTarget
 * 4. Call decode_next_frame() → emit frame_decoded(cv::Mat)
 * 5. In playback mode, schedule next frame after miFlipPeriodInMillisecond ms
 */
class CVVideoLoaderThread : public QThread
{
    Q_OBJECT
public:
    explicit
    CVVideoLoaderThread(QObject *parent, CVVideoLoaderModel *model);

    /// @brief Destructor; calls close_video() and waits for thread exit.
    ~CVVideoLoaderThread() override {};

    /// @}

    /// @name Video Control
    /// @{

    /**
     * @brief Opens a video file for decoding.
     * @param filename Path to the video file.
     * @return true if cv::VideoCapture opened successfully; false on error.
     */
    bool open_video(const QString& filename);

    /// @brief Closes the current video and resets decode state.
    void close_video();

    /**
     * @brief Sets the inter-frame delay in milliseconds for continuous playback.
     * @param ms Delay between frames (e.g. 33 ms ≈ 30 fps).
     */
    void set_flip_period(int ms) { miFlipPeriodInMillisecond = ms; }

    /**
     * @brief Enables or disables looping when end-of-video is reached.
     * @param loop true = loop back to frame 0; false = pause at end.
     */
    void set_loop(bool loop) { mbLoop = loop; }

    /// @brief Starts continuous frame decode loop.
    void start_playback();

    /// @brief Pauses continuous playback (no more frames decoded until advanced).
    void stop_playback();

    /**
     * @brief Signals the run() loop to terminate and unblocks any waiting semaphores.
     *
     * Must be called before QThread::wait() in the destructor to ensure clean exit.
     */
    void request_abort();

    /**
     * @brief Seeks to a specific frame number on next decode cycle.
     * @param frame_no Target frame index (0-based).
     */
    void seek_to_frame(int frame_no);

    /// @brief Requests decoding of the next single frame.
    void advance_frame();

    /// @}

    /// @name State Queries
    /// @{

    /// @brief Returns the index of the last decoded frame.
    int get_current_frame() const { return miCurrentFrame; }

    /// @brief Returns true if a video file is currently open.
    bool is_opened() const { return mbVideoOpened; }

    /// @}

Q_SIGNALS:
    /// @name Thread Signals
    /// @{

    /**
     * @brief Emitted after each frame is decoded, before pool adoption.
     * @param frame Raw decoded cv::Mat (BGR or grayscale, 8U).
     */
    void frame_decoded(cv::Mat frame);

    /**
     * @brief Emitted once when the video file is successfully opened.
     * @param max_frames Total frame count.
     * @param size Frame width × height.
     * @param format OpenCV mat type string (e.g. "CV_8UC3").
     */
    void video_opened(int max_frames, cv::Size size, QString format);

    /// @brief Emitted when playback reaches the last frame.
    void video_ended();

    /// @}

protected:
    /// @brief Main decode/playback loop; blocks on mFrameRequestSemaphore.
    void run() override;

private:
    /// @brief Reads one frame from cv::VideoCapture and emits frame_decoded().
    void decode_next_frame();

    QSemaphore mFrameRequestSemaphore; ///< Released per frame-advance request
    QSemaphore mSeekSemaphore;         ///< Released when a seek is pending

    bool mbAbort{false};               ///< Signals run() to exit
    bool mbPlayback{false};            ///< Continuous play mode active
    bool mbLoop{true};                 ///< Loop on end-of-video
    bool mbVideoOpened{false};         ///< cv::VideoCapture is open

    int miFlipPeriodInMillisecond{100}; ///< Inter-frame delay for playback
    int miMaxNoFrames{0};              ///< Total frame count
    int miCurrentFrame{0};             ///< Last decoded frame index
    int miSeekTarget{-1};              ///< Pending seek target (-1 = none)

    cv::Size mcvVideoSize{320, 240};    ///< Frame dimensions
    QString msImageFormat{"CV_8UC3"};  ///< OpenCV mat type string

    cv::VideoCapture mcvVideoCapture;  ///< OpenCV video reader
    CVVideoLoaderModel *mpModel{nullptr}; ///< Back-pointer for frame pool access
};

/**
 * @class CVVideoLoaderModel
 * @brief Node model for loading and playing back video files frame-by-frame.
 *
 * Wraps CVVideoLoaderThread with property management, CVImagePool integration,
 * and an embedded playback widget. Connects thread signals to main-thread slots
 * via Qt::QueuedConnection for thread safety.
 *
 * **Node Metadata:**
 * - Category: Input
 * - Display Name: CV Video Loader
 *
 * **Output Ports:**
 * - Port 0: CVImageData   — current decoded video frame
 * - Port 1: CVSizeData    — video frame dimensions (width × height)
 * - Port 2: InformationData — OpenCV type string (e.g. "CV_8UC3")
 *
 * **Input Ports:**
 * - Port 0: SyncData — optional trigger for single-frame advance
 *   (enabled via `use_sync_signal` property)
 *
 * **Properties:**
 * - `video_filename`: path to video file
 * - `flip_period`: inter-frame delay in ms (controls effective FPS)
 * - `loop`: loop on end-of-video
 * - `use_sync_signal`: when true, advance one frame per sync pulse
 * - `pool_size`: CVImagePool depth (1-128)
 *
 * **Frame Pool:**
 * Lazily allocated on first decoded frame via ensure_frame_pool().
 * Re-created when video dimensions change. Protected by QMutex.
 * `mShuttingDown` atomic flag prevents pool access after destructor begins.
 *
 * **Save/Load:**
 * Serialises all properties plus msVideoFilename so the flow auto-opens
 * the file and restores playback position on load.
 */
class CVVideoLoaderModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    friend class CVVideoLoaderThread;

    /// @name Construction & Destruction
    /// @{

    /**
     * @brief Constructs node with default parameters and creates embedded widget.
     */
    CVVideoLoaderModel();

    /**
     * @brief Destroys the node; requests thread abort and waits for clean exit.
     *
     * Sets mShuttingDown, calls request_abort(), then QThread::wait() to prevent
     * use-after-free on frame pool or model members.
     */
    ~CVVideoLoaderModel() override;

    /// @}

    /// @name Persistence
    /// @{

    /**
     * @brief Saves node state to JSON (filename, properties, pool settings).
     * @return JSON object for .flow file serialization.
     */
    QJsonObject save() const override;

    /**
     * @brief Restores node state from JSON and reloads video file if present.
     * @param p JSON object from .flow file.
     */
    void load(QJsonObject const &p) override;

    /// @}

    /// @name Port Interface
    /// @{

    /**
     * @brief Returns port counts.
     * @param portType In (1) or Out (3).
     * @return Port count for the given direction.
     */
    unsigned int nPorts(PortType portType) const override;

    /**
     * @brief Returns data type for the given port.
     *
     * - Out 0: CVImageData, Out 1: CVSizeData, Out 2: InformationData
     * - In  0: SyncData
     *
     * @param portType Port direction.
     * @param portIndex Port index.
     * @return Appropriate NodeDataType.
     */
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns current output data for the given port.
     * @param port Output port index (0=image, 1=size, 2=format string).
     * @return Shared pointer to output NodeData, or nullptr if not available.
     */
    std::shared_ptr<NodeData> outData(PortIndex port) override;

    /**
     * @brief Receives sync input to advance one frame (when sync mode enabled).
     * @param nodeData Incoming SyncData (presence triggers advance).
     * @param portIndex Input port index (0).
     */
    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;

    /// @}

    /// @name UI Integration
    /// @{

    /// @brief Returns the embedded playback control widget.
    QWidget * embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Updates a node property by ID.
     * @param id Property identifier (e.g. "flip_period", "loop", "pool_size").
     * @param value New property value.
     */
    void setModelProperty( QString & id, const QVariant & value ) override;

    /// @brief Returns the minimized node icon (CVVideoLoader.png).
    QPixmap
    minPixmap() const override { return _minPixmap; }

    /// @}

    /// @name Deferred Initialization
    /// @{

    /**
     * @brief Creates CVVideoLoaderThread, connects signals, and opens saved video file.
     *
     * Called by PBDataFlowGraphModel after the node is placed in the scene or
     * restored from a .flow file. Thread is not started here; it starts on
     * the first play/advance request.
     */
    void late_constructor() override;

    /// @}

    /// @name Node Metadata
    /// @{

    /// Node category for UI organization ("Input").
    static const QString _category;

    /// Display name in node menu ("CV Video Loader").
    static const QString _model_name;

    /// @}

private Q_SLOTS:
    /// @brief Re-emits output ports when the node enable state changes.
    void enable_changed( bool enabled ) override;

    /**
     * @brief Dispatches widget button events to thread control methods.
     * @param button Button code (0=file, 1=play/pause, 2=forward, 3=backward).
     */
    void em_button_clicked( int button );

    /**
     * @brief Seeks the video thread to the requested frame.
     * @param no_frame Target frame index from slider or spinbox.
     */
    void no_frame_changed( int no_frame );

    /**
     * @brief Updates the slider and spinbox to reflect actual playback position.
     * @param frame_no Current frame index reported by the thread.
     */
    void update_frame_ui(int frame_no);

    /**
     * @brief Initialises UI controls after a video file is opened.
     * @param max_frames Total frame count (sets slider maximum).
     * @param size Frame dimensions (emitted on port 1).
     * @param format OpenCV type string (emitted on port 2).
     */
    void video_file_opened(int max_frames, cv::Size size, QString format);

    /// @brief Handles end-of-video: pauses UI and optionally loops.
    void on_video_ended();

    /// @brief Re-opens video and restores seek when a connection is created.
    void inputConnectionCreated(QtNodes::ConnectionId const& conn) override;

    /// @brief Clears frame output when sync connection is removed.
    void inputConnectionDeleted(QtNodes::ConnectionId const& conn) override;

private:
    /**
     * @brief Opens the file chooser dialog and loads the selected video.
     * @param filename (in/out) Updated with the chosen file path.
     */
    void set_video_filename(QString & filename);

    /**
     * @brief Adopts a decoded cv::Mat into the frame pool and emits port 0.
     * @param frame Raw decoded frame from CVVideoLoaderThread.
     */
    void process_decoded_frame(cv::Mat frame);

    /**
     * @brief Allocates or re-allocates the CVImagePool when frame geometry changes.
     * @param width  Frame width in pixels.
     * @param height Frame height in pixels.
     * @param type   OpenCV mat type (e.g. CV_8UC3).
     */
    void ensure_frame_pool(int width, int height, int type);

    /// @brief Releases the current frame pool and resets pool state.
    void reset_frame_pool();

    /// @brief Returns true if destructor teardown is in progress.
    bool isShuttingDown() const { return mShuttingDown.load(std::memory_order_acquire); }

    QString msVideoFilename {""};
    int miFlipPeriodInMillisecond{100};   ///< Inter-frame delay (ms)
    bool mbLoop {true};                   ///< Loop on end-of-video
    QString msImage_Format{ "CV_8UC3" }; ///< Cached frame type string
    cv::Size mcvImage_Size{ cv::Size(320,240) }; ///< Cached frame dimensions
    int miMaxNoFrames{ 0 };               ///< Total frame count

    CVVideoLoaderEmbeddedWidget * mpEmbeddedWidget;
    CVVideoLoaderThread * mpVideoLoaderThread{nullptr};

    std::shared_ptr< CVImageData > mpCVImageData; ///< Current output frame

    bool mbUseSyncSignal{false};          ///< Advance frames via sync input

    int miPoolSize{CVImagePool::DefaultPoolSize};
    FrameSharingMode meSharingMode{FrameSharingMode::PoolMode};
    std::shared_ptr<CVImagePool> mpFramePool;
    int miPoolFrameWidth{0};
    int miPoolFrameHeight{0};
    int miActivePoolSize{0};
    QMutex mFramePoolMutex;
    int miFrameMatType{CV_8UC3};
    std::atomic<bool> mShuttingDown{false}; ///< Guards destructor/thread teardown race
    QPixmap _minPixmap;
};
