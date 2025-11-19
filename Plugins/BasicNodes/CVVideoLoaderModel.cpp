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

#include "CVVideoLoaderModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtWidgets/QFileDialog>
#include "qtvariantproperty_p.h"
#include <QMessageBox>
#include "SyncData.hpp"
#include <QtNodes/internal/ConnectionIdUtils.hpp>

const QString CVVideoLoaderModel::_category = QString( "Source" );

const QString CVVideoLoaderModel::_model_name = QString( "CV Video Loader" );

/////////////////////////////////////////////////////////////////////////
// CVVideoLoaderThread Implementation
/////////////////////////////////////////////////////////////////////////

CVVideoLoaderThread::CVVideoLoaderThread(QObject *parent, std::shared_ptr<CVImageData> pCVImageData)
    : QThread(parent), mpCVImageData(pCVImageData)
{
}

CVVideoLoaderThread::~CVVideoLoaderThread()
{
    mbAbort = true;
    mFrameRequestSemaphore.release();
    mSeekSemaphore.release();
    wait();
}

bool
CVVideoLoaderThread::open_video(const QString& filename)
{
    if (mbVideoOpened)
    {
        close_video();
    }

    mcvVideoCapture = cv::VideoCapture(filename.toStdString());
    if (!mcvVideoCapture.isOpened())
    {
        mbVideoOpened = false;
        return false;
    }

    mbVideoOpened = true;
    miMaxNoFrames = static_cast<int>(mcvVideoCapture.get(cv::CAP_PROP_FRAME_COUNT));
    miCurrentFrame = 0;

    cv::Mat firstFrame;
    mcvVideoCapture >> firstFrame;
    if (!firstFrame.empty())
    {
        mcvVideoSize = cv::Size(firstFrame.cols, firstFrame.rows);
        int channels = firstFrame.channels();
        if (channels == 1)
            msImageFormat = "CV_8UC1";
        else if (channels == 3)
            msImageFormat = "CV_8UC3";

        mpCVImageData->data() = firstFrame.clone();
        mpCVImageData->set_timestamp();
        miCurrentFrame = 1;

        Q_EMIT video_opened(miMaxNoFrames, mcvVideoSize, msImageFormat);
        Q_EMIT frame_ready(0);
    }
    else
    {
        close_video();
        return false;
    }

    if (!isRunning())
        start();

    return true;
}

void
CVVideoLoaderThread::close_video()
{
    if (mcvVideoCapture.isOpened())
    {
        mcvVideoCapture.release();
    }
    mbVideoOpened = false;
    mbPlayback = false;
    miMaxNoFrames = 0;
    miCurrentFrame = 0;
}

void
CVVideoLoaderThread::start_playback()
{
    mbPlayback = true;
    mFrameRequestSemaphore.release();
}

void
CVVideoLoaderThread::stop_playback()
{
    mbPlayback = false;
}

void
CVVideoLoaderThread::seek_to_frame(int frame_no)
{
    if (!mbVideoOpened || frame_no < 0 || frame_no >= miMaxNoFrames)
        return;

    miSeekTarget = frame_no;
    mSeekSemaphore.release();
}

void
CVVideoLoaderThread::advance_frame()
{
    mFrameRequestSemaphore.release();
}

void
CVVideoLoaderThread::run()
{
    while (!mbAbort)
    {
        if (mSeekSemaphore.tryAcquire())
        {
            if (mbVideoOpened && miSeekTarget >= 0 && miSeekTarget < miMaxNoFrames)
            {
                mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, miSeekTarget);
                mcvVideoCapture >> mpCVImageData->data();
                if (!mpCVImageData->data().empty())
                {
                    mpCVImageData->set_timestamp();
                    miCurrentFrame = miSeekTarget + 1;
                    Q_EMIT frame_ready(miSeekTarget);
                }
                miSeekTarget = -1;
            }
        }

        if (mbPlayback)
        {
            decode_next_frame();
            QThread::msleep(miFlipPeriodInMillisecond);
        }
        else if (mFrameRequestSemaphore.tryAcquire(1, 10))
        {
            decode_next_frame();
        }
        else
        {
            QThread::msleep(10);
        }
    }
}

