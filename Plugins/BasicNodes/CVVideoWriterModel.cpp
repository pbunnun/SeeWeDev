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

#include "CVVideoWriterModel.hpp"


#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

#include "qtvariantproperty_p.h"
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>

const QString CVVideoWriterModel::_category = QString("Output");

const QString CVVideoWriterModel::_model_name = QString( "CV Video Writer" );

VideoWriterThread::VideoWriterThread( QObject * parent )
    : QThread(parent)
{

}


VideoWriterThread::
~VideoWriterThread()
{
    mbAbort = true;
    cv::Mat image;
    mqCVImage.enqueue( image );
    mWaitingSemaphore.release();
    wait();
}

void
VideoWriterThread::
start_writer(QString filename, int max_frame_per_video, int fps )
{
    msFilename = filename;
    miFPS = fps;
    miFramePerVideo = max_frame_per_video;
    miRecordingStatus = 1;
    if( !isRunning() )
        start();
}

void
VideoWriterThread::
stop_writer()
{
    miFrameCounter = 0;
    miRecordingStatus = 2;
    cv::Mat image;
    mqCVImage.enqueue( image );
    mWaitingSemaphore.release();
}

void
VideoWriterThread::
run()
{
    while( !mbAbort )
    {
        mWaitingSemaphore.acquire();
        cv::Mat image = mqCVImage.dequeue();
        if( !mbWriterReady )
            continue;
        if( miRecordingStatus == 2 )
        {
            mVideoWriter.release();
            miRecordingStatus = 0;
            mbWriterReady = false;
            if( mWaitingSemaphore.available() != 0 )
                mWaitingSemaphore.acquire( mWaitingSemaphore.available() );
            mqCVImage.clear();

            continue;
        }
        if( image.cols != mSize.width || image.rows != mSize.height || image.channels() != miChannels )
        {
            mVideoWriter.release();
            miRecordingStatus = 0;
            mbWriterReady = false;
            Q_EMIT video_writer_error_signal( 1 );
        }
        else
        {
            mVideoWriter << image;
            if( miFrameCounter ++ == miFramePerVideo )
            {
               miFrameCounter = 0;
               mVideoWriter.release();
               open_writer( image );
            }
        }
    }
}

bool
VideoWriterThread::
open_writer( const cv::Mat & image )
{
    mSize = cv::Size( image.cols, image.rows );
    bool bColor = false;
    miChannels = image.channels();
    if( miChannels > 1 )
        bColor = true;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    int fourcc = cv::VideoWriter::fourcc('D','I','V','X');
    QString filename = msFilename + "V" + QString::number(miFilenameCounter) + ".avi";
    while( QFile::exists( filename ) )
        filename = msFilename + "V" + QString::number(miFilenameCounter++) + ".avi";
#elif defined( __APPLE__ )
    int fourcc = cv::VideoWriter::fourcc('D','I','V','X');
    QString filename = msFilename + "V" + QString::number(miFilenameCounter) + ".avi";
    while( QFile::exists( filename ) )
        filename = msFilename + "V" + QString::number(miFilenameCounter++) + ".avi";
#elif defined( __linux__ )
    //int fourcc = cv::VideoWriter::fourcc('D','I','V','X'); // match with avi
    //int fourcc = cv::VideoWriter::fourcc('X','2','6','4');
    //int fourcc = cv::VideoWriter::fourcc('H','E','V','C');
    //int fourcc = cv::VideoWriter::fourcc('M','P','4','V');
    int fourcc = cv::VideoWriter::fourcc('m','p','4','v');
    QString filename = msFilename + "V" + QString::number(miFilenameCounter) + ".mp4";
    while( QFile::exists( filename ) )
        filename = msFilename + "V" + QString::number(miFilenameCounter++) + ".mp4";
#endif
    return mVideoWriter.open( filename.toStdString(), cv::CAP_FFMPEG, fourcc, miFPS, mSize, bColor );
}

void
VideoWriterThread::
add_image( const cv::Mat & in_image )
{
    if( !in_image.empty() )
    {
        if( !mbWriterReady )
        {
            mbWriterReady = open_writer( in_image );
            if( !mbWriterReady )
            {
                miRecordingStatus = 0;
                Q_EMIT video_writer_error_signal( 0 );
            }
            else
            {
                cv::Mat image;
                in_image.copyTo( image );
                mqCVImage.enqueue( image );
                mWaitingSemaphore.release();
            }
        }
        else
        {
            cv::Mat image;
            in_image.copyTo( image );
            mqCVImage.enqueue( image );
            mWaitingSemaphore.release();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

CVVideoWriterModel::
CVVideoWriterModel()
    : PBNodeDelegateModel( _model_name ),
      mpEmbeddedWidget( new QPushButton( qobject_cast<QWidget *>(this) ) ),
    _minPixmap(":/Video Writer.png")
{
    mpEmbeddedWidget->setText("Start");
    mpEmbeddedWidget->setStyleSheet("QPushButton { background-color : yellow; }");
    connect( mpEmbeddedWidget, &QPushButton::clicked, this, &CVVideoWriterModel::em_button_clicked );

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msOutput_Filename;
    filePathPropertyType.msFilter = "*.avi";
    filePathPropertyType.msMode = "save";
    QString propId = "output_filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Output Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    IntPropertyType intPropertyType;
    intPropertyType.miMax = 1000;
    intPropertyType.miMin = 1;
    intPropertyType.miValue = 10;
    propId = "fps";
    auto propFPS = std::make_shared< TypedProperty<IntPropertyType> >("Recorded FPS", propId, QMetaType::Int, intPropertyType);
    mvProperty.push_back( propFPS );
    mMapIdToProperty[ propId ] = propFPS;

    intPropertyType.miMax = 10000000;
    intPropertyType.miMin = 1;
    intPropertyType.miValue = 1000;
    propId = "fpv";
    auto propFPV = std::make_shared< TypedProperty<IntPropertyType> >("Frame Per Video", propId, QMetaType::Int, intPropertyType);
    mvProperty.push_back( propFPV );
    mMapIdToProperty[ propId ] = propFPV;
}

unsigned int
CVVideoWriterModel::
nPorts(PortType portType) const
{
    unsigned int result = 0;

    switch (portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 0;
        break;

    default:
        break;
    }

    return result;
}

NodeDataType
CVVideoWriterModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
    {
        return CVImageData().type();
    }
    return NodeDataType();
}

void
CVVideoWriterModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;
    if( nodeData )
    {
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
            processData( d );
    }
}


QJsonObject
CVVideoWriterModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["output_filename"] = msOutput_Filename;
    cParams["fps"] = miFPS;
    cParams["fpv"] = miFramePerVideo;
    modelJson["cParams"] = cParams;
    return modelJson;
}


