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

#include "CVUSBCameraModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <memory>
#include <vector>
#include <utility>
#include <QtWidgets/QFileDialog>
#include <QVariant>
#include "qtvariantproperty_p.h"

const QString CVUSBCameraModel::_category = QString( "Source" );

const QString CVUSBCameraModel::_model_name = QString( "CV USB Camera" );

// ================ CVUSBCameraWorker Implementation ================

CVUSBCameraWorker::
CVUSBCameraWorker(QObject *parent)
    : QObject(parent),
      mpTimer(new QTimer(this))
{
    connect(mpTimer, &QTimer::timeout, this, &CVUSBCameraWorker::captureFrame);
}

void
CVUSBCameraWorker::
setCameraId(int cameraId)
{
    if (miCameraID == cameraId)
        return;
    miCameraID = cameraId;
    checkCamera();
}

void
CVUSBCameraWorker::
setParams(CVUSBCameraParameters params)
{
    mUSBCameraParams = params;
    checkCamera();
}

void
CVUSBCameraWorker::
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
CVUSBCameraWorker::
fireSingleShot()
{
    if (!mbConnected)
        return;
    captureFrame();
}

void
CVUSBCameraWorker::
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
        Q_EMIT frameCaptured(frame);
}

void
CVUSBCameraWorker::
checkCamera()
{
    if (mpTimer)
        mpTimer->stop();

    if (mCVVideoCapture.isOpened())
        mCVVideoCapture.release();

    mbConnected = false;

    if (miCameraID == -1)
    {
        Q_EMIT cameraReady(false);
        return;
    }

    try
    {
        bool autoFocusSupported = true;
        bool autoExposureSupported = true;
        bool autoWbSupported = true;
        bool brightnessSupported = true;
        bool gainSupported = true;
        bool exposureSupported = true;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        mCVVideoCapture = cv::VideoCapture(miCameraID, cv::CAP_DSHOW );
#elif defined(__APPLE__)
        mCVVideoCapture = cv::VideoCapture(miCameraID);
#elif defined(__linux__)
        mCVVideoCapture = cv::VideoCapture(miCameraID, cv::CAP_V4L2);
#endif
        if( mCVVideoCapture.isOpened() )
        {
            mCVVideoCapture.set(cv::CAP_PROP_FOURCC, mUSBCameraParams.miFourCC);
            mCVVideoCapture.set(cv::CAP_PROP_FPS, mUSBCameraParams.miFps);
            mCVVideoCapture.set(cv::CAP_PROP_FRAME_WIDTH, mUSBCameraParams.miWidth);
            mCVVideoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, mUSBCameraParams.miHeight);
            autoWbSupported = mCVVideoCapture.set(cv::CAP_PROP_AUTO_WB, mUSBCameraParams.miAutoWB);

            autoFocusSupported = mCVVideoCapture.set(cv::CAP_PROP_AUTOFOCUS, mUSBCameraParams.miAutoFocus);

            mdFPS = mCVVideoCapture.get(cv::CAP_PROP_FPS);
            if( mdFPS != 0. )
                miDelayTime = unsigned(1000./mdFPS);
            else
                miDelayTime = 10;
            if(miDelayTime > 15 )
                miDelayTime =  miDelayTime - 3;

#if defined (__linux__)
            brightnessSupported = mCVVideoCapture.set(cv::CAP_PROP_BRIGHTNESS, mUSBCameraParams.miBrightness);
            autoExposureSupported = mCVVideoCapture.set(cv::CAP_PROP_AUTO_EXPOSURE, mUSBCameraParams.miAutoExposure);
            if( mUSBCameraParams.miAutoExposure == 1 ) // Manual Exposure
            {
                gainSupported = mCVVideoCapture.set(cv::CAP_PROP_GAIN, mUSBCameraParams.miGain);
                exposureSupported = mCVVideoCapture.set(cv::CAP_PROP_EXPOSURE, mUSBCameraParams.miExposure);
            }
#endif

            Q_EMIT capabilitiesDetected(autoFocusSupported, autoExposureSupported, autoWbSupported,
                                        brightnessSupported, gainSupported, exposureSupported);
            Q_EMIT fpsUpdated(mdFPS);

            mbConnected = true;
            Q_EMIT cameraReady(true);

            if (!mbSingleShotMode && mpTimer)
                mpTimer->start(static_cast<int>(miDelayTime));
        }
        else
        {
            Q_EMIT capabilitiesDetected(true, true, true, true, true, true);
            Q_EMIT cameraReady(false);
            mbConnected = false;
        }
    }
    catch ( cv::Exception &e) {
        qDebug() << e.what();
        Q_EMIT cameraReady( false );
    }
}

