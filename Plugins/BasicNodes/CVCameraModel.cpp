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

#include "CVCameraModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtWidgets/QFileDialog>
#include <QVariant>
#include "qtvariantproperty_p.h"

const QString CVCameraModel::_category = QString( "Source" );

const QString CVCameraModel::_model_name = QString( "CV Camera" );

CVCameraThread::CVCameraThread(CVCameraModel *model)
    : QThread(model), mpModel(model)
{

}

CVCameraThread::
~CVCameraThread()
{
    mbAbort = true;
    mSingleShotSemaphore.release();
    wait();
}

void
CVCameraThread::
run()
{
    while(!mbAbort)
    {
        if( mCameraCheckSemaphore.tryAcquire() )
            check_camera();
        if( mbConnected )
        {
            if( mbSingleShotMode.load() )
            {
                mSingleShotSemaphore.acquire();
                
                cv::Mat frame;
                mCVVideoCapture >> frame;
                if( !frame.empty() )
                {
                    long frameId = mFrameCounter.fetch_add(1, std::memory_order_relaxed);
                    Q_EMIT frame_captured( frame );
                }
                else
                    mCVVideoCapture.set(cv::CAP_PROP_POS_FRAMES, -1);
            }
            else
            {
                if( !mCVVideoCapture.grab() )
                    continue;
                cv::Mat frame;
                if( !mCVVideoCapture.retrieve( frame ) )
                    continue;
                if( !frame.empty() )
                {
                    long frameId = mFrameCounter.fetch_add(1, std::memory_order_relaxed);
                    Q_EMIT frame_captured( frame );
                }
            }
        }
        QThread::msleep( miDelayTime );
   }
}

void
CVCameraThread::
set_camera_id(int camera_id)
{
    if( miCameraID != camera_id )
    {
        miCameraID = camera_id;
        mCameraCheckSemaphore.release();
        if( mbSingleShotMode.load() )
            mSingleShotSemaphore.release();
        if( !isRunning() )
            start();
    }
}

void
CVCameraThread::
set_params( CVCameraParameters & params )
{
    mCameraParams = params;
    mCameraCheckSemaphore.release();
    if( mbSingleShotMode.load() )
        mSingleShotSemaphore.release();
}

