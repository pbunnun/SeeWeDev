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

#include "CVImageLoaderModel.hpp"
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

const QString CVImageLoaderModel::_category = QString( "Source" );

const QString CVImageLoaderModel::_model_name = QString( "CV Image Loader" );

CVImageLoaderModel::
CVImageLoaderModel()
    : PBNodeDelegateModel( _model_name, true ),
      mpEmbeddedWidget( new CVImageLoaderEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );

    mpEmbeddedWidget->set_active(false);
    connect( mpEmbeddedWidget, &CVImageLoaderEmbeddedWidget::button_clicked_signal, this, &CVImageLoaderModel::em_button_clicked );
    connect( mpEmbeddedWidget, &CVImageLoaderEmbeddedWidget::widget_resized_signal, this, &CVImageLoaderModel::embeddedWidgetSizeUpdated );
    connect( &mTimer, &QTimer::timeout, this, &CVImageLoaderModel::flip_image);

    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpInformationData = std::make_shared< InformationData >( );
    mpCVSizeData = std::make_shared< CVSizeData >( cv::Size() );

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msImageFilename;
    filePathPropertyType.msFilter = "*.jpg;*.jpeg;*.bmp;*.tiff;*.tif;*.pbm;*.png";
    filePathPropertyType.msMode = "open";
    QString propId = "filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType > >( "Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType );
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    PathPropertyType pathPropertyType;
    pathPropertyType.msPath = msDirname;
    propId = "dirname";
    auto propDirName = std::make_shared< TypedProperty<PathPropertyType > >( "Dirname", propId, QtVariantPropertyManager::pathTypeId(), pathPropertyType );
    mvProperty.push_back( propDirName );
    mMapIdToProperty[ propId ] = propDirName;

    IntPropertyType intPropertyType;
    intPropertyType.miMax = 60000;
    intPropertyType.miMin = 5;
    intPropertyType.miValue = miFlipPeriodInMillisecond;
    propId = "flip_period";
    auto propSpinBox = std::make_shared< TypedProperty< IntPropertyType > >( "Flip Period (ms)", propId, QMetaType::Int, intPropertyType );
    mvProperty.push_back( propSpinBox );
    mMapIdToProperty[ propId ] = propSpinBox;

    propId = "is_loop";
    auto propIsLoop = std::make_shared< TypedProperty< bool > >( "Loop Flip", propId, QMetaType::Bool, true );
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

    propId = "info_time";
    auto propInfoTime = std::make_shared< TypedProperty< bool > >( "Time", propId, QMetaType::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoTime );
    mMapIdToProperty[ propId ] = propInfoTime;
    propId = "info_image_type";
    auto propInfoImageType = std::make_shared< TypedProperty< bool > >( "Image Type", propId, QMetaType::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoImageType );
    mMapIdToProperty[ propId ] = propInfoImageType;
    propId = "info_image_format";
    auto propInfoImageFormat = std::make_shared< TypedProperty< bool > >( "Image Format", propId, QMetaType::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoImageFormat );
    mMapIdToProperty[ propId ] = propInfoImageFormat;
    propId = "info_image_size";
    auto propInfoImageSize = std::make_shared< TypedProperty< bool > >( "Image Size", propId, QMetaType::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoImageSize );
    mMapIdToProperty[ propId ] = propInfoImageSize;
    propId = "info_image_filename";
    auto propInfoImageFilename = std::make_shared< TypedProperty< bool > >( "Image Filename", propId, QMetaType::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoImageFilename );
    mMapIdToProperty[ propId ] = propInfoImageFilename;
}


unsigned int
CVImageLoaderModel::
nPorts( PortType portType ) const
{
    switch ( portType )
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
CVImageLoaderModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::Out )
        switch( portIndex )
        {
        case 0:
            return CVImageData().type();
        case 1:
            return InformationData().type();
        case 2:
            return CVSizeData().type();
        default:
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
CVImageLoaderModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex )
{
    if( !isEnable() )
        return;
    if( portIndex == 0 )
    {
        auto d = std::dynamic_pointer_cast< SyncData > ( nodeData );
        if( d )
            mbSyncSignal = d->data();
    }
}