CVUSBCameraModel::
CVUSBCameraModel()
    : PBAsyncDataModel( _model_name, true ),
        mpEmbeddedWidget( new CVUSBCameraEmbeddedWidget( qobject_cast<QWidget *>(this) ) ),
        mMinPixmap(":/USBCamera.png")
{
    qRegisterMetaType<cv::Mat>( "cv::Mat" );
    connect( mpEmbeddedWidget, &CVUSBCameraEmbeddedWidget::button_clicked_signal, this, &CVUSBCameraModel::em_button_clicked );
    //There are two interactive methods for an embeeded widget.
    //The first method is calling the following line and mpEmbeddedWidget->set_active must not be called again.
    //setEnable and enable_changed functions must be called explicitly in em_button_clicked slot.
    //The embedded widget will always accept a mouse interaction.
    mpEmbeddedWidget->set_active( true );

    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpInformationData = std::make_shared< InformationData >( );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList();
    enumPropertyType.miCurrentIndex = enumPropertyType.mslEnumNames.indexOf( QString::number( mCVUSBCameraProperty.miCameraID ) );
    QString propId = "camera_id";
    auto propCameraID = std::make_shared< TypedProperty< EnumPropertyType > >("Camera ID", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propCameraID );
    mMapIdToProperty[ propId ] = propCameraID;

    EnumPropertyType resEnum;
    resEnum.mslEnumNames = QStringList();
    resEnum.miCurrentIndex = 0;
    propId = "resolution";
    auto propResolution = std::make_shared< TypedProperty< EnumPropertyType > >("Resolution", propId, QtVariantPropertyManager::enumTypeId(), resEnum);
    mvProperty.push_back( propResolution );
    mMapIdToProperty[ propId ] = propResolution;

    EnumPropertyType formatEnum;
    formatEnum.mslEnumNames = QStringList() << "MJPG" << "YUYV" << "H264" << "I420" << "YV12" << "RGB3";
    formatEnum.miCurrentIndex = 0; // Default to MJPG
    propId = "image_format";
    auto propImageFormat = std::make_shared< TypedProperty< EnumPropertyType > >("Image Format", propId, QtVariantPropertyManager::enumTypeId(), formatEnum);
    mvProperty.push_back( propImageFormat );
    mMapIdToProperty[ propId ] = propImageFormat;

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

    propId = "auto_focus";
    auto propAutoFocus = std::make_shared< TypedProperty< bool > >("Auto Focus", propId, QMetaType::Bool, true);
    mvProperty.push_back( propAutoFocus );
    mMapIdToProperty[ propId ] = propAutoFocus;

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

    mCurrentResolution = make_resolution_string( mUSBCameraParams.miWidth, mUSBCameraParams.miHeight );
}

CVUSBCameraModel::
~CVUSBCameraModel()
{
    // Worker lifecycle handled by PBAsyncDataModel
}

void
CVUSBCameraModel::
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
CVUSBCameraModel::
camera_status_changed( bool status )
{
    mCVUSBCameraProperty.mbCameraStatus = status;
}

QObject*
CVUSBCameraModel::
createWorker()
{
    mpCameraWorker = new CVUSBCameraWorker();
    return mpCameraWorker;
}

void
CVUSBCameraModel::
connectWorker(QObject* worker)
{
    auto *cameraWorker = qobject_cast<CVUSBCameraWorker*>(worker);
    if (!cameraWorker)
        return;

    // Connect worker signals to model slots
    connect(cameraWorker, &CVUSBCameraWorker::frameCaptured,
            this, &CVUSBCameraModel::process_captured_frame, Qt::QueuedConnection);
    connect(cameraWorker, &CVUSBCameraWorker::cameraReady,
            this, &CVUSBCameraModel::camera_status_changed, Qt::QueuedConnection);
    connect(cameraWorker, &CVUSBCameraWorker::fpsUpdated,
            this, [this](double fps) { mdLastFps = fps; }, Qt::QueuedConnection);
    connect(cameraWorker, &CVUSBCameraWorker::capabilitiesDetected,
            this, &CVUSBCameraModel::update_camera_capabilities, Qt::QueuedConnection);
}