// need to move this function to run in a thread, eg. run function, otherwise it will block a main GUI loop a bit.
void
CVCameraThread::
check_camera()
{
    if( mbConnected )
    {
        mCVVideoCapture.release();
        mbConnected = false;
    }
    if( miCameraID != -1 )
    {
        try
        {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
            mCVVideoCapture = cv::VideoCapture(miCameraID, cv::CAP_DSHOW );
#elif defined(__APPLE__)
            mCVVideoCapture = cv::VideoCapture(miCameraID);
#elif defined(__linux__)
            mCVVideoCapture = cv::VideoCapture(miCameraID, cv::CAP_V4L2);
#endif
            if( mCVVideoCapture.isOpened() )
            {
                mCVVideoCapture.set(cv::CAP_PROP_FOURCC, mCameraParams.miFourCC);
                mCVVideoCapture.set(cv::CAP_PROP_FPS, mCameraParams.miFps);
                mCVVideoCapture.set(cv::CAP_PROP_FRAME_WIDTH, mCameraParams.miWidth);
                mCVVideoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, mCameraParams.miHeight);
                mCVVideoCapture.set(cv::CAP_PROP_AUTO_WB, mCameraParams.miAutoWB);
                //mCVVideoCapture.set(cv::CAP_PROP_WB_TEMPERATURE, 6000);
                mdFPS = mCVVideoCapture.get(cv::CAP_PROP_FPS);
                qDebug() << "Config FPS : " << mdFPS;
                if( mdFPS != 0. )
                    miDelayTime = unsigned(1000./mdFPS);
                else
                    miDelayTime = 10;
                if(miDelayTime > 15 )
                    miDelayTime =  miDelayTime - 3;

                int fourcc = static_cast<int>(mCVVideoCapture.get(cv::CAP_PROP_FOURCC));

                // Convert the FourCC code to a readable string
                char fourcc_str[] = {
                (char)(fourcc & 0XFF),
                (char)((fourcc & 0XFF00) >> 8),
                (char)((fourcc & 0XFF0000) >> 16),
                (char)((fourcc & 0XFF000000) >> 24),
                '\0'
                };
    
                qDebug() << "Camera output format (FourCC): " << QString(fourcc_str) << " " << fourcc;
#if defined (__linux__)
                if(!mCVVideoCapture.set(cv::CAP_PROP_BRIGHTNESS, mCameraParams.miBrightness))
                {
                    qDebug() << "Error: Brightness!";
                    double value = mCVVideoCapture.get(cv::CAP_PROP_BRIGHTNESS);
                    mCameraParams.miBrightness = value;
                    qDebug() << "Get brightness : " << value;
                }
                else
                {
                    double value = mCVVideoCapture.get(cv::CAP_PROP_BRIGHTNESS);
                    mCameraParams.miBrightness = value;
                    qDebug() << "Get brightness : " << value;
                }
                if(!mCVVideoCapture.set(cv::CAP_PROP_AUTO_EXPOSURE, mCameraParams.miAutoExposure))
                {
                    qDebug() << "Error: Auto Exposure!";
                    double value = mCVVideoCapture.get(cv::CAP_PROP_AUTO_EXPOSURE);
                    mCameraParams.miAutoExposure = value;
                    qDebug() << "Get Auto Exposure: " << value;
                }
                else
                {
                    double value = mCVVideoCapture.get(cv::CAP_PROP_AUTO_EXPOSURE);
                    mCameraParams.miAutoExposure = value;
                    qDebug() << "Get Auto Exposure: " << value;
                }
                if( mCameraParams.miAutoExposure == 1 ) // Manual Exposure
                {
                    if(!mCVVideoCapture.set(cv::CAP_PROP_GAIN, mCameraParams.miGain))
                    {
                        qDebug() << "Error: gain!";
                        double value = mCVVideoCapture.get(cv::CAP_PROP_GAIN);
                        mCameraParams.miGain = value;
                        qDebug() << "Get gain : " << value;
                    }
                    else
                    {
                        double value = mCVVideoCapture.get(cv::CAP_PROP_GAIN);
                        mCameraParams.miGain = value;
                        qDebug() << "Get gain : " << value;
                    }
                    if(!mCVVideoCapture.set(cv::CAP_PROP_EXPOSURE, mCameraParams.miExposure))
                    {
                        qDebug() << "Error: Exposure!";
                        double value = mCVVideoCapture.get(cv::CAP_PROP_EXPOSURE);
                        mCameraParams.miExposure = value;
                        qDebug() << "Get Exposure: " << value;
                    }
                    else
                    {
                        double value = mCVVideoCapture.get(cv::CAP_PROP_EXPOSURE);
                        mCameraParams.miExposure = value;
                        qDebug() << "Get Exposure: " << value;
                    }
                }
#endif
                mbConnected = true;
                Q_EMIT camera_ready(true);
            }
            else
            {
                Q_EMIT camera_ready(false);
                mbConnected = false;
            }
        }
        catch ( cv::Exception &e) {
            qDebug() << e.what();
            Q_EMIT camera_ready( false );
        }
    }
    else
        Q_EMIT camera_ready( false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CVCameraModel::
CVCameraModel()
    : PBNodeDelegateModel( _model_name, true ),
      mpEmbeddedWidget( new CVCameraEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat" );
    connect( mpEmbeddedWidget, &CVCameraEmbeddedWidget::button_clicked_signal, this, &CVCameraModel::em_button_clicked );
    //There are two interactive methods for an embeeded widget.
    //The first method is calling the following line and mpEmbeddedWidget->set_active must not be called again.
    //setEnable and enable_changed functions must be called explicitly in em_button_clicked slot.
    //The embedded widget will always accept a mouse interaction.
    mpEmbeddedWidget->set_active( true );

    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpInformationData = std::make_shared< InformationData >( );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList( {"0", "1", "2", "3", "4"} );
    enumPropertyType.miCurrentIndex = enumPropertyType.mslEnumNames.indexOf( QString::number( mCameraProperty.miCameraID ) );
    QString propId = "camera_id";
    auto propCameraID = std::make_shared< TypedProperty< EnumPropertyType > >("Camera ID", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propCameraID );
    mMapIdToProperty[ propId ] = propCameraID;

    IntPropertyType intPropertyType;
    intPropertyType.miMax = 64;
    intPropertyType.miMin = -64;
    intPropertyType.miValue = -10;
    propId = "brightness";
    auto propBrightness = std::make_shared< TypedProperty< IntPropertyType > >("Brightness", propId, QMetaType::Int, intPropertyType);
    mvProperty.push_back( propBrightness );
    mMapIdToProperty[ propId ] = propBrightness;

    propId = "auto_wb";
    auto propAutoWB = std::make_shared< TypedProperty< bool > >("Auto White Balance", propId, QMetaType::Bool, false);
    mvProperty.push_back( propAutoWB );
    mMapIdToProperty[ propId ] = propAutoWB;

    propId = "auto_exposure";
    auto propAutoExposure = std::make_shared< TypedProperty< bool > >("Auto Exposure", propId, QMetaType::Bool, false);
    mvProperty.push_back( propAutoExposure );
    mMapIdToProperty[ propId ] = propAutoExposure;

    intPropertyType.miMax = 5000;
    intPropertyType.miMin = 1;
    intPropertyType.miValue = 2000;
    propId = "exposure";
    auto propExposure = std::make_shared< TypedProperty< IntPropertyType > >("Exposure(1/s)", propId, QMetaType::Int, intPropertyType);
    mvProperty.push_back( propExposure );
    mMapIdToProperty[ propId ] = propExposure;

    intPropertyType.miMax = 100;
    intPropertyType.miMin = 1;
    intPropertyType.miValue = 70;
    propId = "gain";
    auto propGain = std::make_shared< TypedProperty< IntPropertyType > >("Gain", propId, QMetaType::Int, intPropertyType);
    mvProperty.push_back( propGain );
    mMapIdToProperty[ propId ] = propGain;
}

CVCameraModel::
~CVCameraModel()
{
    mShuttingDown.store(true, std::memory_order_release);
    
    if( mpCVCameraThread )
    {
        // Disconnect signals FIRST to prevent callbacks during destruction
        disconnect( mpCVCameraThread, nullptr, this, nullptr );
        // The mpCVCameraThread is a child of this model, so it will be deleted automatically. 
        // delete mpCVCameraThread;
        // mpCVCameraThread = nullptr;
    }
}

void
CVCameraModel::
process_captured_frame( cv::Mat frame )
{
    if( frame.empty() || isShuttingDown() )
        return;

    FrameMetadata metadata;
    metadata.producerId = getNodeId();
    metadata.frameId = 0; // Frame counter managed by thread

    // Create a fresh CVImageData per frame
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

    // Emit data update
    if( isEnable() )
        updateAllOutputPorts();
}

void
CVCameraModel::
camera_status_changed( bool status )
{
    mCameraProperty.mbCameraStatus = status;
}

unsigned int
CVCameraModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 1 );
    case PortType::Out:
        return( 2 );
    default:
        return( 0 );
    }
}

