#include "CVVDOLoaderModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtWidgets/QFileDialog>
#include "qtvariantproperty.h"
#include <QMessageBox>
#include "SyncData.hpp"

VDOThread::
VDOThread(QObject *parent)
    : QThread(parent)
{

}

VDOThread::
~VDOThread()
{
    mbAbort = true;
    mPauseCond.wakeAll();
    wait();
}

void
VDOThread::
run()
{
    qint64 duration = 0;
    while( !mbAbort )
    {
        mElapsedTimer.start();
        mMutex.lock();
        if( mbPause )
            mPauseCond.wait(&mMutex);
        mMutex.unlock();
        if( mbCapturing )
        {
            auto no_frame = mcvVideoCapture.get(cv::CAP_PROP_POS_FRAMES);
            if( no_frame < miNoFrames )
            {
                mcvVideoCapture >> mcvImage;
                Q_EMIT image_ready(mcvImage , no_frame);
            }
            else if( mbLoop )
            {
                mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, -1);
                mcvVideoCapture >> mcvImage;
                Q_EMIT image_ready(mcvImage, 0);
            }
            else
                pause();
            duration = miDelayTime - mElapsedTimer.elapsed() - 4;
            if( duration > 0 )
                msleep(duration);
        }
        else
            msleep(miDelayTime);
    }
}

void
VDOThread::
set_display_frame( int no_frame )
{
    if( !mbPause )
        return;
    if( no_frame < miNoFrames )
    {
        mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, no_frame );
        mcvVideoCapture >> mcvImage;
        Q_EMIT image_ready(mcvImage, no_frame);
    }
}

void
VDOThread::
next_frame()
{
    if( !mbPause )
        return;
    auto no_frame = mcvVideoCapture.get(cv::CAP_PROP_POS_FRAMES);
    if( no_frame < miNoFrames )
    {
        mcvVideoCapture >> mcvImage;
        Q_EMIT image_ready(mcvImage, no_frame);
    }
    else if( mbLoop )
    {
        mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, -1);
        mcvVideoCapture >> mcvImage;
        Q_EMIT image_ready(mcvImage, 0);
    }
}

void
VDOThread::
previous_frame()
{
    if( !mbPause )
        return;
    auto no_frame = mcvVideoCapture.get(cv::CAP_PROP_POS_FRAMES);
    if( no_frame != 1 )
    {
        mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, no_frame - 2);
        mcvVideoCapture >> mcvImage;
        Q_EMIT image_ready(mcvImage, no_frame - 1);
    }
    else
    {
        mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, miNoFrames - 2 );
        mcvVideoCapture >> mcvImage;
        Q_EMIT image_ready(mcvImage, miNoFrames - 1);
    }
}

void
VDOThread::
set_video_filename(QString filename)
{
    if( mbCapturing )
        mcvVideoCapture.release();
    mcvVideoCapture = cv::VideoCapture(filename.toStdString());
    if( mcvVideoCapture.isOpened() )
    {
        /*
        double fps = mcvVideoCapture.get(cv::CAP_PROP_FPS);
        if( fps != 0. )
            miDelayTime = floor(1000./fps);
        else
            miDelayTime = 30;
        */
        mcvVideoCapture >> mcvImage;
        if( !mcvImage.empty() )
        {
            auto channels = mcvImage.channels();
            if( channels == 1 )
            {
                msImage_Format = "CV_8UC1";
                msImage_Type = "Gray";
            }
            else if( channels == 3 )
            {
                msImage_Format = "CV_8UC3";
                msImage_Type = "Color";
            }
            mcvImage_Size = cv::Size(mcvImage.cols, mcvImage.rows);
            miNoFrames = mcvVideoCapture.get(cv::CAP_PROP_FRAME_COUNT);
            Q_EMIT image_ready(mcvImage, 0);
        }
    }
    if( !mbCapturing )
    {
        mbCapturing = true;
        start();
    }
}

/////////////////////////////////////////////////////////////////////////
/// \brief CVVDOLoaderModel::CVVDOLoaderModel
///
CVVDOLoaderModel::
CVVDOLoaderModel()
    : PBNodeDataModel( _model_name, true ),
      mpEmbeddedWidget( new CVVDOLoaderEmbeddedWidget( qobject_cast<QWidget *>(this) ) ),
      mpVDOThread( new VDOThread(this) )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpVDOThread, &VDOThread::image_ready, this, &CVVDOLoaderModel::received_image );

    mpEmbeddedWidget->set_active(false);
    connect( mpEmbeddedWidget, &CVVDOLoaderEmbeddedWidget::button_clicked_signal, this, &CVVDOLoaderModel::em_button_clicked );
    connect( mpEmbeddedWidget, &CVVDOLoaderEmbeddedWidget::slider_value_signal, this, &CVVDOLoaderModel::frame_changed );

    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

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
    intPropertyType.miMin = 5;
    intPropertyType.miValue = miFlipPeriodInMillisecond;
    propId = "flip_period";
    auto propSpinBox = std::make_shared< TypedProperty< IntPropertyType > >( "Flip Period (ms)", propId, QVariant::Int, intPropertyType );
    mvProperty.push_back( propSpinBox );
    mMapIdToProperty[ propId ] = propSpinBox;

    propId = "is_loop";
    auto propIsLoop = std::make_shared< TypedProperty< bool > >( "Loop Play", propId, QVariant::Bool, true );
    mvProperty.push_back( propIsLoop );
    mMapIdToProperty[ propId ] = propIsLoop;

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = 0;
    sizePropertyType.miHeight = 0;
    propId = "image_size";
    auto propImageSize = std::make_shared< TypedProperty< SizePropertyType > >( "Size", propId, QVariant::Size, sizePropertyType, "", true );
    mvProperty.push_back( propImageSize );
    mMapIdToProperty[ propId ] = propImageSize;

    propId = "image_format";
    auto propFormat = std::make_shared< TypedProperty< QString > >( "Format", propId, QVariant::String, "", "", true );
    mvProperty.push_back( propFormat );
    mMapIdToProperty[ propId ] = propFormat;
}