unsigned int
CVUSBCameraModel::
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
CVUSBCameraModel::
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
CVUSBCameraModel::
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
CVUSBCameraModel::
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
CVUSBCameraModel::
inputConnectionCreated(const QtNodes::ConnectionId& connection)
{
    Q_UNUSED(connection);
    if( mpCameraWorker )
        QMetaObject::invokeMethod(mpCameraWorker, "setSingleShotMode", Qt::QueuedConnection, Q_ARG(bool, true));
}

void
CVUSBCameraModel::
inputConnectionDeleted(const QtNodes::ConnectionId& connection)
{
    Q_UNUSED(connection);
    if( mpCameraWorker )
        QMetaObject::invokeMethod(mpCameraWorker, "setSingleShotMode", Qt::QueuedConnection, Q_ARG(bool, false));
}

QJsonObject
CVUSBCameraModel::
save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams = modelJson[ "cParams" ].toObject();
    cParams[ "camera_id" ] = mCVUSBCameraProperty.miCameraID;
    cParams[ "width" ] = mUSBCameraParams.miWidth;
    cParams[ "height" ] = mUSBCameraParams.miHeight;
    cParams[ "fourcc" ] = mUSBCameraParams.miFourCC;
    cParams[ "brightness" ] = miBrightness;
    cParams[ "gain" ] = miGain;
    cParams[ "exposure" ] = miExposure;
    cParams[ "auto_exposure" ] = mbAutoExposure;
    cParams[ "auto_wb" ] = mbAutoWB;
    cParams[ "auto_focus" ] = mbAutoFocus;
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
CVUSBCameraModel::
load(const QJsonObject &p)
{
    PBAsyncDataModel::load(p);
    QJsonObject paramsObj = p[ "cParams" ].toObject();
    const bool restoringSavedState = !paramsObj.isEmpty();
    mAutoRefreshAllowed = !restoringSavedState;
    late_constructor();

    if( !restoringSavedState )
        return;

    QJsonValue v = paramsObj[ "camera_id" ];
    if( !v.isNull() )
    {
        mCVUSBCameraProperty.miCameraID = v.toInt();
    }

    v = paramsObj[ "width" ];
    if( !v.isNull() )
        mUSBCameraParams.miWidth = v.toInt();
    v = paramsObj[ "height" ];
    if( !v.isNull() )
        mUSBCameraParams.miHeight = v.toInt();

    mCurrentResolution = make_resolution_string( mUSBCameraParams.miWidth, mUSBCameraParams.miHeight );

    // Populate properties with saved values without probing the system
    auto camProp = mMapIdToProperty[ "camera_id" ];
    auto typedCamProp = std::static_pointer_cast< TypedProperty< EnumPropertyType> >( camProp );
    typedCamProp->getData().mslEnumNames = QStringList{ QString::number( mCVUSBCameraProperty.miCameraID ) };
    typedCamProp->getData().miCurrentIndex = 0;

    auto resProp = mMapIdToProperty[ "resolution" ];
    auto typedResProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( resProp );
    typedResProp->getData().mslEnumNames = QStringList{ mCurrentResolution };
    typedResProp->getData().miCurrentIndex = 0;

    mpEmbeddedWidget->set_camera_id_options( typedCamProp->getData().mslEnumNames, mCVUSBCameraProperty.miCameraID );
    mpEmbeddedWidget->set_camera_property( mCVUSBCameraProperty );
    if( isEnable() && mpCameraWorker )
        QMetaObject::invokeMethod(mpCameraWorker, "setCameraId", Qt::QueuedConnection, Q_ARG(int, mCVUSBCameraProperty.miCameraID));

    CVUSBCameraParameters params = mUSBCameraParams;
    params.miWidth = mUSBCameraParams.miWidth;
    params.miHeight = mUSBCameraParams.miHeight;

    v = paramsObj[ "fourcc" ];
    if( !v.isNull() )
    {
        mUSBCameraParams.miFourCC = v.toInt();
        params.miFourCC = mUSBCameraParams.miFourCC;
        
        // Set the format property to match the loaded FourCC
        auto formatProp = mMapIdToProperty[ "image_format" ];
        auto typedFormatProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( formatProp );
        
        // Determine which format matches the loaded FourCC
        int fourcc = mUSBCameraParams.miFourCC;
        int formatIndex = 0; // Default to MJPG
        if( fourcc == cv::VideoWriter::fourcc('M','J','P','G') )
            formatIndex = 0;
        else if( fourcc == cv::VideoWriter::fourcc('Y','U','Y','V') )
            formatIndex = 1;
        else if( fourcc == cv::VideoWriter::fourcc('H','2','6','4') )
            formatIndex = 2;
        else if( fourcc == cv::VideoWriter::fourcc('I','4','2','0') )
            formatIndex = 3;
        else if( fourcc == cv::VideoWriter::fourcc('Y','V','1','2') )
            formatIndex = 4;
        else if( fourcc == cv::VideoWriter::fourcc('R','G','B','3') )
            formatIndex = 5;
        
        typedFormatProp->getData().miCurrentIndex = formatIndex;
    }

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

    v = paramsObj[ "auto_focus" ];
    if( !v.isNull() )
    {
        auto prop = mMapIdToProperty[ "auto_focus" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        mbAutoFocus = v.toBool();
        typedProp->getData() = mbAutoFocus;
        params.miAutoFocus = mbAutoFocus ? 1 : 0;
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
    mUSBCameraParams = params;
    if( mpCameraWorker )
        QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
    Q_EMIT property_structure_changed_signal();
}

void
CVUSBCameraModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBAsyncDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "camera_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( value.toString() );

        mCVUSBCameraProperty.miCameraID = value.toInt();
        mpEmbeddedWidget->set_camera_property( mCVUSBCameraProperty );
        if( isEnable() && mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setCameraId", Qt::QueuedConnection, Q_ARG(int, mCVUSBCameraProperty.miCameraID));

        refresh_camera_capabilities();
    }
    else if( id == "resolution" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        QStringList names = typedProp->getData().mslEnumNames;
        if( value.toInt() >= 0 && value.toInt() < names.size() )
        {
            mCurrentResolution = names.at( value.toInt() );
            int w = mUSBCameraParams.miWidth;
            int h = mUSBCameraParams.miHeight;
            if( parse_resolution_string( mCurrentResolution, w, h ) )
            {
                mUSBCameraParams.miWidth = w;
                mUSBCameraParams.miHeight = h;
                mUSBCameraParams.miWidth = w;
                mUSBCameraParams.miHeight = h;
                if( mpCameraWorker )
                    QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
            }
        }
    }
    else if( id == "image_format" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        QStringList names = typedProp->getData().mslEnumNames;
        if( value.toInt() >= 0 && value.toInt() < names.size() )
        {
            QString format = names.at( value.toInt() );
            int fourcc = 0;
            if( format == "MJPG" )
                fourcc = cv::VideoWriter::fourcc('M','J','P','G');
            else if( format == "YUYV" )
                fourcc = cv::VideoWriter::fourcc('Y','U','Y','V');
            else if( format == "H264" )
                fourcc = cv::VideoWriter::fourcc('H','2','6','4');
            else if( format == "I420" )
                fourcc = cv::VideoWriter::fourcc('I','4','2','0');
            else if( format == "YV12" )
                fourcc = cv::VideoWriter::fourcc('Y','V','1','2');
            else if( format == "RGB3" )
                fourcc = cv::VideoWriter::fourcc('R','G','B','3');

            if( fourcc != 0 )
            {
                mUSBCameraParams.miFourCC = fourcc;
                mUSBCameraParams.miFourCC = fourcc;
                if( mpCameraWorker )
                    QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
            }
        }
    }
    else if( id == "brightness" )
    {
        if( !mCVUSBCameraCapabilities.mbBrightnessSupported )
            return;

        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miBrightness = value.toInt();
        typedProp->getData().miValue = miBrightness;

        mUSBCameraParams.miBrightness = miBrightness;
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
    }
    else if( id == "gain" )
    {
        if( !mCVUSBCameraCapabilities.mbGainSupported )
            return;

        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miGain = value.toInt();
        typedProp->getData().miValue = miGain;

        mUSBCameraParams.miGain = miGain;
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
    }
    else if( id == "auto_wb" )
    {
        if( !mCVUSBCameraCapabilities.mbAutoWbSupported )
            return;

        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        mbAutoWB = value.toBool();
        typedProp->getData() = mbAutoWB;

        mUSBCameraParams.miAutoWB = mbAutoWB ? 1 : 0;
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
    }
    else if( id == "auto_exposure" )
    {
        if( !mCVUSBCameraCapabilities.mbAutoExposureSupported )
            return;

        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        mbAutoExposure = value.toBool();
        typedProp->getData() = mbAutoExposure;

        mUSBCameraParams.miAutoExposure = mbAutoExposure ? 3 : 1;
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
    }
    else if( id == "auto_focus" )
    {
        if( !mCVUSBCameraCapabilities.mbAutoFocusSupported )
            return;

        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        mbAutoFocus = value.toBool();
        typedProp->getData() = mbAutoFocus;

        mUSBCameraParams.miAutoFocus = mbAutoFocus ? 1 : 0;
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
    }
    else if( id == "exposure" )
    {
        if( !mCVUSBCameraCapabilities.mbExposureSupported )
            return;

        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        miExposure = value.toInt();
        typedProp->getData().miValue = miExposure;

        mUSBCameraParams.miExposure = miExposure;
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setParams", Qt::QueuedConnection, Q_ARG(CVUSBCameraParameters, mUSBCameraParams));
    }
}

void
CVUSBCameraModel::
late_constructor()
{
    PBAsyncDataModel::late_constructor();
}

void
CVUSBCameraModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed( enable );

    mpEmbeddedWidget->set_ready_state( !enable );
    if( enable )
    {
        if( mAutoRefreshAllowed )
        {
            refresh_camera_list();
            refresh_camera_capabilities();
        }
        mCVUSBCameraProperty.miCameraID = mpEmbeddedWidget->get_camera_property().miCameraID;
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setCameraId", Qt::QueuedConnection, Q_ARG(int, mCVUSBCameraProperty.miCameraID));
    }
    else
    {
        if( mpCameraWorker )
            QMetaObject::invokeMethod(mpCameraWorker, "setCameraId", Qt::QueuedConnection, Q_ARG(int, -1));
    }
}