NodeDataType
CVCameraModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if(portType == PortType::In)
    {
        return SyncData().type();
    }
    else if( portType == PortType::Out )
    {
        if( portIndex == 0 )
            return CVImageData().type();
        else if( portIndex == 1 )
            return InformationData().type();
        else
            return NodeDataType();
    }
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
CVCameraModel::
outData(PortIndex portIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() && mpCVImageData->data().data != nullptr )
    {
        if( portIndex == 0 )
            result = mpCVImageData;
        else if( portIndex == 1 )
        {
            QString currentTime = QTime::currentTime().toString( "hh:mm:ss.zzz" ) + " :: ";
            QString sInformation = "\n";
            cv::Mat image = mpCVImageData->data();
            if( image.channels() == 1 )
                sInformation += currentTime + "Image Type : Gray\n" + currentTime + "Image Format : CV_8UC1\n";
            else if( image.channels() == 3 )
                sInformation += currentTime + "Image Type : Color\n" + currentTime + "Image Format : CV_8UC3\n";
            sInformation += currentTime + "FPS : " + QString::number(mpCVCameraThread->get_fps()) + "\n";
            sInformation += currentTime + "Width x Height : " + QString::number( image.cols ) + " x " + QString::number( image.rows );
            mpInformationData->set_information( sInformation );
            result = mpInformationData;
        }
    }
    return result;
}

