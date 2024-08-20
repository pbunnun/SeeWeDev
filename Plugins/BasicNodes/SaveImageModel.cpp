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

#include "SaveImageModel.hpp"

#include "nodes/DataModelRegistry"
#include "CVImageData.hpp"
#include "SyncData.hpp"
#include "Connection"
#include <QtWidgets/QFileDialog>
#include "qtvariantproperty.h"
#include <opencv2/imgcodecs.hpp>

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
        auto filename = mFilenameQueue.dequeue();
        auto image = mImageQueue.dequeue();
        cv::imwrite(filename.toStdString(), image);
        image.release();
    }
}

void
SavingImageThread::
add_new_image( const cv::Mat & image, QString filename )
{
    QString dst_filename = mqDirname.absoluteFilePath(filename);
    mImageQueue.enqueue(image.clone());
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
SaveImageModel::
SaveImageModel()
    : PBNodeDataModel( _model_name )
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
}

unsigned int
SaveImageModel::
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
SaveImageModel::
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
SaveImageModel::
outData(PortIndex)
{
    return mpSyncData;
}

void
SaveImageModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() || !nodeData )
        return;

    if(portIndex == 0)
    {
        mpSyncData->data() = false;
        //Q_EMIT dataUpdated(0);
        mpCVImageInData = std::dynamic_pointer_cast< CVImageData >(nodeData);
        if( mbSyncData2SaveImage )
        {
            return;
        }
        else if( !mbUseProvidedFilename )
        {
            QString filename = msPrefix_Filename + "-" + QString::number(miCounter++) + ".jpg";
            if( !mpCVImageInData->data().empty() )
            {
                mpSavingImageThread->add_new_image( mpCVImageInData->data(), filename );
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
                mpCVImageInData = nullptr;
            }
        }
        else if( mpFilenameData )
        {
            if( !mpCVImageInData->data().empty() )
            {
                mpSavingImageThread->add_new_image( mpCVImageInData->data(), mpFilenameData->info() );
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
                mpCVImageInData = nullptr;
                mpFilenameData = nullptr;
            }
        }
    }
    else if(portIndex == 1)
    {
        mpFilenameData = std::dynamic_pointer_cast< InformationData >( mpFilenameData );
        if( mpCVImageInData )
        {
            if( !mpCVImageInData->data().empty() && mpFilenameData )
            {
                mpSavingImageThread->add_new_image( mpCVImageInData->data(), mpFilenameData->info() );
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
                mpCVImageInData = nullptr;
                mpFilenameData = nullptr;
            }
        }
    }
    else if(portIndex == 2)
    {
        auto sync_nd = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( sync_nd->data() && mpCVImageInData )
        {
            QString filename = msPrefix_Filename + "-" + QString::number(miCounter++) + ".jpg";
            if( !mpCVImageInData->data().empty() )
            {
                mpSavingImageThread->add_new_image( mpCVImageInData->data(), filename );
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
                mpCVImageInData = nullptr;
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
SaveImageModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();
    if( !msDirname.isEmpty() )
    {
        QJsonObject cParams;
        cParams["dirname"] = msDirname;
        cParams["prefix_filename"] = msPrefix_Filename;
        modelJson["cParams"] = cParams;
    }
    return modelJson;
}

void
SaveImageModel::
restore( QJsonObject const &p )
{
    PBNodeDataModel::restore(p);
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
    }
}

void
SaveImageModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

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
}

void
SaveImageModel::
inputConnectionCreated(QtNodes::Connection const& conx)
{
    if( conx.getPortIndex(PortType::In) == 1 )
        mbUseProvidedFilename = true;
    else if( conx.getPortIndex(PortType::In) == 2 )
        mbSyncData2SaveImage = true;
}

void
SaveImageModel::
inputConnectionDeleted(QtNodes::Connection const& conx)
{
    if( conx.getPortIndex(PortType::In) == 1 )
        mbUseProvidedFilename = false;
    else if( conx.getPortIndex(PortType::In) == 2)
        mbSyncData2SaveImage = false;
}

void
SaveImageModel::
late_constructor()
{
    if( !mpSavingImageThread )
    {
        mpSavingImageThread = new SavingImageThread( this );
    }
}
const QString SaveImageModel::_category = QString( "Utility" );

const QString SaveImageModel::_model_name = QString( "Save Image" );
