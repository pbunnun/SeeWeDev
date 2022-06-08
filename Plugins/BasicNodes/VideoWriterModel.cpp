#include "VideoWriterModel.hpp"

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

#include "qtvariantproperty.h"
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>

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
start_writer( QString filename, double fps )
{
    msFilename = filename;
    mdFPS = fps;
    miRecordingStatus = 1;
    if( !isRunning() )
        start();
}

void
VideoWriterThread::
stop_writer()
{
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
#elif defined( __APPLE__ )
    int fourcc = cv::VideoWriter::fourcc('D','I','V','X');
#elif defined( __linux__ )
    int fourcc = cv::VideoWriter::fourcc('D','I','V','X');
#endif
    return mVideoWriter.open( msFilename.toStdString(), fourcc, mdFPS, mSize, bColor );
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


VideoWriterModel::
VideoWriterModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget( new QPushButton( qobject_cast<QWidget *>(this) ) )
{
    mpEmbeddedWidget->setText("Start");
    mpEmbeddedWidget->setStyleSheet("QPushButton { background-color : yellow; }");
    connect( mpEmbeddedWidget, &QPushButton::clicked, this, &VideoWriterModel::em_button_clicked );

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msOutput_Filename;
    filePathPropertyType.msFilter = "*.avi";
    filePathPropertyType.msMode = "save";
    QString propId = "output_filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Output Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    IntPropertyType intPropertyType;
    intPropertyType.miMax = 60;
    intPropertyType.miMin = 1;
    intPropertyType.miValue = 10;
    propId = "fps";
    auto propFPS = std::make_shared< TypedProperty<IntPropertyType> >("FPS", propId, QVariant::Int, intPropertyType);
    mvProperty.push_back( propFPS );
    mMapIdToProperty[ propId ] = propFPS;
}

unsigned int
VideoWriterModel::
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
VideoWriterModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
    {
        return CVImageData().type();
    }
    return NodeDataType();
}

void
VideoWriterModel::
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
VideoWriterModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();
    QJsonObject cParams;
    cParams["output_filename"] = msOutput_Filename;
    cParams["fps"] = static_cast<int>(mdFPS);
    modelJson["cParams"] = cParams;
    return modelJson;
}


void
VideoWriterModel::
restore( QJsonObject const &p )
{
    PBNodeDataModel::restore( p );
    late_constructor();

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj["output_filename"];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty["output_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
            typedProp->getData() = v.toString();
            msOutput_Filename = v.toString();
        }
        v = paramsObj["fps"];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty["fps"];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
            typedProp->getData().miValue = v.toInt();
            mdFPS = static_cast<double>(v.toInt());
        }
    }
}


void
VideoWriterModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );
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
        mdFPS = static_cast<double>( value.toInt() );
    }
}


void
VideoWriterModel::
late_constructor()
{
    if( !mpVideoWriterThread )
    {
        mpVideoWriterThread = new VideoWriterThread(this);
        connect( mpVideoWriterThread, &VideoWriterThread::video_writer_error_signal, this, &VideoWriterModel::video_writer_error_occured );
    }
}

void
VideoWriterModel::
processData(const std::shared_ptr< CVImageData > & in)
{
    if( mbRecording )
    {
        cv::Mat& in_image = in->image();
        if( !in_image.empty() )
            mpVideoWriterThread->add_image( in_image );
    }
}

void
VideoWriterModel::
enable_changed(bool enable)
{
    PBNodeDataModel::enable_changed( enable );

    mpEmbeddedWidget->setEnabled( enable );
}


void
VideoWriterModel::
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
VideoWriterModel::
em_button_clicked( bool checked )
{
    if( mbRecording )
    {
        mpEmbeddedWidget->setText("Start");
        mpEmbeddedWidget->setStyleSheet("QPushButton { background-color : yellow; }");
        mpVideoWriterThread->stop_writer();
        mbRecording = false;
    }
    else
    { 
        if( msOutput_Filename.isEmpty() )
        {
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
                                                            tr("Video (*.avi)"));
            if( !filename.isEmpty() )
            {
                if( !filename.endsWith( "avi", Qt::CaseInsensitive ) )
                    filename += ".avi";
                auto prop = mMapIdToProperty["output_filename"];
                auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
                typedProp->getData() = filename;
                msOutput_Filename = filename;
            }
        }

        if( !msOutput_Filename.isEmpty() )
        {
            mpEmbeddedWidget->setText("Stop");
            mpEmbeddedWidget->setStyleSheet("QPushButton { background-color : red; }");
            mpVideoWriterThread->start_writer(msOutput_Filename, mdFPS);
            mbRecording = true;
        }
    }
}

const QString VideoWriterModel::_category = QString("Output");

const QString VideoWriterModel::_model_name = QString( "Video Writer" );
