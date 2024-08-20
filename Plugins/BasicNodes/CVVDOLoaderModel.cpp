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

#include "CVVDOLoaderModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtWidgets/QFileDialog>
#include "internal/Connection.hpp"
#include "qtvariantproperty.h"
#include <QMessageBox>
#include "SyncData.hpp"

/////////////////////////////////////////////////////////////////////////
/// \brief CVVDOLoaderModel::CVVDOLoaderModel
///
CVVDOLoaderModel::
CVVDOLoaderModel()
    : PBNodeDataModel( _model_name, true ),
      mpEmbeddedWidget( new CVVDOLoaderEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );

    mpEmbeddedWidget->set_active(false);
    connect( mpEmbeddedWidget, &CVVDOLoaderEmbeddedWidget::button_clicked_signal, this, &CVVDOLoaderModel::em_button_clicked );
    connect( mpEmbeddedWidget, &CVVDOLoaderEmbeddedWidget::slider_value_signal, this, &CVVDOLoaderModel::no_frame_changed );
    connect( &mTimer, &QTimer::timeout, this, &CVVDOLoaderModel::next_frame );

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
    intPropertyType.miMin = 0;
    intPropertyType.miValue = miFlipPeriodInMillisecond;
    propId = "flip_period";
    auto propSpinBox = std::make_shared< TypedProperty< IntPropertyType > >( "Flip Period (ms)", propId, QMetaType::Int, intPropertyType );
    mvProperty.push_back( propSpinBox );
    mMapIdToProperty[ propId ] = propSpinBox;

    propId = "is_loop";
    auto propIsLoop = std::make_shared< TypedProperty< bool > >( "Loop Play", propId, QMetaType::Bool, true );
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
    }
    else if( portType == PortType::In )
    {
        if( portIndex == 0 )
            return SyncData().type();
    }
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
            mbSyncSignal = d->data();
    }
}

std::shared_ptr<NodeData>
CVVDOLoaderModel::
outData(PortIndex portIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
    {
        if( portIndex == 0 && mpCVImageData->data().data != nullptr )
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
        cParams["use_sync_signal"] = mbUseSyncSignal;
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

        v = paramsObj[ "use_sync_signal" ];
        if( !v.isNull() )
            mbUseSyncSignal = v.toBool();

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
        if( mTimer.isActive() )
            mTimer.start( miFlipPeriodInMillisecond );
    }
    else if( id == "is_loop" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbLoop = value.toBool();
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

    mTimer.stop();

    QFileInfo fi( msVideoFilename );
    mpEmbeddedWidget->set_filename( fi.fileName() );
    mpEmbeddedWidget->set_active(true);

    if( mbCapturing )
        mcvVideoCapture.release();
    mcvVideoCapture = cv::VideoCapture(filename.toStdString());
    if( mcvVideoCapture.isOpened() )
    {
        mcvVideoCapture >> mpCVImageData->data();
        miNextFrame += 1;

        auto image = mpCVImageData->data();
        if( !image.empty() )
        {
            mcvImage_Size = cv::Size(image.cols, image.rows);
            miMaxNoFrames = mcvVideoCapture.get(cv::CAP_PROP_FRAME_COUNT);
            auto channels = image.channels();
            if( channels == 1 )
                msImage_Format = "CV_8UC1";
            else if( channels == 3 )
                msImage_Format = "CV_8UC3";

            mpEmbeddedWidget->set_maximum_slider( miMaxNoFrames );

            auto prop = mMapIdToProperty["image_size"];
            auto typedPropSize = std::static_pointer_cast<TypedProperty<SizePropertyType>>( prop );
            typedPropSize->getData().miWidth = image.cols;
            typedPropSize->getData().miHeight = image.rows;
            Q_EMIT property_changed_signal( prop );

            prop = mMapIdToProperty[ "image_format" ];
            auto typedPropFormat = std::static_pointer_cast<TypedProperty<QString>>( prop );
            typedPropFormat->getData() = msImage_Format;
            Q_EMIT property_changed_signal( prop );

        }
    }

    if( !mbCapturing )
        mbCapturing = true;
}

int CVVDOLoaderModel::getMaxNoFrames() const
{
    return miMaxNoFrames;
}

void CVVDOLoaderModel::setMaxNoFrames(int newMaxNoFrames)
{
    miMaxNoFrames = newMaxNoFrames;
}

void
CVVDOLoaderModel::
em_button_clicked( int button )
{
    if( button == 0 )		// Backward
    {
        if( miNextFrame >= 2 )
            mpEmbeddedWidget->set_slider_value( miNextFrame - 2 );
    }
    else if( button == 1 )	// Auto Play
    {
        mTimer.start(miFlipPeriodInMillisecond);
    }
    else if( button == 2 )	// Pause
    {
        mTimer.stop();
    }
    else if( button == 3 )	// Forward
    {
        if( miNextFrame < miMaxNoFrames )
            mpEmbeddedWidget->set_slider_value( miNextFrame );
        else if( mbLoop )
        {
            mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
            if( mpCVImageData->data().data != nullptr )
                mpEmbeddedWidget->set_slider_value( 0 );
        }
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
next_frame( )
{
    if( mbUseSyncSignal && !mbSyncSignal )
        return;
    mcvVideoCapture >> mpCVImageData->data();
    if( mpCVImageData->data().data != nullptr )
    {
        mpEmbeddedWidget->set_slider_value( miNextFrame );
        miNextFrame += 1;
        mbSyncSignal = false;
// mpEmbeddedWidget->Slider signal is blocked after playing the video file.
        if( isEnable() )
            Q_EMIT dataUpdated( 0 );
    }
}

void
CVVDOLoaderModel::
no_frame_changed( int no_frame )
{
    if( no_frame < miMaxNoFrames )
    {
        mcvVideoCapture.set(cv::CAP_PROP_POS_FRAMES, no_frame );
        mcvVideoCapture >> mpCVImageData->data();
        miNextFrame = no_frame + 1;
        mbSyncSignal = false;
        if( isEnable() )
            Q_EMIT dataUpdated( 0 );
    }
}

void
CVVDOLoaderModel::
inputConnectionCreated(QtNodes::Connection const& conx)
{
    if( conx.getPortIndex(PortType::In) == 0 )
        mbUseSyncSignal = true;
}

void
CVVDOLoaderModel::
inputConnectionDeleted(QtNodes::Connection const& conx)
{
    if( conx.getPortIndex(PortType::In) == 0 )
        mbUseSyncSignal = false;
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
