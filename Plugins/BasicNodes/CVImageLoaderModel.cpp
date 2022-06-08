#include "CVImageLoaderModel.hpp"
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
#include "Connection"

CVImageLoaderModel::
CVImageLoaderModel()
    : PBNodeDataModel( _model_name, true ),
      mpEmbeddedWidget( new CVImageLoaderEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );

    mpEmbeddedWidget->set_active(false);
    connect( mpEmbeddedWidget, &CVImageLoaderEmbeddedWidget::button_clicked_signal, this, &CVImageLoaderModel::em_button_clicked );
    connect( &mTimer, &QTimer::timeout, this, &CVImageLoaderModel::flip_image);

    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpInformationData = std::make_shared< InformationData >( );
    //Example for using subclasses of InformationData
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
    auto propSpinBox = std::make_shared< TypedProperty< IntPropertyType > >( "Flip Period (ms)", propId, QVariant::Int, intPropertyType );
    mvProperty.push_back( propSpinBox );
    mMapIdToProperty[ propId ] = propSpinBox;

    propId = "is_loop";
    auto propIsLoop = std::make_shared< TypedProperty< bool > >( "Loop Flip", propId, QVariant::Bool, true );
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

    propId = "info_time";
    auto propInfoTime = std::make_shared< TypedProperty< bool > >( "Time", propId, QVariant::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoTime );
    mMapIdToProperty[ propId ] = propInfoTime;
    propId = "info_image_type";
    auto propInfoImageType = std::make_shared< TypedProperty< bool > >( "Image Type", propId, QVariant::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoImageType );
    mMapIdToProperty[ propId ] = propInfoImageType;
    propId = "info_image_format";
    auto propInfoImageFormat = std::make_shared< TypedProperty< bool > >( "Image Format", propId, QVariant::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoImageFormat );
    mMapIdToProperty[ propId ] = propInfoImageFormat;
    propId = "info_image_size";
    auto propInfoImageSize = std::make_shared< TypedProperty< bool > >( "Image Size", propId, QVariant::Bool, true, "Info Display" );
    mvProperty.push_back( propInfoImageSize );
    mMapIdToProperty[ propId ] = propInfoImageSize;
    propId = "info_image_filename";
    auto propInfoImageFilename = std::make_shared< TypedProperty< bool > >( "Image Filename", propId, QVariant::Bool, true, "Info Display" );
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
            mbSyncSignal = d->state();
    }
}

std::shared_ptr<NodeData>
CVImageLoaderModel::
outData(PortIndex portIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
    {
        if( portIndex == 0 && mpCVImageData->image().data != nullptr )
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
    QJsonObject modelJson = PBNodeDataModel::save();
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
restore( QJsonObject const &p )
{
    PBNodeDataModel::restore( p );

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
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    if( id == "filename" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
        typedProp->getData().msFilename = value.toString();

        QString filename = value.toString();
        set_image_filename( filename );
    }
    else if( id == "dirname" )
    {
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
            if( isSelected() )
                Q_EMIT property_changed_signal( prop );
            else
                set_image_filename( mvsImageFilenames[miFilenameIndex] );
            mpEmbeddedWidget->set_active(true);
        }
    }
}

void
CVImageLoaderModel::
set_image_filename(QString & filename)
{
    if( msImageFilename == filename )
        return;

    msImageFilename = filename;
    if( !QFile::exists( msImageFilename ) )
        return;

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
        mpCVSizeData->size().width = cvImage.cols;
        mpCVSizeData->size().height = cvImage.rows;

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
    if( button == 0 )		// Backward
    {
        miFilenameIndex -= 1;
        if( miFilenameIndex < 0 )
            miFilenameIndex = mvsImageFilenames.size()-1;
        auto prop = mMapIdToProperty[ "filename" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
        typedProp->getData().msFilename = mvsImageFilenames[miFilenameIndex];

        if( isSelected() )
            Q_EMIT property_changed_signal( prop );
        else
            set_image_filename( mvsImageFilenames[miFilenameIndex] );
    }
    else if( button == 1 )	// Open Directory
    {
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
            auto prop = mMapIdToProperty[ "dirname" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > >( prop );
            typedProp->getData().msPath = path;

            if( isSelected() )
                Q_EMIT property_changed_signal( prop );
            else
                set_dirname(path);
        }
    }
    else if( button == 2 )	// Auto Play
    {
        mTimer.start(miFlipPeriodInMillisecond);
    }
    else if( button == 3 )	// Pause
    {
        mTimer.stop();
    }
    else if( button == 4 )	// Forward
    {
        miFilenameIndex += 1;
        if( miFilenameIndex >= mvsImageFilenames.size() )
            miFilenameIndex = 0;
        auto prop = mMapIdToProperty[ "filename" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
        typedProp->getData().msFilename = mvsImageFilenames[miFilenameIndex];

        if( isSelected() )
            Q_EMIT property_changed_signal( prop );
        else
            set_image_filename( mvsImageFilenames[miFilenameIndex] );
    }
    else if( button == 5 )  // Open File
    {
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
            auto prop = mMapIdToProperty[ "filename" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
            typedProp->getData().msFilename = filename;

            if( isSelected() )
                Q_EMIT property_changed_signal( prop );
            else
                set_image_filename( filename );

            // Clear Directory Name
            prop = mMapIdToProperty[ "dirname" ];
            auto pathTypedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > > ( prop );
            pathTypedProp->getData().msPath = "";
            msDirname = "";
            mvsImageFilenames.clear();
            mpEmbeddedWidget->set_active(false);
            if( isSelected() )
                Q_EMIT property_changed_signal( prop );
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
    if( miFilenameIndex >= mvsImageFilenames.size() )
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
    auto prop = mMapIdToProperty[ "filename" ];
    auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
    typedProp->getData().msFilename = mvsImageFilenames[miFilenameIndex];

    if( isSelected() )
        Q_EMIT property_changed_signal( prop );
    else
        set_image_filename( mvsImageFilenames[miFilenameIndex] );

    mbSyncSignal = false;
}

void
CVImageLoaderModel::
inputConnectionCreated(QtNodes::Connection const& conx)
{
    if( conx.getPortIndex(PortType::In) == 0 )
        mbUseSyncSignal = true;
}

void
CVImageLoaderModel::
inputConnectionDeleted(QtNodes::Connection const& conx)
{
    if( conx.getPortIndex(PortType::In) == 0 )
        mbUseSyncSignal = false;
}
/*
void
CVImageLoaderModel::
enable_changed( bool enable )
{
    if( enable )
        updateAllOutputPorts();
}
*/
const QString CVImageLoaderModel::_category = QString( "Source" );

const QString CVImageLoaderModel::_model_name = QString( "CV Image Loader" );