std::shared_ptr<NodeData>
CVImageLoaderModel::
outData(PortIndex portIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
    {
        if( portIndex == 0 && mpCVImageData->data().data != nullptr )
            result = mpCVImageData;
        else if( portIndex == 1 )
            result = mpInformationData;
        else if( portIndex == 2)
            result = mpCVSizeData;
    }
    return result;
}


QJsonObject
CVImageLoaderModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    if( !msImageFilename.isEmpty() || !msDirname.isEmpty() )
    {
        QJsonObject cParams;
        cParams["filename"] = msImageFilename;
        cParams["dirname"] = msDirname;
        cParams["flip_period"] = miFlipPeriodInMillisecond;
        cParams["is_loop"] = mbLoop;
        cParams["info_time"] = mbInfoTime;
        cParams["info_image_type"] = mbInfoImageType;
        cParams["info_image_format"] = mbInfoImageFormat;
        cParams["info_image_size"] = mbInfoImageSize;
        cParams["info_image_filename"] = mbInfoImageFilename;
        cParams["use_sync_signal"] = mbUseSyncSignal;
        modelJson["cParams"] = cParams;
    }
    return modelJson;
}

void
CVImageLoaderModel::
load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        bool b_no_dirname = true;
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

        v = paramsObj[ "info_time" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "info_time" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
            typedProp->getData() = v.toBool();
            mbInfoTime = v.toBool();
        }
        v = paramsObj[ "info_image_type" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "info_image_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
            typedProp->getData() = v.toBool();
            mbInfoImageType = v.toBool();
        }
        v = paramsObj[ "info_image_format" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "info_image_format" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
            typedProp->getData() = v.toBool();
            mbInfoImageFormat = v.toBool();
        }
        v = paramsObj[ "info_image_size" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "info_image_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
            typedProp->getData() = v.toBool();
            mbInfoImageSize = v.toBool();
        }
        v = paramsObj[ "info_image_filename" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "info_image_filename" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
            typedProp->getData() = v.toBool();
            mbInfoImageFilename = v.toBool();
        }

        v = paramsObj[ "dirname" ];
        if( !v.isNull() )
        {
            QString dirname = v.toString();
            if( !dirname.isEmpty() && QFile::exists(dirname) )
            {
                auto prop = mMapIdToProperty[ "dirname" ];
                auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > >( prop );
                typedProp->getData().msPath = v.toString();

                set_dirname( dirname );
                if( !mvsImageFilenames.empty() )
                {
                    QFileInfo fi( mvsImageFilenames[miFilenameIndex] );
                    mpEmbeddedWidget->set_filename( fi.fileName() );
                    set_image_filename( mvsImageFilenames[miFilenameIndex] );
                }
                b_no_dirname = false;
            }
        }
        if( b_no_dirname )
        {
            v = paramsObj[ "filename" ];
            if( !v.isNull() )
            {
                if( QFile::exists(v.toString()) )
                {
                    auto prop = mMapIdToProperty[ "filename" ];
                    auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
                    typedProp->getData().msFilename = v.toString();

                    QString filename = v.toString();
                    set_image_filename( filename );
                }
            }
        }

    }
}

