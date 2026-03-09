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

#include "CVRTSPCameraModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <memory>
#include <vector>
#include <utility>
#include <chrono>
#include <QtWidgets/QFileDialog>
#include <QVariant>
#include "qtvariantproperty_p.h"

const QString CVRTSPCameraModel::_category = QString( "Source" );

const QString CVRTSPCameraModel::_model_name = QString( "CV RTSP Camera" );

// ================ CVRTSPCameraWorker Implementation ================

CVRTSPCameraWorker::
CVRTSPCameraWorker(QObject *parent)
    : QObject(parent),
      mpTimer(new QTimer(this))
{
    connect(mpTimer, &QTimer::timeout, this, &CVRTSPCameraWorker::captureFrame);
}

void
CVRTSPCameraWorker::
setRtspUrl(const QString& url)
{
    if (msRTSPUrl == url)
        return;
    msRTSPUrl = url;
    checkCamera();
}

void
CVRTSPCameraWorker::
setParams(CVRTSPCameraParameters params)
{
    mRTSPCameraParams = params;
    checkCamera();
}

void
CVRTSPCameraWorker::
setSingleShotMode(bool enabled)
{
    mbSingleShotMode = enabled;
    if (mpTimer)
    {
        if (mbSingleShotMode)
            mpTimer->stop();
        else if (mbConnected)
            mpTimer->start(static_cast<int>(miDelayTime));
    }
}

void
CVRTSPCameraWorker::
fireSingleShot()
{
    if (!mbConnected)
        return;
    captureFrame();
}

void
CVRTSPCameraWorker::
captureFrame()
{
    if (!mbConnected)
        return;

    if (!mCVVideoCapture.grab())
        return;
    cv::Mat frame;
    if (!mCVVideoCapture.retrieve(frame))
        return;

    if (!frame.empty())
    {
        Q_EMIT frameCaptured(frame);
        
        // Update FPS
        static auto lastTime = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTime - lastTime;
        if (elapsed.count() > 0.5)
        {
            mdFPS = 1.0 / elapsed.count();
            Q_EMIT fpsUpdated(mdFPS);
            lastTime = currentTime;
        }
    }
}

void
CVRTSPCameraWorker::
checkCamera()
{
    if (mbConnected)
    {
        if (mpTimer)
            mpTimer->stop();
        mCVVideoCapture.release();
        mbConnected = false;
    }
    
    if (!msRTSPUrl.isEmpty())
    {
        try
        {
            // Open RTSP stream - no camera parameters can be set, stream comes pre-encoded
            mCVVideoCapture = cv::VideoCapture(msRTSPUrl.toStdString(), cv::CAP_FFMPEG);
            
            // Apply latency reduction settings
            mCVVideoCapture.set(cv::CAP_PROP_BUFFERSIZE, mRTSPCameraParams.miBufferSize);
            
            if (mCVVideoCapture.isOpened())
            {
                // Get stream FPS for frame timing
                mdFPS = mCVVideoCapture.get(cv::CAP_PROP_FPS);
                qDebug() << "RTSP Stream FPS : " << mdFPS;
                if (mdFPS != 0.)
                    miDelayTime = unsigned(1000. / mdFPS);
                else
                    miDelayTime = 10;
                if (miDelayTime > 15)
                    miDelayTime = miDelayTime - 3;

                int fourcc = static_cast<int>(mCVVideoCapture.get(cv::CAP_PROP_FOURCC));
                char fourcc_str[] = {
                    (char)(fourcc & 0XFF),
                    (char)((fourcc & 0XFF00) >> 8),
                    (char)((fourcc & 0XFF0000) >> 16),
                    (char)((fourcc & 0XFF000000) >> 24),
                    '\0'
                };
                qDebug() << "RTSP Stream format (FourCC): " << QString(fourcc_str) << " " << fourcc;
                
                mbConnected = true;
                Q_EMIT cameraReady(true);
                Q_EMIT fpsUpdated(mdFPS);
                
                // Start timer for capture if not in single-shot mode
                if (!mbSingleShotMode && mpTimer)
                    mpTimer->start(static_cast<int>(miDelayTime));
            }
            else
            {
                Q_EMIT cameraReady(false);
                mbConnected = false;
            }
        }
        catch (cv::Exception &e)
        {
            qDebug() << "RTSP connection error: " << e.what();
            Q_EMIT cameraReady(false);
        }
    }
    else
    {
        Q_EMIT cameraReady(false);
    }
}

// ================ CVRTSPCameraModel Implementation ================

