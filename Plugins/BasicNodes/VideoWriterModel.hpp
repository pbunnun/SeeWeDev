//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#ifndef VIDEOWRITERMODEL_HPP
#define VIDEOWRITERMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QQueue>
#include <QtWidgets/QPushButton>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"
#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class VideoWriterThread : public QThread
{
    Q_OBJECT
public:
    explicit
    VideoWriterThread( QObject *parent = nullptr );

    ~VideoWriterThread() override;

    void
    add_image( const cv::Mat & );

    void
    start_writer( QString filename, double fps );

    void
    stop_writer();

Q_SIGNALS:
    void
    video_writer_error_signal(int);

protected:
    void
    run() override;

private:
    bool open_writer( const cv::Mat & image );

    QSemaphore mWaitingSemaphore;

    QString msFilename;
    double mdFPS {10};
    int miRecordingStatus {0};
    cv::Size mSize;
    int miChannels {0};

    QQueue< cv::Mat > mqCVImage;
    cv::VideoWriter mVideoWriter;
    bool mbWriterReady {false};
    bool mbAbort {false};
};

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class VideoWriterModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    VideoWriterModel();

    virtual
    ~VideoWriterModel() override
    {
        if( mpVideoWriterThread )
            delete mpVideoWriterThread;
    }

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    void
    late_constructor() override;

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:

    void
    em_button_clicked(bool);

    void
    enable_changed(bool) override;

    void
    video_writer_error_occured(int);

private:
    QPushButton * mpEmbeddedWidget;
    bool mbRecording { false };

    VideoWriterThread * mpVideoWriterThread { nullptr };

    QString msOutput_Filename;
    double mdFPS;

    void processData(const std::shared_ptr< CVImageData > & in);
};
#endif