void
CVVideoLoaderThread::decode_next_frame()
{
    if (!mbVideoOpened)
        return;

    mcvVideoCapture >> mpCVImageData->data();
    if (!mpCVImageData->data().empty())
    {
        mpCVImageData->set_timestamp();
        Q_EMIT frame_ready(miCurrentFrame);
        miCurrentFrame++;
    }
    else
    {
        if (mbLoop)
        {
            mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
            mcvVideoCapture >> mpCVImageData->data();
            if (!mpCVImageData->data().empty())
            {
                mpCVImageData->set_timestamp();
                miCurrentFrame = 1;
                Q_EMIT frame_ready(0);
            }
        }
        else
        {
            mbPlayback = false;
            Q_EMIT video_ended();
        }
    }
}

/////////////////////////////////////////////////////////////////////////
// CVVideoLoaderModel Implementation
/////////////////////////////////////////////////////////////////////////

CVVideoLoaderModel::
CVVideoLoaderModel()
    : PBNodeDelegateModel( _model_name, true ),
    mpEmbeddedWidget( new CVVideoLoaderEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    qRegisterMetaType<cv::Size>( "cv::Size" );
}

unsigned int
CVVideoLoaderModel::
nPorts( PortType ) const
{
    return 1;
}

NodeDataType
CVVideoLoaderModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::Out )
    {
        if( portIndex == 0 )
            return CVImageData().type();
    }
    else if( portType == PortType::In )
    {
        if( portIndex == 0 )
            return SyncData().type();
    }
    return NodeDataType();
}

void
CVVideoLoaderModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex )
{
    if( !isEnable() )
        return;
    if( portIndex == 0 )
    {
        auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( d && d->data() && mpVDOLoaderThread )
        {
            mpVDOLoaderThread->advance_frame();
        }
    }
}

void
CVVideoLoaderModel::
frame_decoded(int frame_no)
{
    mpEmbeddedWidget->set_slider_value(frame_no);
    if (isEnable())
        Q_EMIT dataUpdated(0);
}

void
CVVideoLoaderModel::
video_file_opened(int max_frames, cv::Size size, QString format)
{
    miMaxNoFrames = max_frames;
    mcvImage_Size = size;
    msImage_Format = format;

    mpEmbeddedWidget->set_maximum_slider(miMaxNoFrames);

    auto prop = mMapIdToProperty["image_size"];
    auto typedPropSize = std::static_pointer_cast<TypedProperty<SizePropertyType>>(prop);
    typedPropSize->getData().miWidth = size.width;
    typedPropSize->getData().miHeight = size.height;
    Q_EMIT property_changed_signal(prop);

    prop = mMapIdToProperty["image_format"];
    auto typedPropFormat = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedPropFormat->getData() = msImage_Format;
    Q_EMIT property_changed_signal(prop);

    if (isEnable())
        Q_EMIT dataUpdated(0);
}

void
CVVideoLoaderModel::
on_video_ended()
{
    mpEmbeddedWidget->pause_video();
}

std::shared_ptr<NodeData>
CVVideoLoaderModel::
outData(PortIndex portIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
    {
        if( portIndex == 0 && mpCVImageData->data().data != nullptr)
            result = mpCVImageData;
    }
    return result;
}

QJsonObject
CVVideoLoaderModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    if( !msVideoFilename.isEmpty() )
    {
        QJsonObject cParams;
        cParams["filename"] = msVideoFilename;
        cParams["flip_period"] = miFlipPeriodInMillisecond;
        cParams["is_loop"] = mbLoop;
        cParams["use_sync_signal"] = mbUseSyncSignal;
        modelJson["cParams"] = cParams;
    }
    return modelJson;
}

void
CVVideoLoaderModel::
load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "flip_period" ];
        if( !v.isNull() )
            miFlipPeriodInMillisecond = v.toInt();

        v = paramsObj[ "use_sync_signal" ];
        if( !v.isNull() )
            mbUseSyncSignal = v.toBool();

        v = paramsObj[ "is_loop" ];
        if( !v.isNull() )
            mbLoop = v.toBool();

        v = paramsObj[ "filename" ];
        if( !v.isNull() )
            msVideoFilename = v.toString();
    }
}