CVRTSPCameraModel::
CVRTSPCameraModel()
    : PBAsyncDataModel( _model_name, true ),
        mpEmbeddedWidget( new CVRTSPCameraEmbeddedWidget( qobject_cast<QWidget *>(this) ) ),
        mMinPixmap(":/RTSPCamera.png")
{
    qRegisterMetaType<cv::Mat>( "cv::Mat" );
    connect( mpEmbeddedWidget, &CVRTSPCameraEmbeddedWidget::button_clicked_signal, this, &CVRTSPCameraModel::em_button_clicked );
    //There are two interactive methods for an embeeded widget.
    //The first method is calling the following line and mpEmbeddedWidget->set_active must not be called again.
    //setEnable and enable_changed functions must be called explicitly in em_button_clicked slot.
    //The embedded widget will always accept a mouse interaction.
    mpEmbeddedWidget->set_active( true );

    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpInformationData = std::make_shared< InformationData >( );
    mpSyncData = std::make_shared< SyncData >();

    // RTSP URL string property
    QString propId = "rtsp_url";
    auto propRTSPUrl = std::make_shared< TypedProperty< QString > >("RTSP URL", propId, QMetaType::QString, mCameraProperty.msRTSPUrl);
    mvProperty.push_back( propRTSPUrl );
    mMapIdToProperty[ propId ] = propRTSPUrl;

    // Latency reduction properties
    IntPropertyType bufferType;
    bufferType.miMin = 1;
    bufferType.miMax = 10;
    bufferType.miValue = miBufferSize;
    propId = "buffer_size";
    auto propBuffer = std::make_shared< TypedProperty< IntPropertyType > >("Buffer Size (1=Low Latency)", propId, QMetaType::Int, bufferType);
    mvProperty.push_back( propBuffer );
    mMapIdToProperty[ propId ] = propBuffer;

    IntPropertyType timeoutType;
    timeoutType.miMin = 1000;
    timeoutType.miMax = 30000;
    timeoutType.miValue = miConnectTimeoutMs;
    propId = "connect_timeout_ms";
    auto propConnectTimeout = std::make_shared< TypedProperty< IntPropertyType > >("Connection Timeout (ms)", propId, QMetaType::Int, timeoutType);
    mvProperty.push_back( propConnectTimeout );
    mMapIdToProperty[ propId ] = propConnectTimeout;

    timeoutType.miValue = miReadTimeoutMs;
    propId = "read_timeout_ms";
    auto propReadTimeout = std::make_shared< TypedProperty< IntPropertyType > >("Read Timeout (ms)", propId, QMetaType::Int, timeoutType);
    mvProperty.push_back( propReadTimeout );
    mMapIdToProperty[ propId ] = propReadTimeout;

    // RTSP streams come pre-encoded - format, resolution, exposure, etc. are controlled on the camera side
    // No client-side properties needed for these
}

CVRTSPCameraModel::
~CVRTSPCameraModel()
{
    // Worker lifecycle handled by PBAsyncDataModel
}

QObject*
CVRTSPCameraModel::
createWorker()
{
    mpCameraWorker = new CVRTSPCameraWorker();
    return mpCameraWorker;
}

void
CVRTSPCameraModel::
connectWorker(QObject* worker)
{
    auto *cameraWorker = qobject_cast<CVRTSPCameraWorker*>(worker);
    if (!cameraWorker)
        return;

    // Connect worker signals to model slots
    connect(cameraWorker, &CVRTSPCameraWorker::frameCaptured,
            this, &CVRTSPCameraModel::process_captured_frame, Qt::QueuedConnection);
    connect(cameraWorker, &CVRTSPCameraWorker::cameraReady,
            this, &CVRTSPCameraModel::camera_status_changed, Qt::QueuedConnection);
    connect(cameraWorker, &CVRTSPCameraWorker::cameraReady,
            mpEmbeddedWidget, &CVRTSPCameraEmbeddedWidget::camera_status_changed, Qt::QueuedConnection);
    connect(cameraWorker, &CVRTSPCameraWorker::fpsUpdated,
            this, [this](double fps) { mdLastFps = fps; }, Qt::QueuedConnection);
}

void
CVRTSPCameraModel::
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
    if( getSharingMode() == FrameSharingMode::PoolMode )
    {
        ensure_frame_pool( frame.cols, frame.rows, frame.type() );
        auto poolCopy = getFramePool();
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
    {
        if( mpSyncData )
            mpSyncData->data() = true;
        updateAllOutputPorts();
    }
}

