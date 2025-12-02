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
#include <QtCore/QMutexLocker>
#include <QtCore/QTime>
#include <QtCore/QtGlobal>
#include <QtWidgets/QFileDialog>
#include "qtvariantproperty_p.h"
#include <QMessageBox>
#include "SyncData.hpp"
#include <QtNodes/internal/ConnectionIdUtils.hpp>

using CVDevLibrary::FrameMetadata;

const QString CVVideoLoaderModel::_category = QString( "Source" );

const QString CVVideoLoaderModel::_model_name = QString( "CV Video Loader" );

/////////////////////////////////////////////////////////////////////////
// CVVideoLoaderThread Implementation
/////////////////////////////////////////////////////////////////////////

CVVideoLoaderThread::CVVideoLoaderThread(QObject *parent, CVVideoLoaderModel *model)
    : QThread(parent)
    , mpModel(model)
{
}

void CVVideoLoaderThread::request_abort()
{
    mbAbort = true;
    mFrameRequestSemaphore.release();
    mSeekSemaphore.release();
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

        // Signal-only handoff of first frame; model will adopt & ensure pool
        Q_EMIT frame_decoded(firstFrame);
        miCurrentFrame = 1;

        Q_EMIT video_opened(miMaxNoFrames, mcvVideoSize, msImageFormat);
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
    if (mpModel)
        mpModel->reset_frame_pool();
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
    if( miCurrentFrame != frame_no )
    { 
        miSeekTarget = frame_no;
        mSeekSemaphore.release();
    }
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
                cv::Mat frame;
                mcvVideoCapture >> frame;
                if (!frame.empty())
                {
                    miCurrentFrame = miSeekTarget;
                    Q_EMIT frame_decoded(frame);
                }
                miSeekTarget = -1;
            }
            continue;
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

    cv::Mat frame;
    mcvVideoCapture >> frame;
    if (!frame.empty())
    {
        Q_EMIT frame_decoded(frame);
        miCurrentFrame++;
    }
    else
    {
        if (mbLoop)
        {
            mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
            mcvVideoCapture >> frame;
            if (!frame.empty())
            {
                miCurrentFrame = 1;
                Q_EMIT frame_decoded(frame);
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

    IntPropertyType poolSizeProperty;
    poolSizeProperty.miMin = 1;
    poolSizeProperty.miMax = 128;
    poolSizeProperty.miValue = miPoolSize;
    propId = "pool_size";
    auto propPoolSize = std::make_shared< TypedProperty< IntPropertyType > >( "Pool Size", propId, QMetaType::Int, poolSizeProperty );
    mvProperty.push_back( propPoolSize );
    mMapIdToProperty[ propId ] = propPoolSize;

    EnumPropertyType sharingModeProperty;
    sharingModeProperty.mslEnumNames = { "Pool Mode", "Broadcast Mode" };
    sharingModeProperty.miCurrentIndex = ( meSharingMode == FrameSharingMode::PoolMode ) ? 0 : 1;
    propId = "sharing_mode";
    auto propSharingMode = std::make_shared< TypedProperty< EnumPropertyType > >( "Sharing Mode", propId, QtVariantPropertyManager::enumTypeId(), sharingModeProperty );
    mvProperty.push_back( propSharingMode );
    mMapIdToProperty[ propId ] = propSharingMode;
}

CVVideoLoaderModel::~CVVideoLoaderModel()
{
    mShuttingDown.store(true, std::memory_order_release);
    if (mpVideoLoaderThread)
    {
        mpVideoLoaderThread->stop_playback();
        mpVideoLoaderThread->request_abort(); // signal abort to run loop
        mpVideoLoaderThread->requestInterruption();
        mpVideoLoaderThread->wait();
        disconnect(mpVideoLoaderThread, nullptr, this, nullptr);
    }
    // Release any pooled frame handles before destroying pool
    mpCVImageData.reset();
    reset_frame_pool();
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
        if( d && d->data() && mpVideoLoaderThread )
        {
            mpVideoLoaderThread->advance_frame();
        }
    }
}

void
CVVideoLoaderModel::
update_frame_ui(int frame_no)
{
    mpEmbeddedWidget->set_slider_value(frame_no);
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
        cParams["pool_size"] = miPoolSize;
        cParams["sharing_mode"] = (meSharingMode == FrameSharingMode::PoolMode) ? 0 : 1;
        modelJson["cParams"] = cParams;
    }
    return modelJson;
}