void
CVUSBCameraModel::
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
    
    if( button == 3 )
    {
        refresh_camera_list();
        refresh_camera_capabilities();
        Q_EMIT embeddedWidgetSizeUpdated();
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

            mCVUSBCameraProperty.miCameraID = mpEmbeddedWidget->get_camera_property().miCameraID;
            if( isEnable() && mpCameraWorker )
                QMetaObject::invokeMethod(mpCameraWorker, "setCameraId", Qt::QueuedConnection, Q_ARG(int, mCVUSBCameraProperty.miCameraID));

            refresh_camera_capabilities();
        }
    }
    
    Q_EMIT embeddedWidgetSizeUpdated();
}

void
CVUSBCameraModel::
setSelected( bool selected )
{
    PBNodeDelegateModel::setSelected( selected );
    //The second method is to uncomment the following line and comment mpEmbeddedWidget->set_active above.
    //The embedded widget will not accept any mouse interaction unless the node is selected.
    //mpEmbeddedWidget->set_active( selected );
}

template <typename T>
void
CVUSBCameraModel::set_property_read_only(const QString &id, bool readOnly)
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

QStringList
CVUSBCameraModel::
enumerate_camera_ids(int maxCandidates) const
{
    QStringList ids;
    for( int i = 0; i < maxCandidates; ++i )
    {
        cv::VideoCapture probe;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        probe.open( i, cv::CAP_DSHOW );
#elif defined(__APPLE__)
        probe.open( i );
#elif defined(__linux__)
        probe.open( i, cv::CAP_V4L2 );
#else
        probe.open( i );
#endif
        if( probe.isOpened() )
        {
            ids << QString::number( i );
        }
    }

    ids.removeDuplicates();
    return ids;
}