void
CVRTSPCameraModel::
camera_status_changed( bool status )
{
    mCameraProperty.mbCameraStatus = status;
}

unsigned int
CVRTSPCameraModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 1 );
    case PortType::Out:
        return( 3 );
    default:
        return( 0 );
    }
}

NodeDataType
CVRTSPCameraModel::
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
        else if( portIndex == 2 )
            return SyncData().type();
        else
            return NodeDataType();
    }
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
CVRTSPCameraModel::
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
            sInformation += currentTime + "FPS : " + QString::number(mdLastFps) + "\n";
            sInformation += currentTime + "Width x Height : " + QString::number( image.cols ) + " x " + QString::number( image.rows );
            mpInformationData->set_information( sInformation );
            result = mpInformationData;
        }
        else if( portIndex == 2 )
            result = mpSyncData;
    }
    return result;
}

void
CVRTSPCameraModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if(nodeData)
    {
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
        if( d )
        {
            if( mpCameraWorker )
                QMetaObject::invokeMethod(mpCameraWorker, "fireSingleShot", Qt::QueuedConnection);
        }
    }
}

void
CVRTSPCameraModel::
inputConnectionCreated(const QtNodes::ConnectionId& connection)
{
    Q_UNUSED(connection);
    if( mpCameraWorker )
        QMetaObject::invokeMethod(mpCameraWorker, "setSingleShotMode", Qt::QueuedConnection, Q_ARG(bool, true));
}

void
CVRTSPCameraModel::
inputConnectionDeleted(const QtNodes::ConnectionId& connection)
{
    Q_UNUSED(connection);
    if( mpCameraWorker )
        QMetaObject::invokeMethod(mpCameraWorker, "setSingleShotMode", Qt::QueuedConnection, Q_ARG(bool, false));
}

QJsonObject
CVRTSPCameraModel::
save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams = modelJson[ "cParams" ].toObject();
    cParams[ "rtsp_url" ] = mCameraProperty.msRTSPUrl;
    cParams[ "buffer_size" ] = miBufferSize;
    cParams[ "connect_timeout_ms" ] = miConnectTimeoutMs;
    cParams[ "read_timeout_ms" ] = miReadTimeoutMs;
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
CVRTSPCameraModel::
load(const QJsonObject &p)
{
    PBAsyncDataModel::load(p);
    QJsonObject paramsObj = p[ "cParams" ].toObject();
    const bool restoringSavedState = !paramsObj.isEmpty();
    mAutoRefreshAllowed = !restoringSavedState;
    late_constructor();

    if( !restoringSavedState )
        return;

    QJsonValue v = paramsObj[ "rtsp_url" ];
    if( !v.isNull() )
    {
        mCameraProperty.msRTSPUrl = v.toString();
    }

    // Populate properties with saved values
    auto urlProp = mMapIdToProperty[ "rtsp_url" ];
    auto typedUrlProp = std::static_pointer_cast< TypedProperty< QString> >( urlProp );
    typedUrlProp->getData() = mCameraProperty.msRTSPUrl;

    // Restore latency settings
    QJsonValue bufVal = paramsObj[ "buffer_size" ];
    if( !bufVal.isNull() )
    {
        miBufferSize = bufVal.toInt();
        mRTSPCameraParams.miBufferSize = miBufferSize;
        auto bufProp = mMapIdToProperty[ "buffer_size" ];
        auto typedBufProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( bufProp );
        typedBufProp->getData().miValue = miBufferSize;
    }

    QJsonValue connVal = paramsObj[ "connect_timeout_ms" ];
    if( !connVal.isNull() )
    {
        miConnectTimeoutMs = connVal.toInt();
        mRTSPCameraParams.miConnectTimeoutMs = miConnectTimeoutMs;
        auto connProp = mMapIdToProperty[ "connect_timeout_ms" ];
        auto typedConnProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( connProp );
        typedConnProp->getData().miValue = miConnectTimeoutMs;
    }

    QJsonValue readVal = paramsObj[ "read_timeout_ms" ];
    if( !readVal.isNull() )
    {
        miReadTimeoutMs = readVal.toInt();
        mRTSPCameraParams.miReadTimeoutMs = miReadTimeoutMs;
        auto readProp = mMapIdToProperty[ "read_timeout_ms" ];
        auto typedReadProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( readProp );
        typedReadProp->getData().miValue = miReadTimeoutMs;
    }

    mpEmbeddedWidget->set_camera_property( mCameraProperty );
    if( isEnable() && mpCameraWorker )
        QMetaObject::invokeMethod(mpCameraWorker, "setRtspUrl", Qt::QueuedConnection, Q_ARG(QString, mCameraProperty.msRTSPUrl));

    Q_EMIT property_structure_changed_signal();
}