void
CVVideoLoaderModel::
load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);
    late_constructor();

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "flip_period" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "flip_period" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            miFlipPeriodInMillisecond = v.toInt();
            if (mpVideoLoaderThread)
                mpVideoLoaderThread->set_flip_period(miFlipPeriodInMillisecond);
        }

        v = paramsObj[ "use_sync_signal" ];
        if( !v.isNull() )
        {
            mbUseSyncSignal = v.toBool();
        }

        v = paramsObj[ "is_loop" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "is_loop" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mbLoop = v.toBool();
        }

        v = paramsObj[ "pool_size" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "pool_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            int newSize = qMax( 1, qMin(128, v.toInt()) );
            typedProp->getData().miValue = newSize;
            
            miPoolSize = newSize;
        }

        v = paramsObj[ "sharing_mode" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "sharing_mode" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            int enumCount = static_cast<int>( typedProp->getData().mslEnumNames.size() );
            if( enumCount <= 0 )
                enumCount = 1;
            int newIndex = qBound( 0, v.toInt(), enumCount - 1 );
            typedProp->getData().miCurrentIndex = newIndex;
            
            meSharingMode = ( newIndex == 0) ? FrameSharingMode::PoolMode : FrameSharingMode::BroadcastMode;
        }

        v = paramsObj[ "filename" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "filename" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
            typedProp->getData().msFilename = v.toString();

            QString filename = v.toString();
            set_video_filename( filename );
        }
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
        if (mpVideoLoaderThread)
            mpVideoLoaderThread->set_flip_period(miFlipPeriodInMillisecond);
    }
    else if( id == "is_loop" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbLoop = value.toBool();
        if (mpVideoLoaderThread)
            mpVideoLoaderThread->set_loop(mbLoop);
    }
    else if( id == "pool_size" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        int newSize = qMax( 1, qMin(128, value.toInt()) );
        if( miPoolSize == newSize )
            return;

        typedProp->getData().miValue = newSize;
        miPoolSize = newSize;
        reset_frame_pool();
        ensure_frame_pool( mcvImage_Size.width, mcvImage_Size.height, miFrameMatType );
    }
    else if( id == "sharing_mode" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        int enumCount = static_cast<int>( typedProp->getData().mslEnumNames.size() );
        if( enumCount <= 0 )
            enumCount = 1;
        int newIndex = qBound( 0, value.toInt(), enumCount - 1 );
        FrameSharingMode newMode = ( newIndex == 0 ) ? FrameSharingMode::PoolMode : FrameSharingMode::BroadcastMode;
        if( meSharingMode == newMode && typedProp->getData().miCurrentIndex == newIndex )
            return;

        typedProp->getData().miCurrentIndex = newIndex;
        meSharingMode = newMode;

        std::shared_ptr<CVImagePool> poolCopy;
        {
            QMutexLocker locker( &mFramePoolMutex );
            poolCopy = mpFramePool;
        }
        if( poolCopy )
            poolCopy->setMode( meSharingMode );

        if( meSharingMode == FrameSharingMode::PoolMode )
            ensure_frame_pool( mcvImage_Size.width, mcvImage_Size.height, miFrameMatType );
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

    if ( mpVideoLoaderThread )
        mpVideoLoaderThread->open_video(msVideoFilename);
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

        mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
        mpVideoLoaderThread = new CVVideoLoaderThread(this, this);
        connect(mpVideoLoaderThread, &CVVideoLoaderThread::frame_decoded, this, &CVVideoLoaderModel::process_decoded_frame);
        connect(mpVideoLoaderThread, &CVVideoLoaderThread::video_opened, this, &CVVideoLoaderModel::video_file_opened);
        connect(mpVideoLoaderThread, &CVVideoLoaderThread::video_ended, this, &CVVideoLoaderModel::on_video_ended);

        mpVideoLoaderThread->set_flip_period(miFlipPeriodInMillisecond);
        mpVideoLoaderThread->set_loop(mbLoop);
    }
}