QString CVUSBCameraModel::make_resolution_string(int width, int height)
{
    return QString::number( width ) + "x" + QString::number( height );
}

bool CVUSBCameraModel::parse_resolution_string(const QString &text, int &width, int &height)
{
    const QStringList parts = text.split( 'x', Qt::SkipEmptyParts );
    if( parts.size() != 2 )
        return false;
    bool okW=false, okH=false;
    int w = parts.at(0).toInt( &okW );
    int h = parts.at(1).toInt( &okH );
    if( !okW || !okH )
        return false;
    width = w;
    height = h;
    return true;
}

QStringList
CVUSBCameraModel::
enumerate_resolutions_for_camera(int cameraId) const
{
    static const std::vector<std::pair<int,int>> kCandidates = {
        {640, 480}, {800, 600}, {1024, 768}, {1280, 720}, {1600, 1200},
        {1920, 1080}, {2048, 1536}, {2592, 1944}, {3264, 2448}, {3840, 2160}
    };

    QStringList found;

    cv::VideoCapture cap;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    cap.open( cameraId, cv::CAP_DSHOW );
#elif defined(__APPLE__)
    cap.open( cameraId );
#elif defined(__linux__)
    cap.open( cameraId, cv::CAP_V4L2 );
#else
    cap.open( cameraId );
#endif

    if( !cap.isOpened() )
        return found;

    // Try to preserve current codec/fps if possible
    if( mUSBCameraParams.miFourCC )
        cap.set( cv::CAP_PROP_FOURCC, mUSBCameraParams.miFourCC );
    if( mUSBCameraParams.miFps > 0 )
        cap.set( cv::CAP_PROP_FPS, mUSBCameraParams.miFps );

    double origW = cap.get( cv::CAP_PROP_FRAME_WIDTH );
    double origH = cap.get( cv::CAP_PROP_FRAME_HEIGHT );

    for( const auto &res : kCandidates )
    {
        cap.set( cv::CAP_PROP_FRAME_WIDTH, res.first );
        cap.set( cv::CAP_PROP_FRAME_HEIGHT, res.second );
        const int w = static_cast<int>( cap.get( cv::CAP_PROP_FRAME_WIDTH ) );
        const int h = static_cast<int>( cap.get( cv::CAP_PROP_FRAME_HEIGHT ) );
        if( w == res.first && h == res.second )
            found << make_resolution_string( w, h );
    }

    // Restore
    if( origW > 0 && origH > 0 )
    {
        cap.set( cv::CAP_PROP_FRAME_WIDTH, origW );
        cap.set( cv::CAP_PROP_FRAME_HEIGHT, origH );
    }

    found.removeDuplicates();
    return found;
}