void
CVImageLoaderModel::
setModelProperty( QString & id, const QVariant & value )
{
    DEBUG_LOG_INFO() << "[setModelProperty] id:" << id << "value:" << value;

    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
    {
        DEBUG_LOG_INFO() << "[setModelProperty] Property not in map, returning";
        return;
    }

    if( id == "filename" )
    {
        DEBUG_LOG_INFO() << "[setModelProperty] Setting filename";
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
        typedProp->getData().msFilename = value.toString();

        QString filename = value.toString();
        set_image_filename( filename );
    }
    else if( id == "dirname" )
    {
        DEBUG_LOG_INFO() << "[setModelProperty] Setting dirname";
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > >( prop );
        typedProp->getData().msPath = value.toString();

        QString dirname = value.toString();
        set_dirname( dirname );
    }
    else if( id == "flip_period" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
        typedProp->getData().miValue = value.toInt();

        miFlipPeriodInMillisecond = value.toInt();
        if( mTimer.isActive() )
            mTimer.start(miFlipPeriodInMillisecond);
    }
    else if( id == "is_loop" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbLoop = value.toBool();
    }
    else if( id == "info_time" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbInfoTime = value.toBool();
    }
    else if( id == "info_image_type" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbInfoImageType = value.toBool();
    }
    else if( id == "info_image_format" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbInfoImageFormat = value.toBool();
    }
    else if( id == "info_image_size" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbInfoImageSize = value.toBool();
    }
    else if( id == "info_image_filename" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
        typedProp->getData() = value.toBool();

        mbInfoImageFilename = value.toBool();
    }
}