void
CVRTSPCameraModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBAsyncDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "rtsp_url" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();

        mCameraProperty.msRTSPUrl = value.toString();
        mpEmbeddedWidget->set_camera_property( mCameraProperty );
        if( isEnable() && mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setRtspUrl", Qt::QueuedConnection, Q_ARG(QString, mCameraProperty.msRTSPUrl));
    }
    else if( id == "buffer_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miBufferSize = value.toInt();
        typedProp->getData().miValue = miBufferSize;
        mRTSPCameraParams.miBufferSize = miBufferSize;
        
        // Apply buffer size to VideoCapture
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVRTSPCameraParameters, mRTSPCameraParams));
    }
    else if( id == "connect_timeout_ms" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miConnectTimeoutMs = value.toInt();
        typedProp->getData().miValue = miConnectTimeoutMs;
        mRTSPCameraParams.miConnectTimeoutMs = miConnectTimeoutMs;
        
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVRTSPCameraParameters, mRTSPCameraParams));
    }
    else if( id == "read_timeout_ms" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miReadTimeoutMs = value.toInt();
        typedProp->getData().miValue = miReadTimeoutMs;
        mRTSPCameraParams.miReadTimeoutMs = miReadTimeoutMs;
        
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVRTSPCameraParameters, mRTSPCameraParams));
    }
    // RTSP streams come pre-encoded - no client-side properties for format, resolution, brightness, etc.
}

void
CVRTSPCameraModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed( enable );

    mpEmbeddedWidget->set_ready_state( !enable );
    if( mpCameraWorker )
    {
        if( enable )
        {
            mCameraProperty.msRTSPUrl = mpEmbeddedWidget->get_camera_property().msRTSPUrl;
            QMetaObject::invokeMethod(mpCameraWorker, "setRtspUrl", Qt::QueuedConnection, Q_ARG(QString, mCameraProperty.msRTSPUrl));
        }
        else
            QMetaObject::invokeMethod(mpCameraWorker, "setRtspUrl", Qt::QueuedConnection, Q_ARG(QString, QString()));
    }
}

void
CVRTSPCameraModel::
late_constructor()
{
    start_late_constructor();
}

void
CVRTSPCameraModel::
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
    
    // Button 3 removed (no refresh needed for RTSP)

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
            DEBUG_LOG_INFO() << "[em_button_clicked] Update RTSP URL";
            prop = mMapIdToProperty[ "rtsp_url" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = mpEmbeddedWidget->get_camera_property().msRTSPUrl;
            Q_EMIT property_changed_signal( prop );

            mCameraProperty.msRTSPUrl = mpEmbeddedWidget->get_camera_property().msRTSPUrl;
            if( isEnable() && mpCameraWorker )
                QMetaObject::invokeMethod(mpCameraWorker, "setRtspUrl", Qt::QueuedConnection, Q_ARG(QString, mCameraProperty.msRTSPUrl));
        }
    }
    
    Q_EMIT embeddedWidgetSizeUpdated();
}

void
CVRTSPCameraModel::
setSelected( bool selected )
{
    PBNodeDelegateModel::setSelected( selected );
    //The second method is to uncomment the following line and comment mpEmbeddedWidget->set_active above.
    //The embedded widget will not accept any mouse interaction unless the node is selected.
    //mpEmbeddedWidget->set_active( selected );
}

template <typename T>
void
CVRTSPCameraModel::set_property_read_only(const QString &id, bool readOnly)
{
    auto it = mMapIdToProperty.find( id );
    if( it == mMapIdToProperty.end() )
        return;

    auto prop = it.value();
    auto typedProp = std::static_pointer_cast< TypedProperty< T > >( prop );
    if( typedProp->isReadOnly() == readOnly )
        return;

    typedProp->setReadOnly( readOnly );
    Q_EMIT property_changed_signal( prop );
}

void
CVRTSPCameraModel::
refresh_camera_capabilities()
{
    // RTSP streams come pre-encoded - no client-side configuration needed
    Q_EMIT property_structure_changed_signal();
}

void
CVRTSPCameraModel::
refresh_resolutions()
{
    // RTSP streams come pre-encoded - resolution is whatever the stream provides
    Q_EMIT property_structure_changed_signal();
}

void
CVRTSPCameraModel::
refresh_camera_list()
{
    // RTSP doesn't enumerate cameras - URL is set manually
}

// Helper template functions