QStringList
CVUSBCameraModel::
enumerate_formats_for_camera(int cameraId) const
{
    QStringList supported;
    
    cv::VideoCapture cap;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    cap.open( cameraId, cv::CAP_DSHOW );
#elif defined(__APPLE__)
    cap.open( cameraId );
#elif defined(__linux__)
    cap.open( cameraId, cv::CAP_V4L2 );
#else
    cap.open( cameraId );
#endif

    if( !cap.isOpened() )
        return supported;
    
    // List of formats to test: MJPG, YUYV, H264, I420, YV12, RGB3
    struct FormatInfo {
        const char* name;
        int fourcc;
    } formats[] = {
        { "MJPG", cv::VideoWriter::fourcc('M','J','P','G') },
        { "YUYV", cv::VideoWriter::fourcc('Y','U','Y','V') },
        { "H264", cv::VideoWriter::fourcc('H','2','6','4') },
        { "I420", cv::VideoWriter::fourcc('I','4','2','0') },
        { "YV12", cv::VideoWriter::fourcc('Y','V','1','2') },
        { "RGB3", cv::VideoWriter::fourcc('R','G','B','3') }
    };
    
    // Save current format
    int originalFourcc = static_cast<int>(cap.get(cv::CAP_PROP_FOURCC));
    
    // Test each format
    for( const auto& fmt : formats )
    {
        if( cap.set(cv::CAP_PROP_FOURCC, fmt.fourcc) )
        {
            // Verify the format was actually set by reading it back
            int readBack = static_cast<int>(cap.get(cv::CAP_PROP_FOURCC));
            if( readBack == fmt.fourcc )
            {
                supported.append(QString(fmt.name));
            }
        }
    }
    
    // Restore original format
    if( originalFourcc != 0 )
        cap.set(cv::CAP_PROP_FOURCC, originalFourcc);
    
    // If no formats detected, provide defaults
    if( supported.isEmpty() )
    {
        supported << "MJPG" << "YUYV";
    }
    
    return supported;
}