void
CVVideoLoaderModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    if( id == "filename" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
        typedProp->getData().msFilename = value.toString();

        QString filename = value.toString();
        set_video_filename( filename );
    }
    else if( id == "flip_period" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
        typedProp->getData().miValue = value.toInt();

        miFlipPeriodInMillisecond = value.toInt();
        if (mpVDOLoaderThread)
            mpVDOLoaderThread->set_flip_period(miFlipPeriodInMillisecond);
    }
    else if( id == "is_loop" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbLoop = value.toBool();
        if (mpVDOLoaderThread)
            mpVDOLoaderThread->set_loop(mbLoop);
    }
}

void
CVVideoLoaderModel::
set_video_filename(QString & filename)
{
    if( msVideoFilename == filename )
        return;

    if( !QFile::exists( filename ) )
        return;

    msVideoFilename = filename;

    QFileInfo fi( msVideoFilename );
    mpEmbeddedWidget->set_filename( fi.fileName() );

    if (mpVDOLoaderThread && mpVDOLoaderThread->open_video(msVideoFilename))
    {
        // metadata will arrive via video_opened signal
    }
}

void
CVVideoLoaderModel::
late_constructor()
{
    if ( start_late_constructor() )
    {
        connect( mpEmbeddedWidget, &CVVideoLoaderEmbeddedWidget::button_clicked_signal, this, &CVVideoLoaderModel::em_button_clicked );
        connect( mpEmbeddedWidget, &CVVideoLoaderEmbeddedWidget::slider_value_signal, this, &CVVideoLoaderModel::no_frame_changed );
        connect( mpEmbeddedWidget, &CVVideoLoaderEmbeddedWidget::widget_resized_signal, this, &CVVideoLoaderModel::embeddedWidgetSizeUpdated );

        FilePathPropertyType filePathPropertyType;
        filePathPropertyType.msFilename = msVideoFilename;
        filePathPropertyType.msFilter = "*.mp4;*.mpg;*.wmv;*.avi";
        filePathPropertyType.msMode = "open";
        QString propId = "filename";
        auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType > >( "Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType );
        mvProperty.push_back( propFileName );
        mMapIdToProperty[ propId ] = propFileName;

        IntPropertyType intPropertyType;
        intPropertyType.miMax = 60000;
        intPropertyType.miMin = 0;
        intPropertyType.miValue = miFlipPeriodInMillisecond;
        propId = "flip_period";
        auto propSpinBox = std::make_shared< TypedProperty< IntPropertyType > >( "Flip Period (ms)", propId, QMetaType::Int, intPropertyType );
        mvProperty.push_back( propSpinBox );
        mMapIdToProperty[ propId ] = propSpinBox;

        propId = "is_loop";
        auto propIsLoop = std::make_shared< TypedProperty< bool > >( "Loop Play", propId, QMetaType::Bool, mbLoop );
        mvProperty.push_back( propIsLoop );
        mMapIdToProperty[ propId ] = propIsLoop;

        SizePropertyType sizePropertyType;
        sizePropertyType.miWidth = 0;
        sizePropertyType.miHeight = 0;
        propId = "image_size";
        auto propImageSize = std::make_shared< TypedProperty< SizePropertyType > >( "Size", propId, QMetaType::QSize, sizePropertyType, "", true );
        mvProperty.push_back( propImageSize );
        mMapIdToProperty[ propId ] = propImageSize;

        propId = "image_format";
        auto propFormat = std::make_shared< TypedProperty< QString > >( "Format", propId, QMetaType::QString, "", "", true );
        mvProperty.push_back( propFormat );
        mMapIdToProperty[ propId ] = propFormat;

        mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
        mpVDOLoaderThread = new CVVideoLoaderThread(this, mpCVImageData);
        connect(mpVDOLoaderThread, &CVVideoLoaderThread::frame_ready, this, &CVVideoLoaderModel::frame_decoded);
        connect(mpVDOLoaderThread, &CVVideoLoaderThread::video_opened, this, &CVVideoLoaderModel::video_file_opened);
        connect(mpVDOLoaderThread, &CVVideoLoaderThread::video_ended, this, &CVVideoLoaderModel::on_video_ended);

        mpVDOLoaderThread->set_flip_period(miFlipPeriodInMillisecond);
        mpVDOLoaderThread->set_loop(mbLoop);
        if( !msVideoFilename.isEmpty() )
        {
            if( QFile::exists( msVideoFilename ) )
            {
                QFileInfo fi( msVideoFilename );
                mpEmbeddedWidget->set_filename( fi.fileName() );
                mpVDOLoaderThread->open_video( msVideoFilename );
            }
        }
    }
}

int CVVideoLoaderModel::getMaxNoFrames() const
{
    return miMaxNoFrames;
}

void CVVideoLoaderModel::setMaxNoFrames(int newMaxNoFrames)
{
    miMaxNoFrames = newMaxNoFrames;
}

void
CVVideoLoaderModel::
em_button_clicked( int button )
{
    DEBUG_LOG_INFO() << "[em_button_clicked] button:" << button << "isSelected:" << isSelected();

    if (!isSelected())
    {
        if( button == 1 || button == 2 )
            mpEmbeddedWidget->set_toggle_play( button == 2 ? true : false );
        Q_EMIT selection_request_signal();
        return;
    }

    if( button == 0 )
    {
        if( !mpVDOLoaderThread || !mpVDOLoaderThread->is_opened() )
        {
            return;
        }
        int currentFrame = mpVDOLoaderThread->get_current_frame();
        if( currentFrame >= 2 )
            mpVDOLoaderThread->seek_to_frame(currentFrame - 2);
    }
    else if( button == 1 )
    {
        if( !mpVDOLoaderThread || !mpVDOLoaderThread->is_opened() )
        {
            mpEmbeddedWidget->set_toggle_play( false );
            return;
        }
        mpVDOLoaderThread->start_playback();
    }
    else if( button == 2 )
    {
        if( !mpVDOLoaderThread || !mpVDOLoaderThread->is_opened() )
        {
            return;
        }
        mpVDOLoaderThread->stop_playback();
    }
    else if( button == 3 )
    {
        if( !mpVDOLoaderThread || !mpVDOLoaderThread->is_opened() )
        {
            return;
        }
        int currentFrame = mpVDOLoaderThread->get_current_frame();
        if( currentFrame < miMaxNoFrames )
            mpVDOLoaderThread->advance_frame();
        else if( mbLoop )
        {
            mpVDOLoaderThread->seek_to_frame(0);
        }
    }
    else if( button == 4 )
    {
        QString dir = QDir::homePath();
        if( !msVideoFilename.isEmpty() )
        {
            QFileInfo fi(msVideoFilename);
            if( !fi.absoluteDir().isEmpty() )
                dir = fi.absoluteDir().absolutePath();
        }
        QString filename = QFileDialog::getOpenFileName( nullptr,
                                                         tr( "Open Video File" ),
                                                         dir,
                                                         tr( "Video Files (*.mp4 *.mpg *.wmv *.avi") );
        if( !filename.isEmpty() )
        {
            auto prop = mMapIdToProperty[ "filename" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
            typedProp->getData().msFilename = filename;

            set_video_filename( filename );
            if( isSelected() )
                Q_EMIT property_changed_signal( prop );
        }
    }
}

void
CVVideoLoaderModel::
no_frame_changed( int no_frame )
{
    if (!isSelected())
    {
        mpEmbeddedWidget->set_slider_value( 0 );
        Q_EMIT selection_request_signal();
        return;
    }

    if( !mpVDOLoaderThread || !mpVDOLoaderThread->is_opened() )
    {
        mpEmbeddedWidget->set_slider_value( 0 );
        return;
    }

    if( no_frame < miMaxNoFrames )
    {
        mpVDOLoaderThread->seek_to_frame(no_frame);
    }
}

void
CVVideoLoaderModel::
inputConnectionCreated(QtNodes::ConnectionId const& conx)
{
    if( QtNodes::getPortIndex(PortType::In, conx) == 0 )
        mbUseSyncSignal = true;
}

void
CVVideoLoaderModel::
inputConnectionDeleted(QtNodes::ConnectionId const& conx)
{
    if( QtNodes::getPortIndex(PortType::In, conx) == 0 )
        mbUseSyncSignal = false;
}

void
CVVideoLoaderModel::
enable_changed( bool enable )
{
    PBNodeDelegateModel::enable_changed( enable );

    if( !enable )
    {
        if (mpVDOLoaderThread)
            mpVDOLoaderThread->stop_playback();
        mpEmbeddedWidget->pause_video();
    }
}
