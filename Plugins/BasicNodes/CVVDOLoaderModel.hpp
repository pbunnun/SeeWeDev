#ifndef CVVDOLOADERMODEL_HPP
#define CVVDOLOADERMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QTimer>
#include <QElapsedTimer>
#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

#include "CVImageData.hpp"
#include <opencv2/videoio.hpp>
#include "InformationData.hpp"
#include "CVSizeData.hpp"
#include "CVVDOLoaderEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class VDOThread : public QThread
{
    Q_OBJECT
public:
    explicit VDOThread(QObject *parent = nullptr);
    ~VDOThread() override;
    void set_video_filename(QString filename);
    void set_period( int milisecond ) { miDelayTime = milisecond; };
    void set_loop_play( bool loop ) { mbLoop = loop; };
    void set_display_frame( int );

    void resume()
    {
        mMutex.lock();
        mbPause = false;
        mMutex.unlock();
        mPauseCond.wakeAll();
    }

    void pause()
    {
        mMutex.lock();
        mbPause = true;
        mMutex.unlock();
    }

    void next_frame();
    void previous_frame();

    QString & get_image_type() { return msImage_Type; };
    QString & get_image_format() { return msImage_Format; };
    cv::Size & get_image_size() { return mcvImage_Size; };
    int get_no_frames() { return miNoFrames; };

Q_SIGNALS:
    void image_ready(cv::Mat & image, int no_frame);

protected:
    void run() override;

private:
    bool mbAbort{false};
    bool mbCapturing{false};
    unsigned long miDelayTime;
    cv::Mat mcvImage;
    cv::VideoCapture mcvVideoCapture;

    QMutex mMutex;
    QWaitCondition mPauseCond;
    QElapsedTimer mElapsedTimer;
    bool mbPause{true};
    bool mbLoop{false};

    QString msImage_Type{ "Color" };
    QString msImage_Format{ "CV_8UC3" };
    cv::Size mcvImage_Size{ cv::Size(320,240) };
    int miNoFrames{ 0 };
};

class CVVDOLoaderModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CVVDOLoaderModel();

    virtual
    ~CVVDOLoaderModel() {}

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData>, int) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    bool
    resizable() const override { return true; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:
    void
    em_button_clicked( int button );

    void
    frame_changed( int no_frame );

    void
    received_image( cv::Mat & image, int no_frame );

private:
    void
    set_video_filename(QString &);

    QString msVideoFilename {""};
    int miFlipPeriodInMillisecond{30};
    bool mbLoop {true};

    CVVDOLoaderEmbeddedWidget * mpEmbeddedWidget;
    VDOThread * mpVDOThread;

    std::shared_ptr< CVImageData > mpCVImageData;
};
#endif