void
CVUSBCameraModel::
refresh_camera_capabilities()
{
    // Refresh both resolutions and supported formats for current camera
    QStringList resolutions = enumerate_resolutions_for_camera( mCVUSBCameraProperty.miCameraID );
    if( resolutions.isEmpty() )
    {
        resolutions = QStringList({
            make_resolution_string(640,480),
            make_resolution_string(1280,720),
            make_resolution_string(1920,1080),
            make_resolution_string(2592,1944)
        });
    }

    auto resProp = mMapIdToProperty[ "resolution" ];
    auto typedResProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( resProp );
    typedResProp->getData().mslEnumNames = resolutions;

    // Keep current selection if possible, otherwise choose first
    if( mCurrentResolution.isEmpty() )
        mCurrentResolution = make_resolution_string( mUSBCameraParams.miWidth, mUSBCameraParams.miHeight );

    int resIdx = resolutions.indexOf( mCurrentResolution );
    if( resIdx < 0 && !resolutions.isEmpty() )
    {
        resIdx = 0;
        int w = 0, h = 0;
        if( parse_resolution_string( resolutions.first(), w, h ) )
        {
            mUSBCameraParams.miWidth = w;
            mUSBCameraParams.miHeight = h;
            mCurrentResolution = resolutions.first();
        }
    }
    typedResProp->getData().miCurrentIndex = resIdx;

    // Refresh supported formats
    QStringList formats = enumerate_formats_for_camera( mCVUSBCameraProperty.miCameraID );
    
    auto formatProp = mMapIdToProperty[ "image_format" ];
    auto typedFormatProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( formatProp );
    typedFormatProp->getData().mslEnumNames = formats;
    
    // Determine current format index based on current FourCC
    int fourcc = mUSBCameraParams.miFourCC;
    int formatIdx = 0; // Default to first
    QString currentFormat;
    if( fourcc == cv::VideoWriter::fourcc('M','J','P','G') )
        currentFormat = "MJPG";
    else if( fourcc == cv::VideoWriter::fourcc('Y','U','Y','V') )
        currentFormat = "YUYV";
    else if( fourcc == cv::VideoWriter::fourcc('H','2','6','4') )
        currentFormat = "H264";
    else if( fourcc == cv::VideoWriter::fourcc('I','4','2','0') )
        currentFormat = "I420";
    else if( fourcc == cv::VideoWriter::fourcc('Y','V','1','2') )
        currentFormat = "YV12";
    else if( fourcc == cv::VideoWriter::fourcc('R','G','B','3') )
        currentFormat = "RGB3";
    
    formatIdx = formats.indexOf(currentFormat);
    if( formatIdx < 0 && !formats.isEmpty() )
    {
        formatIdx = 0;
        // Update to first supported format
        QString firstFormat = formats.first();
        if( firstFormat == "MJPG" )
            mUSBCameraParams.miFourCC = cv::VideoWriter::fourcc('M','J','P','G');
        else if( firstFormat == "YUYV" )
            mUSBCameraParams.miFourCC = cv::VideoWriter::fourcc('Y','U','Y','V');
        else if( firstFormat == "H264" )
            mUSBCameraParams.miFourCC = cv::VideoWriter::fourcc('H','2','6','4');
        else if( firstFormat == "I420" )
            mUSBCameraParams.miFourCC = cv::VideoWriter::fourcc('I','4','2','0');
        else if( firstFormat == "YV12" )
            mUSBCameraParams.miFourCC = cv::VideoWriter::fourcc('Y','V','1','2');
        else if( firstFormat == "RGB3" )
            mUSBCameraParams.miFourCC = cv::VideoWriter::fourcc('R','G','B','3');
    }
    typedFormatProp->getData().miCurrentIndex = formatIdx;

    // Rebuild Property Browser
    Q_EMIT property_structure_changed_signal();
}