void
CVCameraModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if(nodeData)
    {
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
        if( d )
            mpCVCameraThread->fire_single_shot();
    }
}

QJsonObject
CVCameraModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "camera_id" ] = mCameraProperty.miCameraID;
    cParams[ "brightness" ] = miBrightness;
    cParams[ "gain" ] = miGain;
    cParams[ "exposure" ] = miExposure;
    cParams[ "auto_exposure" ] = mbAutoExposure;
    cParams[ "auto_wb" ] = mbAutoWB;
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
CVCameraModel::
load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);
    late_constructor();

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "camera_id" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "camera_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType> >( prop );
            typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( v.toString() );

            mCameraProperty.miCameraID = v.toInt();
        }

        mpEmbeddedWidget->set_camera_property( mCameraProperty );
        if( isEnable() )
            mpCVCameraThread->set_camera_id( mCameraProperty.miCameraID );

        CVCameraParameters params;
        v = paramsObj["brightness"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "brightness"];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            miBrightness = v.toInt();
            typedProp->getData().miValue = miBrightness;
            params.miBrightness = miBrightness;
        }

        v = paramsObj["gain"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "gain" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            miGain = v.toInt();
            typedProp->getData().miValue = miGain;
            params.miGain = miGain;
        }

        v = paramsObj[ "auto_wb" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "auto_wb" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            mbAutoWB = v.toBool();
            typedProp->getData() = mbAutoWB;
            params.miAutoWB = mbAutoWB ? 1 : 0;
        }

        v = paramsObj[ "auto_exposure" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "auto_exposure" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            mbAutoExposure = v.toBool();
            typedProp->getData() = mbAutoExposure;
            params.miAutoExposure = mbAutoExposure ? 3 : 1;
        }

        v = paramsObj[" exposure" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "exposure" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            miExposure = v.toInt();
            typedProp->getData().miValue = miExposure;
            params.miExposure = miExposure;

        }
        mpCVCameraThread->set_params( params );
    }
}

void
CVCameraModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "camera_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( value.toString() );

        mCameraProperty.miCameraID = value.toInt();
        mpEmbeddedWidget->set_camera_property( mCameraProperty );
        if( isEnable() )
            mpCVCameraThread->set_camera_id( mCameraProperty.miCameraID );
    }
    else if( id == "brightness" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miBrightness = value.toInt();
        typedProp->getData().miValue = miBrightness;

        CVCameraParameters params = mpCVCameraThread->get_params();
        params.miBrightness = miBrightness;
        mpCVCameraThread->set_params( params );
    }
    else if( id == "gain" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miGain = value.toInt();
        typedProp->getData().miValue = miGain;

        CVCameraParameters params = mpCVCameraThread->get_params();
        params.miGain = miGain;
        mpCVCameraThread->set_params( params );
    }
    else if( id == "auto_wb" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        mbAutoWB = value.toBool();
        typedProp->getData() = mbAutoWB;

        CVCameraParameters params = mpCVCameraThread->get_params();
        params.miAutoWB = mbAutoWB ? 1 : 0;
        mpCVCameraThread->set_params( params );
    }
    else if( id == "auto_exposure" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        mbAutoExposure = value.toBool();
        typedProp->getData() = mbAutoExposure;

        CVCameraParameters params = mpCVCameraThread->get_params();
        params.miAutoExposure = mbAutoExposure ? 3 : 1;
        mpCVCameraThread->set_params( params );
    }
    else if( id == "exposure" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miExposure = value.toInt();
        typedProp->getData().miValue = miExposure;

        CVCameraParameters params = mpCVCameraThread->get_params();
        params.miExposure = miExposure;
        mpCVCameraThread->set_params( params );
    }
}

