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
 * @file CVVideoLoaderModel.hpp
 * @brief Provides video file loading and playback functionality with frame-by-frame control.
 *
 * This file implements a node that loads video files and outputs frames sequentially,
 * enabling video processing pipelines. It uses OpenCV's cv::VideoCapture for reading
 * various video formats (MP4, AVI, MOV, MKV, etc.) and provides playback controls
 * through an embedded widget.
 *
 * @see CVVideoLoaderModel, CVVideoLoaderEmbeddedWidget, cv::VideoCapture
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

class CVVideoLoaderThread : public QThread
{
    Q_OBJECT
public:
    explicit
    CVVideoLoaderThread(QObject *parent, CVVideoLoaderModel *model);

    ~CVVideoLoaderThread() override {};

    bool open_video(const QString& filename);
    void close_video();
    void set_flip_period(int ms) { miFlipPeriodInMillisecond = ms; }
    void set_loop(bool loop) { mbLoop = loop; }
    void start_playback();
    void stop_playback();
    // Signals the thread's run loop to terminate and unblocks waits
    void request_abort();
    void seek_to_frame(int frame_no);
    void advance_frame();
    int get_current_frame() const { return miCurrentFrame; }
    bool is_opened() const { return mbVideoOpened; }

Q_SIGNALS:
    // Emitted when a frame has been decoded; provides the raw cv::Mat before UI adoption
    void frame_decoded(cv::Mat frame);
    void video_opened(int max_frames, cv::Size size, QString format);
    void video_ended();

protected:
    void run() override;

private:
    void decode_next_frame();

    QSemaphore mFrameRequestSemaphore;
    QSemaphore mSeekSemaphore;

    bool mbAbort{false};
    bool mbPlayback{false};
    bool mbLoop{true};
    bool mbVideoOpened{false};

    int miFlipPeriodInMillisecond{100};
    int miMaxNoFrames{0};
    int miCurrentFrame{0};
    int miSeekTarget{-1};

    cv::Size mcvVideoSize{320, 240};
    QString msImageFormat{"CV_8UC3"};

    cv::VideoCapture mcvVideoCapture;
    CVVideoLoaderModel *mpModel{nullptr};
};

class CVVideoLoaderModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    friend class CVVideoLoaderThread;

    CVVideoLoaderModel();

    ~CVVideoLoaderModel() override; // Defined in cpp: performs safe shutdown

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;
    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData>, PortIndex) override;
    QWidget * embeddedWidget() override { return mpEmbeddedWidget; }
    void setModelProperty( QString &, const QVariant & ) override;
    void late_constructor() override;

    static const QString _category;
    static const QString _model_name;


private Q_SLOTS:
    void enable_changed( bool ) override;
    void em_button_clicked( int button );
    void no_frame_changed( int no_frame );
    void update_frame_ui(int frame_no);
    void video_file_opened(int max_frames, cv::Size size, QString format);
    void on_video_ended();
    void inputConnectionCreated(QtNodes::ConnectionId const&) override;
    void inputConnectionDeleted(QtNodes::ConnectionId const&) override;

private:
    void set_video_filename(QString &);
    void process_decoded_frame(cv::Mat frame);
    void ensure_frame_pool(int width, int height, int type);
    void reset_frame_pool();
    bool isShuttingDown() const { return mShuttingDown.load(std::memory_order_acquire); }

    QString msVideoFilename {""};
    int miFlipPeriodInMillisecond{100};
    bool mbLoop {true};
    QString msImage_Format{ "CV_8UC3" };
    cv::Size mcvImage_Size{ cv::Size(320,240) };
    int miMaxNoFrames{ 0 };

    CVVideoLoaderEmbeddedWidget * mpEmbeddedWidget;
    CVVideoLoaderThread * mpVideoLoaderThread{nullptr};

    std::shared_ptr< CVImageData > mpCVImageData;

    bool mbUseSyncSignal{false};

    int miPoolSize{CVImagePool::DefaultPoolSize};
    FrameSharingMode meSharingMode{FrameSharingMode::PoolMode};
    std::shared_ptr<CVImagePool> mpFramePool;
    int miPoolFrameWidth{0};
    int miPoolFrameHeight{0};
    int miActivePoolSize{0};
    QMutex mFramePoolMutex;
    int miFrameMatType{CV_8UC3};
    std::atomic<bool> mShuttingDown{false};
};