void
CVVDOLoaderModel::
received_image( cv::Mat & image, int no_frame )
{
    if( image.data != nullptr )
    {
        mpCVImageData->set_image( image );
        mpEmbeddedWidget->set_slider_value( no_frame );
        if( isEnable() )
            Q_EMIT dataUpdated( 0 );
    }
}

unsigned int
CVVDOLoaderModel::
nPorts( PortType portType ) const
{
    switch ( portType )
    {
    case PortType::In:
        return( 1 );
    case PortType::Out:
        return( 1 );
    default:
        return( 0 );
    }
}

NodeDataType
CVVDOLoaderModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::Out )
    {
        if( portIndex == 0 )
            return CVImageData().type();
        else
            return NodeDataType();
    }
    else if( portType == PortType::In )
    {
        if( portIndex == 0 )
            return SyncData().type();
        else
            return NodeDataType();
    }
    else
        return NodeDataType();
}

void
CVVDOLoaderModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex )
{
    if( !isEnable() )
        return;
    if( portIndex == 0 )
    {
        auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( d )
        {
            //Do something here with Sync Signal....
        }
    }
}

std::shared_ptr<NodeData>
CVVDOLoaderModel::
outData(PortIndex portIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
    {
        if( portIndex == 0 && mpCVImageData->image().data != nullptr )
            result = mpCVImageData;
    }
    return result;
}


QJsonObject
CVVDOLoaderModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();
    if( !msVideoFilename.isEmpty() )
    {
        QJsonObject cParams;
        cParams["filename"] = msVideoFilename;
        cParams["flip_period"] = miFlipPeriodInMillisecond;
        cParams["is_loop"] = mbLoop;
        modelJson["cParams"] = cParams;
    }
    return modelJson;
}

void
CVVDOLoaderModel::
restore( QJsonObject const &p )
{
    PBNodeDataModel::restore( p );

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "flip_period" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "flip_period" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > > (prop);
            typedProp->getData().miValue = v.toInt();
            miFlipPeriodInMillisecond = v.toInt();
        }

        v = paramsObj[ "is_loop" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "is_loop" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
            typedProp->getData() = v.toBool();
            mbLoop = v.toBool();
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
CVVDOLoaderModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

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
        mpVDOThread->set_period( miFlipPeriodInMillisecond );
    }
    else if( id == "is_loop" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbLoop = value.toBool();
        mpVDOThread->set_loop_play( mbLoop );
    }
}

void
CVVDOLoaderModel::
set_video_filename(QString & filename)
{
    if( msVideoFilename == filename )
        return;

    msVideoFilename = filename;
    if( !QFile::exists( msVideoFilename ) )
        return;

    QFileInfo fi( msVideoFilename );
    mpEmbeddedWidget->set_filename( fi.fileName() );
    mpEmbeddedWidget->set_active(true);

    mpVDOThread->set_video_filename(filename);
    mpVDOThread->set_period( miFlipPeriodInMillisecond );
    mpVDOThread->set_loop_play( mbLoop );

    mpEmbeddedWidget->set_maximum_slider( mpVDOThread->get_no_frames() );

    auto prop = mMapIdToProperty["image_size"];
    auto typedPropSize = std::static_pointer_cast<TypedProperty<SizePropertyType>>( prop );
    typedPropSize->getData().miWidth = mpVDOThread->get_image_size().width;
    typedPropSize->getData().miHeight = mpVDOThread->get_image_size().height;
    Q_EMIT property_changed_signal( prop );

    prop = mMapIdToProperty[ "image_format" ];
    auto typedPropFormat = std::static_pointer_cast<TypedProperty<QString>>( prop );
    typedPropFormat->getData() = mpVDOThread->get_image_format();
    Q_EMIT property_changed_signal( prop );
}

void
CVVDOLoaderModel::
em_button_clicked( int button )
{
    if( button == 0 )		// Backward
    {
        mpVDOThread->previous_frame();
    }
    else if( button == 1 )	// Auto Play
    {
        mpVDOThread->resume();
    }
    else if( button == 2 )	// Pause
    {
        mpVDOThread->pause();
    }
    else if( button == 3 )	// Forward
    {
        mpVDOThread->next_frame();
    }
    else if( button == 4 )  // Open File
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
                                                         tr( "Video Files (*.mp4 *.mpg *.wmv *.avi)") );
        if( !filename.isEmpty() )
        {
            auto prop = mMapIdToProperty[ "filename" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
            typedProp->getData().msFilename = filename;

            if( isSelected() )
                Q_EMIT property_changed_signal( prop );
            else
                set_video_filename( filename );
        }
    }
}

void
CVVDOLoaderModel::
frame_changed( int no_frame )
{
    mpVDOThread->set_display_frame( no_frame );
}
/*
void
CVVDOLoaderModel::
enable_changed( bool enable )
{
    if( enable )
        updateAllOutputPorts();
}
*/
const QString CVVDOLoaderModel::_category = QString( "Source" );

const QString CVVDOLoaderModel::_model_name = QString( "CV Video Loader" );