void
CVCameraModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed( enable );

    mpEmbeddedWidget->set_ready_state( !enable );
    if( enable )
    {
        mCameraProperty.miCameraID = mpEmbeddedWidget->get_camera_property().miCameraID;
        mpCVCameraThread->set_camera_id(mCameraProperty.miCameraID);
    }
    else
        mpCVCameraThread->set_camera_id(-1);
}

void
CVCameraModel::
late_constructor()
{
    if( start_late_constructor() )
    {
        mpCVCameraThread = new CVCameraThread(this);
        connect( mpCVCameraThread, &CVCameraThread::frame_captured, this, &CVCameraModel::process_captured_frame );
        connect( mpCVCameraThread, &CVCameraThread::camera_ready, this, &CVCameraModel::camera_status_changed );
        connect( mpCVCameraThread, &CVCameraThread::camera_ready, mpEmbeddedWidget, &CVCameraEmbeddedWidget::camera_status_changed );
    }
}

void
CVCameraModel::
em_button_clicked( int button )
{
    DEBUG_LOG_INFO() << "[em_button_clicked] button:" << button << "isSelected:" << isSelected();
    
    // If node is not selected, select it first and block the interaction
    // User needs to click again when node is selected to perform the action
    if (!isSelected())
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Node not selected, requesting selection";
        
        // Restore the widget state to reflect the actual enable state
        // since the button handlers already changed it before emitting the signal
        auto prop = mMapIdToProperty[ "enable" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        bool currentEnableState = typedProp->getData();
        mpEmbeddedWidget->set_ready_state( !currentEnableState );
        
        Q_EMIT selection_request_signal();
        return;
    }
    
    if( button == 0 ) //Start
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Start button";
        auto prop = mMapIdToProperty[ "enable" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = true;
        Q_EMIT property_changed_signal( prop );

        enable_changed( true );
    }
    else //Stop
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Stop button";
        auto prop = mMapIdToProperty[ "enable" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = false;
        Q_EMIT property_changed_signal( prop );

        enable_changed( false );

        if( button == 2 )
        {
            DEBUG_LOG_INFO() << "[em_button_clicked] Update camera ID";
            prop = mMapIdToProperty[ "camera_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( QString::number( mpEmbeddedWidget->get_camera_property().miCameraID ) );
            Q_EMIT property_changed_signal( prop );
        }
    }
    
    Q_EMIT embeddedWidgetSizeUpdated();
}

void
CVCameraModel::
setSelected( bool selected )
{
    PBNodeDelegateModel::setSelected( selected );
    //The second method is to uncomment the following line and comment mpEmbeddedWidget->set_active above.
    //The embedded widget will not accept any mouse interaction unless the node is selected.
    //mpEmbeddedWidget->set_active( selected );
}

void
CVCameraModel::
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
CVCameraModel::
reset_frame_pool()
{
    QMutexLocker locker( &mFramePoolMutex );
    mpFramePool.reset();
    miPoolFrameWidth = 0;
    miPoolFrameHeight = 0;
    miActivePoolSize = 0;
}