void
CVImageLoaderModel::
set_dirname(QString & dirname)
{
    QDir directory = QDir(dirname);
    if( !dirname.isEmpty() && directory.exists() )
    {
        if( msDirname == dirname )
            return;

        msDirname = dirname;

        mvsImageFilenames.clear();
        QStringList filters;
        filters << "*.jpg" << "*.jpeg" << "*.bmp" << "*.tiff" << "*.tif" << "*.pbm" << "*.png";
        QStringList filenames = directory.entryList(filters, QDir::Files);
        if( !filenames.isEmpty() )
        {
            mTimer.stop();

            for(QString & filename: filenames)
                mvsImageFilenames.push_back( directory.absoluteFilePath(filename) );
            miFilenameIndex = 0;
            auto prop = mMapIdToProperty[ "filename" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
            typedProp->getData().msFilename = mvsImageFilenames[miFilenameIndex];
            set_image_filename( mvsImageFilenames[miFilenameIndex] );
            if( isSelected() )
                Q_EMIT property_changed_signal( prop );
            mpEmbeddedWidget->set_active(true);
        }
    }
}

void
CVImageLoaderModel::
set_image_filename(QString & filename)
{
    DEBUG_LOG_INFO() << "[set_image_filename] filename:" << filename;
    
    if( msImageFilename == filename )
    {
        DEBUG_LOG_INFO() << "[set_image_filename] Same filename, returning";
        return;
    }

    msImageFilename = filename;
    if( !QFile::exists( msImageFilename ) )
    {
        DEBUG_LOG_INFO() << "[set_image_filename] File does not exist, returning";
        return;
    }

    QImage qImage = QImage( msImageFilename );

    if( qImage.isNull() )
    {
        QMessageBox msg;
        QString msgText = "Cannot load " + msImageFilename + " !!!";
        msg.setIcon( QMessageBox::Critical );
        msg.setText( msgText );
        msg.exec();
        return; // unsupport image formats
    }

    auto q_image_format = qImage.format();
    int cv_image_format = 0;
    QString image_format = "";
    QString sInformation;
    if( mbInfoTime )
        sInformation += "Time\t: " + QTime::currentTime().toString( "hh:mm:ss.zzz" );

    if( q_image_format == QImage::Format_Grayscale8 )
    {
        cv_image_format = CV_8UC1;
        if( mbInfoImageType )
        {
            if( sInformation.size() != 0 )
                sInformation += "\n";
            sInformation += "Type\t: Gray";
        }
        image_format = "CV_8UC1";
    }
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    else if( q_image_format == QImage::Format_Mono || q_image_format == QImage::Format_MonoLSB )
#else
    else if( q_image_format == QImage::Format_Grayscale16 || q_image_format == QImage::Format_Mono || q_image_format == QImage::Format_MonoLSB )
#endif
    {
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
        qImage = qImage.convertToFormat( QImage::Format_Grayscale8 );
#else
        qImage.convertTo( QImage::Format_Grayscale8 );
#endif
        cv_image_format = CV_8UC1;
        if( mbInfoImageType )
        {
            if( sInformation.size() != 0 )
                sInformation += "\n";
            sInformation += "Type\t: Gray";
        }
        image_format = "CV_8UC1";
    }
    else if( q_image_format == QImage::Format_Invalid || q_image_format == QImage::Format_Alpha8 )
    {
        QMessageBox msg;
        QString msgText = "Image format is not supported!";
        msg.setIcon( QMessageBox::Critical );
        msg.setText( msgText );
        msg.exec();
        if( mbInfoImageType )
        {
            if( sInformation.size() != 0 )
                sInformation += "\n";
            sInformation += "Image format is not supported!.";
        }
        image_format = "Not Supported!";
        return; // unsupport image formats
    }
    else
    {
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
        qImage = qImage.convertToFormat( QImage::Format_RGB888 ).rgbSwapped();
#else
        qImage.convertTo( QImage::Format_BGR888 );
#endif
        cv_image_format = CV_8UC3;
        if( mbInfoImageType )
        {
            if( sInformation.size() != 0 )
                sInformation += "\n";
            sInformation += "Type\t: Color";
        }
        image_format = "CV_8UC3";
    }

    if( mbInfoImageFormat )
    {
        if( sInformation.size() != 0 )
            sInformation += "\n";
        sInformation += "Format\t: " + image_format;
    }

    cv::Mat cvImage = cv::Mat( qImage.height(), qImage.width(), cv_image_format, const_cast<uchar*>( qImage.bits() ), static_cast<size_t>(qImage.bytesPerLine()) );

    if( cvImage.data != nullptr )
    {
        QFileInfo fi(msImageFilename);
        DEBUG_LOG_INFO() << "[set_image_filename] Setting embedded widget filename:" << fi.fileName();
        mpEmbeddedWidget->set_filename( fi.fileName() );
        mpCVImageData->set_image( cvImage );
        if( mbInfoImageSize )
        {
            if( sInformation.size() != 0 )
                sInformation += "\n";
            sInformation += "WxH\t: " + QString::number( cvImage.cols ) + " x " + QString::number( cvImage.rows );
        }
        if( mbInfoImageFilename )
        {
            if( sInformation.size() != 0 )
                sInformation += "\n";
            sInformation += fi.fileName(); //"Filename\t: " + fi.fileName();
        }

        mpInformationData->set_information( sInformation );
        mpCVSizeData->data().width = cvImage.cols;
        mpCVSizeData->data().height = cvImage.rows;

        auto prop = mMapIdToProperty["image_size"];
        auto typedPropSize = std::static_pointer_cast<TypedProperty<SizePropertyType>>( prop );
        typedPropSize->getData().miWidth = cvImage.cols;
        typedPropSize->getData().miHeight = cvImage.rows;
        Q_EMIT property_changed_signal( prop );

        prop = mMapIdToProperty[ "image_format" ];
        auto typedPropFormat = std::static_pointer_cast<TypedProperty<QString>>( prop );
        typedPropFormat->getData() = image_format;
        Q_EMIT property_changed_signal( prop );

        if( isEnable() )
            updateAllOutputPorts();
    }
}

void
CVImageLoaderModel::
em_button_clicked( int button )
{
    DEBUG_LOG_INFO() << "[em_button_clicked] button:" << button << "isSelected:" << isSelected();
    
    // If node is not selected, select it first and block the interaction
    // User needs to click again when node is selected to perform the action
    if (!isSelected())
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Node not selected, requesting selection";
        
        // Revert Play/Pause button state if it was clicked while unselected
        if (button == 2 || button == 3)
            mpEmbeddedWidget->revert_play_pause_state();
        
        Q_EMIT selection_request_signal();
        return;
    }
    
    if( button == 0 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Backward button";
        miFilenameIndex -= 1;
        if( miFilenameIndex < 0 )
            miFilenameIndex = mvsImageFilenames.size()-1;
        
        // Use the unified property change system
        requestPropertyChange("filename", mvsImageFilenames[miFilenameIndex]);
    }
    else if( button == 1 )	// Open Directory
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Open Directory button";
        QString dir = QDir::homePath();
        if( !msImageFilename.isEmpty() )
        {
            QFileInfo fi(msImageFilename);
            if( !fi.absoluteDir().isEmpty() )
                dir = fi.absoluteDir().absolutePath();
        }
        QString path = QFileDialog::getExistingDirectory(qobject_cast<QWidget *>(this), tr("Directory"), dir);
        if( !path.isNull() )
        {
            // Use the unified property change system
            requestPropertyChange("dirname", path);
        }
    }
    else if( button == 2 )	// Auto Play
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Auto Play button";
        mTimer.start(miFlipPeriodInMillisecond);
    }
    else if( button == 3 )	// Pause
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Pause button";
        mTimer.stop();
    }
    else if( button == 4 )	// Forward
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Forward button";
        miFilenameIndex += 1;
        if( miFilenameIndex >= static_cast<int>(mvsImageFilenames.size()) )
            miFilenameIndex = 0;
        
        // Use the unified property change system
        requestPropertyChange("filename", mvsImageFilenames[miFilenameIndex]);
    }
    else if( button == 5 )	// Open File
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Open File button";
        QString dir = QDir::homePath();
        if( !msImageFilename.isEmpty() )
        {
            QFileInfo fi(msImageFilename);
            if( !fi.absoluteDir().isEmpty() )
                dir = fi.absoluteDir().absolutePath();
        }
        QString filename = QFileDialog::getOpenFileName( nullptr,
                                                         tr( "Open Image" ),
                                                         dir,
                                                         tr( "Image Files (*.jpg *.jpeg *.bmp *.tiff *.tif *.pbm *.png)") );
        if( !filename.isEmpty() )
        {
            mTimer.stop();
            
            // Use the unified property change system
            requestPropertyChange("filename", filename);

            // Clear Directory Name
            msDirname = "";
            mvsImageFilenames.clear();
            mpEmbeddedWidget->set_active(false);
            requestPropertyChange("dirname", QString(""));
        }
    }
}

