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

#include "CVSaveImageModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"
#include <QtWidgets/QFileDialog>
#include "qtvariantproperty_p.h"
#include <opencv2/imgcodecs.hpp>
#include <QFileInfo>
#include <QtNodes/internal/ConnectionIdUtils.hpp>

const QString CVSaveImageModel::_category = QString( "Utility" );

const QString CVSaveImageModel::_model_name = QString( "CV Save Image" );

SavingImageThread::SavingImageThread(QObject *parent) : QThread(parent)
{
#ifdef _WIN32
    mqDirname = QDir("C:\\");
#else
    mqDirname = QDir("./");
#endif
}

SavingImageThread::~SavingImageThread()
{
    mbAbort = true;
    mNoImageSemaphore.release();
    wait();
    while( mImageQueue.size() > 0 )
    {
        auto image = mImageQueue.dequeue();
        mFilenameQueue.dequeue();
        image.release();
    }
}

void
SavingImageThread::run()
{
    while( !mbAbort )
    {
        mNoImageSemaphore.acquire();
        if( mbAbort )
            continue;
        int retry_counter = 0;
        auto abs_filename = mFilenameQueue.dequeue();
        while( QFile::exists( abs_filename ) && retry_counter < 100 )
        {
            QFileInfo fileinfo(abs_filename);
            QString new_filename = fileinfo.baseName() + "-" + QDateTime::currentDateTime().toString("yyMMdd-hhmmss") + "." + fileinfo.suffix();
            abs_filename = mqDirname.absoluteFilePath(new_filename);
            retry_counter++;
            msleep(10);
        }
        auto image = mImageQueue.dequeue();
        cv::imwrite(abs_filename.toStdString(), image);
        image.release();
    }
}

void
SavingImageThread::
add_new_image( cv::Mat & image, QString filename )
{
    QString dst_filename = mqDirname.absoluteFilePath(filename);
    mImageQueue.enqueue(std::move(image));
    mFilenameQueue.enqueue(dst_filename);
    if( !isRunning() )
        start();
    mNoImageSemaphore.release();
}

void
SavingImageThread::
set_saving_directory( QString dirname )
{
    mqDirname = QDir(dirname);
}
/////////////////////////////////////////////////////////////////////////////////
CVSaveImageModel::
CVSaveImageModel()
    : PBNodeDelegateModel( _model_name )
{
    mpSyncData = std::make_shared<SyncData>( true );

    PathPropertyType pathPropertyType;
    pathPropertyType.msPath = msDirname;
    QString propId = "dirname";
    auto propDirname = std::make_shared< TypedProperty< PathPropertyType > >( "Saving Directory", propId, QtVariantPropertyManager::pathTypeId(), pathPropertyType);
    mvProperty.push_back( propDirname );
    mMapIdToProperty[ propId ] = propDirname;

    propId = "prefix_filename";
    auto propFilename = std::make_shared< TypedProperty< QString > >("Prefix Filename", propId, QMetaType::QString, msPrefix_Filename);
    mvProperty.push_back( propFilename );
    mMapIdToProperty[ propId ] = propFilename;

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"jpg", "png"});
    enumPropertyType.miCurrentIndex = 1;
    propId = "image_format";
    auto propImageFormat = std::make_shared< TypedProperty< EnumPropertyType > >("Image Format", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propImageFormat );
    mMapIdToProperty[ propId ] = propImageFormat;
}

unsigned int
CVSaveImageModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 3;
    else if( portType == PortType::Out )
        return 1;
    else
        return 0;
}

NodeDataType
CVSaveImageModel::
dataType( PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if( portIndex == 0 )
        {
            return CVImageData().type();
        }
        else if( portIndex == 1 )
        {
            return InformationData().type();
        }
        else if( portIndex == 2 )
        {
            return SyncData().type();
        }
    }
    else if( portType == PortType::Out )
        return SyncData().type();
    return NodeDataType();
}

std::shared_ptr<NodeData>
CVSaveImageModel::
outData(PortIndex)
{
    return mpSyncData;
}