void
CVUSBCameraModel::
refresh_resolutions()
{
    QStringList ids = enumerate_resolutions_for_camera( mCVUSBCameraProperty.miCameraID );
    if( ids.isEmpty() )
    {
        ids = QStringList({
            make_resolution_string(640,480),
            make_resolution_string(1280,720),
            make_resolution_string(1920,1080),
            make_resolution_string(2592,1944)
        });
    }

    auto prop = mMapIdToProperty[ "resolution" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
    typedProp->getData().mslEnumNames = ids;

    // Keep current selection if possible, otherwise choose first
    if( mCurrentResolution.isEmpty() )
        mCurrentResolution = make_resolution_string( mUSBCameraParams.miWidth, mUSBCameraParams.miHeight );

    int idx = ids.indexOf( mCurrentResolution );
    if( idx < 0 && !ids.isEmpty() )
    {
        idx = 0;
        int w = 0, h = 0;
        if( parse_resolution_string( ids.first(), w, h ) )
        {
            mUSBCameraParams.miWidth = w;
            mUSBCameraParams.miHeight = h;
            mCurrentResolution = ids.first();
        }
    }
    typedProp->getData().miCurrentIndex = idx;

    // Update embedded widget remains unaffected; only Property Browser
    Q_EMIT property_structure_changed_signal();
}

void
CVUSBCameraModel::
refresh_camera_list()
{
    QStringList ids = enumerate_camera_ids();
    if( ids.isEmpty() )
        ids = QStringList( {"0", "1", "2", "3", "4"} );

    auto prop = mMapIdToProperty[ "camera_id" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
    typedProp->getData().mslEnumNames = ids;

    QString currentIdString = QString::number( mCVUSBCameraProperty.miCameraID );
    int currentIndex = ids.indexOf( currentIdString );
    if( currentIndex < 0 && !ids.isEmpty() )
    {
        currentIndex = 0;
        mCVUSBCameraProperty.miCameraID = ids.first().toInt();
    }
    typedProp->getData().miCurrentIndex = currentIndex;

    mpEmbeddedWidget->set_camera_id_options( ids, mCVUSBCameraProperty.miCameraID );

    // Rebuild Property Browser so enum choices reflect current devices
    Q_EMIT property_structure_changed_signal();
}

void
CVUSBCameraModel::
update_camera_capabilities( bool autoFocusSupported,
                            bool autoExposureSupported,
                            bool autoWbSupported,
                            bool brightnessSupported,
                            bool gainSupported,
                            bool exposureSupported )
{
    const bool capabilitiesChanged =
        mCVUSBCameraCapabilities.mbAutoFocusSupported != autoFocusSupported ||
        mCVUSBCameraCapabilities.mbAutoExposureSupported != autoExposureSupported ||
        mCVUSBCameraCapabilities.mbAutoWbSupported != autoWbSupported ||
        mCVUSBCameraCapabilities.mbBrightnessSupported != brightnessSupported ||
        mCVUSBCameraCapabilities.mbGainSupported != gainSupported ||
        mCVUSBCameraCapabilities.mbExposureSupported != exposureSupported;

    mCVUSBCameraCapabilities.mbAutoFocusSupported = autoFocusSupported;
    mCVUSBCameraCapabilities.mbAutoExposureSupported = autoExposureSupported;
    mCVUSBCameraCapabilities.mbAutoWbSupported = autoWbSupported;
    mCVUSBCameraCapabilities.mbBrightnessSupported = brightnessSupported;
    mCVUSBCameraCapabilities.mbGainSupported = gainSupported;
    mCVUSBCameraCapabilities.mbExposureSupported = exposureSupported;

    if( !capabilitiesChanged )
        return;

    // Disable editing when unsupported; enable when supported
    set_property_read_only<bool>( "auto_focus", !mCVUSBCameraCapabilities.mbAutoFocusSupported );
    set_property_read_only<bool>( "auto_wb", !mCVUSBCameraCapabilities.mbAutoWbSupported );
    set_property_read_only<bool>( "auto_exposure", !mCVUSBCameraCapabilities.mbAutoExposureSupported );
    set_property_read_only<IntPropertyType>( "brightness", !mCVUSBCameraCapabilities.mbBrightnessSupported );
    set_property_read_only<IntPropertyType>( "gain", !mCVUSBCameraCapabilities.mbGainSupported );
    set_property_read_only<IntPropertyType>( "exposure", !mCVUSBCameraCapabilities.mbExposureSupported );
}