void
CVVideoWriterModel::
load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);
    late_constructor();

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj["output_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["output_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
            typedProp->getData() = v.toString();
            msOutput_Filename = v.toString();
        }
        v = paramsObj["fps"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["fps"];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
            typedProp->getData().miValue = v.toInt();
            miFPS = v.toInt();
        }
        v = paramsObj["fpv"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["fpv"];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
            typedProp->getData().miValue = v.toInt();
            miFramePerVideo = v.toInt();
        }
    }
}


void
CVVideoWriterModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "output_filename" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >(prop);
        typedProp->getData() = value.toString();
        msOutput_Filename = value.toString();
    }
    else if( id == "fps" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
        typedProp->getData().miValue = value.toInt();
        miFPS = value.toInt();
    }
    else if( id == "fpv" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
        typedProp->getData().miValue = value.toInt();
        miFramePerVideo = value.toInt();
    }
}


void
CVVideoWriterModel::
late_constructor()
{
    if( !mpVideoWriterThread )
    {
        mpVideoWriterThread = new VideoWriterThread(this);
        connect( mpVideoWriterThread, &VideoWriterThread::video_writer_error_signal, this, &CVVideoWriterModel::video_writer_error_occured );
    }
}

void
CVVideoWriterModel::
processData(const std::shared_ptr< CVImageData > & in)
{
    if( mbRecording )
    {
        cv::Mat& in_image = in->data();
        if( !in_image.empty() )
            mpVideoWriterThread->add_image( in_image );
    }
}

void
CVVideoWriterModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed( enable );

    mpEmbeddedWidget->setEnabled( enable );
}


void
CVVideoWriterModel::
video_writer_error_occured( int error_code )
{
    mbRecording = false;
    mpEmbeddedWidget->setText("Start");
    mpEmbeddedWidget->setStyleSheet("QPushButton { background-color : yellow; }");
    QMessageBox msg;
    QString msgText = "Cannot start Video Writer!";
    if( error_code == 1 )
        msgText = "The input image resolution has been changed!";
    msg.setIcon( QMessageBox::Critical );
    msg.setText( msgText );
    msg.exec();
}


void
CVVideoWriterModel::
em_button_clicked( bool checked )
{
    DEBUG_LOG_INFO() << "[em_button_clicked] checked:" << checked << "isSelected:" << isSelected();
    
    // If node is not selected, select it first and block the interaction
    // User needs to click again when node is selected to perform the action
    if (!isSelected())
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Node not selected, requesting selection";
        Q_EMIT selection_request_signal();
        return;
    }
    
    if( mbRecording )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Stop recording";
        mpEmbeddedWidget->setText("Start");
        mpEmbeddedWidget->setStyleSheet("QPushButton { background-color : yellow; }");
        mpVideoWriterThread->stop_writer();
        mbRecording = false;
    }
    else
    { 
        if( msOutput_Filename.isEmpty() )
        {
            DEBUG_LOG_INFO() << "[em_button_clicked] Output filename empty, requesting file selection";
            /*
            QMessageBox msg;
            QString msgText = "Please set the output video filename in Property Browser!";
            msg.setIcon( QMessageBox::Warning );
            msg.setText( msgText );
            msg.exec();
            */
            QString filename = QFileDialog::getSaveFileName(qobject_cast<QWidget *>(this),
                                                            tr("Save a video to"),
                                                            QDir::homePath(),
                                                            tr("Video (*.avi *.mp4)"));
            if( !filename.isEmpty() )
            {
                auto prop = mMapIdToProperty["output_filename"];
                auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
                typedProp->getData() = filename;
                msOutput_Filename = filename;
            }
        }

        if( !msOutput_Filename.isEmpty() )
        {
            DEBUG_LOG_INFO() << "[em_button_clicked] Start recording to:" << msOutput_Filename;
            mpEmbeddedWidget->setText("Stop");
            mpEmbeddedWidget->setStyleSheet("QPushButton { background-color : red; }");
            mpVideoWriterThread->start_writer(msOutput_Filename, miFramePerVideo, miFPS);
            mbRecording = true;
        }
    }
}