void
CVSaveImageModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() || !nodeData )
        return;

    if(portIndex == 0)
    {
        mpSyncData->data() = false;
        auto p = std::dynamic_pointer_cast< CVImageData >(nodeData);
        p->data().copyTo( mCVMatInImage );   // prevent issues if the source image is modified or destroyed
        if( mbSyncData2SaveImage )
        {
            return;
        }
        else if( msFilename.isEmpty() )
        {
            if( !mCVMatInImage.empty() )
            {
                QString filename = msPrefix_Filename + "-" + QString::number(miCounter++) + "." + msImage_Format;
                mpSavingImageThread->add_new_image( mCVMatInImage, filename );
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
                mCVMatInImage.release();   // just to make sure the data is released 
            }
        }
        else // msFilename is not empty 
        {
            if( !mCVMatInImage.empty() )
            {
                mpSavingImageThread->add_new_image( mCVMatInImage, msFilename );
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
                mCVMatInImage.release();
                msFilename.clear();
            }
        }
    }
    else if(portIndex == 1)
    {
        auto p = std::dynamic_pointer_cast< InformationData >( nodeData );
        msFilename = p->info();
        if( mbSyncData2SaveImage )
        {
            return;
        }
        else if( !mCVMatInImage.empty() && !msFilename.isEmpty() )
        {
            mpSavingImageThread->add_new_image( mCVMatInImage, msFilename );
            mpSyncData->data() = true;
            Q_EMIT dataUpdated(0);
            mCVMatInImage.release();
            msFilename.clear();
        }
    }
    else if(portIndex == 2)
    {
        auto sync_nd = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( sync_nd->data() && !mCVMatInImage.empty() )
        {
            if( msFilename.isEmpty() )
            {
                QString filename = msPrefix_Filename + "-" + QString::number(miCounter++) + "." + msImage_Format;
                mpSavingImageThread->add_new_image( mCVMatInImage, filename );
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
                mCVMatInImage.release();
            }
            else
            {
                mpSavingImageThread->add_new_image( mCVMatInImage, msFilename );
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
                mCVMatInImage.release();
                msFilename.clear();
            }
        }
        else
        {
            mpSyncData->data() = false;
            Q_EMIT dataUpdated(0);
        }
    }
}

QJsonObject
CVSaveImageModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    if( !msDirname.isEmpty() )
    {
        QJsonObject cParams;
        cParams["dirname"] = msDirname;
        cParams["prefix_filename"] = msPrefix_Filename;
        cParams["image_format"] = msImage_Format;
        modelJson["cParams"] = cParams;
    }
    return modelJson;
}

void
CVSaveImageModel::
load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);
    late_constructor();

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj["dirname"];
        if( !v.isNull() )
        {
            QString dirname = v.toString();
            auto qDir = QDir(dirname);
            if( !dirname.isEmpty() && qDir.exists() )
            {
                auto prop = mMapIdToProperty["dirname"];
                auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > > ( prop );
                typedProp->getData().msPath = dirname;
                msDirname = dirname;
                mpSavingImageThread->set_saving_directory( msDirname );
            }
        }
        v = paramsObj["prefix_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["prefix_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
            typedProp->getData() = v.toString();
            msPrefix_Filename = v.toString();
        }
        v = paramsObj["image_format"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["image_format"];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >(prop);
            int iCurrentIndex = 0;
            if( v.toString() == "jpg" )
                iCurrentIndex = 0;
            else if( v.toString() == "png" )
                iCurrentIndex = 1;
            typedProp->getData().miCurrentIndex = iCurrentIndex;
            msImage_Format = v.toString();
        }
    }
}

void
CVSaveImageModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "dirname" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > >( prop );
        typedProp->getData().msPath = value.toString();

        msDirname = value.toString();
        mpSavingImageThread->set_saving_directory( msDirname );
    }
    else if( id == "prefix_filename" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();
        msPrefix_Filename = value.toString();
    }
    else if( id == "image_format" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        if( value.toInt() == 0 )
            msImage_Format = "jpg";
        else if( value.toInt() == 1 )
            msImage_Format = "png";
    }
}

void
CVSaveImageModel::
inputConnectionCreated(QtNodes::ConnectionId const& conx)
{
    if( QtNodes::getPortIndex(PortType::In, conx) == 2 )
        mbSyncData2SaveImage = true;
}

void
CVSaveImageModel::
inputConnectionDeleted(QtNodes::ConnectionId const& conx)
{
    if( QtNodes::getPortIndex(PortType::In, conx) == 2)
        mbSyncData2SaveImage = false;
}

void
CVSaveImageModel::
late_constructor()
{
    if( !mpSavingImageThread )
    {
        mpSavingImageThread = new SavingImageThread( this );
    }
}