void
CVVideoLoaderModel::
em_button_clicked( int button )
{
    if (!isSelected())
    {
        if( button == 1 || button == 2 )
            mpEmbeddedWidget->set_toggle_play( button == 2 ? true : false );
        Q_EMIT selection_request_signal();
        return;
    }

    if( button == 0 )
    {
        if( !mpVideoLoaderThread || !mpVideoLoaderThread->is_opened() )
        {
            return;
        }
        int currentFrame = mpVideoLoaderThread->get_current_frame();
        if( currentFrame >= 1 )
            mpVideoLoaderThread->seek_to_frame(currentFrame - 1);
    }
    else if( button == 1 )
    {
        if( !mpVideoLoaderThread || !mpVideoLoaderThread->is_opened() )
        {
            mpEmbeddedWidget->set_toggle_play( false );
            return;
        }
        mpVideoLoaderThread->start_playback();
    }
    else if( button == 2 )
    {
        if( !mpVideoLoaderThread || !mpVideoLoaderThread->is_opened() )
        {
            return;
        }
        mpVideoLoaderThread->stop_playback();
    }
    else if( button == 3 )
    {
        if( !mpVideoLoaderThread || !mpVideoLoaderThread->is_opened() )
        {
            return;
        }
        int currentFrame = mpVideoLoaderThread->get_current_frame();
        if( currentFrame < miMaxNoFrames )
            mpVideoLoaderThread->advance_frame();
        else if( mbLoop )
        {
            mpVideoLoaderThread->seek_to_frame(0);
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
                                 tr( "Video Files (*.mp4 *.mpg *.wmv *.avi)" ) );
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

    if( !mpVideoLoaderThread || !mpVideoLoaderThread->is_opened() )
    {
        mpEmbeddedWidget->set_slider_value( 0 );
        return;
    }
    
    if( no_frame < miMaxNoFrames )
    {
        mpVideoLoaderThread->seek_to_frame(no_frame);
    }
}

void
CVVideoLoaderModel::
inputConnectionCreated(QtNodes::ConnectionId const& conx)
{
    if( QtNodes::getPortIndex(PortType::In, conx) == 0 )
    {
        mbUseSyncSignal = true;
    }
}

void
CVVideoLoaderModel::
inputConnectionDeleted(QtNodes::ConnectionId const& conx)
{
    if( QtNodes::getPortIndex(PortType::In, conx) == 0 )
    {
        mbUseSyncSignal = false;
    }
}

void
CVVideoLoaderModel::
process_decoded_frame(cv::Mat frame)
{
    if( frame.empty() || isShuttingDown() )
        return;

    FrameMetadata metadata;
    metadata.producerId = getNodeId();
    if( mpVideoLoaderThread )
        metadata.frameId = mpVideoLoaderThread->get_current_frame();

    // Create a fresh CVImageData per frame to avoid overwriting pooled slots still in use.
    auto newImageData = std::make_shared<CVImageData>(cv::Mat());

    bool pooled = false;
    if( meSharingMode == FrameSharingMode::PoolMode )
    {
        ensure_frame_pool( frame.cols, frame.rows, frame.type() );
        std::shared_ptr<CVImagePool> poolCopy;
        {
            QMutexLocker locker( &mFramePoolMutex );
            poolCopy = mpFramePool;
        }
        if( poolCopy )
        {
            auto metadataForPool = metadata;
            auto handle = poolCopy->acquire( 1, std::move( metadataForPool ) );
            if( handle )
            {
                frame.copyTo( handle.matrix() );
                if( newImageData->adoptPoolFrame( std::move( handle ) ) )
                    pooled = true;
            }
        }
    }

    if( !pooled )
    {
        newImageData->updateMove( std::move( frame ), metadata );
    }

    mpCVImageData = std::move(newImageData);

    // Update UI slider with current frame
    if( mpVideoLoaderThread )
        update_frame_ui(mpVideoLoaderThread->get_current_frame());

    // Emit data update to downstream consumers
    if( isEnable() )
        Q_EMIT dataUpdated(0);

    // Synchronous mode: pacing is handled by the merged sync signal arriving
    // on input port 0; the thread waits for `advance_frame()` so no explicit
    // per-frame semaphore/acknowledgement is required here.
}

void
CVVideoLoaderModel::
ensure_frame_pool(int width, int height, int type)
{
    if( width <= 0 || height <= 0 )
        return;

    const int desiredSize = qMax( 1, miPoolSize );
    QMutexLocker locker( &mFramePoolMutex );
    const bool shouldRecreate = !mpFramePool ||
        miPoolFrameWidth != width ||
        miPoolFrameHeight != height ||
        miFrameMatType != type ||
        miActivePoolSize != desiredSize;

    if( shouldRecreate )
    {
        mpFramePool = std::make_shared<CVImagePool>( getNodeId(), width, height, type, static_cast<size_t>( desiredSize ) );
        miPoolFrameWidth = width;
        miPoolFrameHeight = height;
        miFrameMatType = type;
        miActivePoolSize = desiredSize;
    }

    if( mpFramePool )
        mpFramePool->setMode( meSharingMode );
}

void
CVVideoLoaderModel::
reset_frame_pool()
{
    QMutexLocker locker( &mFramePoolMutex );
    mpFramePool.reset();
    miPoolFrameWidth = 0;
    miPoolFrameHeight = 0;
    miActivePoolSize = 0;
}

void
CVVideoLoaderModel::
enable_changed( bool enable )
{
    PBNodeDelegateModel::enable_changed( enable );
    if( !enable )
    {
        if (mpVideoLoaderThread)
            mpVideoLoaderThread->stop_playback();
        mpEmbeddedWidget->pause_video();
    }
}