void
CVImageLoaderModel::
flip_image()
{
    if( mbUseSyncSignal && !mbSyncSignal )
        return;
    miFilenameIndex += 1;
    if( miFilenameIndex >= static_cast<int>(mvsImageFilenames.size()) )
    {
        if( !mbLoop )
        {
            miFilenameIndex = -1;
            mTimer.stop();
            mpEmbeddedWidget->set_flip_pause(false);
            return;
        }
        else
            miFilenameIndex = 0;
    }

    mbSyncSignal = false;

    auto prop = mMapIdToProperty[ "filename" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
    typedProp->getData().msFilename = mvsImageFilenames[miFilenameIndex];

    // Always load the image and update the widget
    set_image_filename( mvsImageFilenames[miFilenameIndex] );
    
    // Also update property browser if node is selected
    if( isSelected() )
        Q_EMIT property_changed_signal( prop );

}

void
CVImageLoaderModel::
inputConnectionCreated(QtNodes::ConnectionId const& conx)
{
    if( QtNodes::getPortIndex(PortType::In, conx) == 0 )
        mbUseSyncSignal = true;
}

void
CVImageLoaderModel::
inputConnectionDeleted(QtNodes::ConnectionId const& conx)
{
    if( QtNodes::getPortIndex(PortType::In, conx) == 0 )
        mbUseSyncSignal = false;
}

void
CVImageLoaderModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed(enable);

    if (!enable) {
        mTimer.stop(); // Stop playback if node is disabled
        mpEmbeddedWidget->set_flip_pause(false); // Update UI state if needed
    }
    else {
        // Optionally resume playback if desired, or just update outputs
        updateAllOutputPorts();
    }
}

