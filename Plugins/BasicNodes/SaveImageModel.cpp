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
#include "Connection"
#include <QtWidgets/QFileDialog>
#include "qtvariantproperty.h"
#include <opencv2/imgcodecs.hpp>

SavingImageThread::SavingImageThread(QObject *parent) : QThread(parent)
{
    mqDirname = QDir("C:\\");
}

SavingImageThread::~SavingImageThread()
{
    mbAbort = true;
    mNoImageSemaphore.release();
    wait();
    while( mImageQueue.size() > 0 )
    {
        mImageQueue.dequeue();
        mFilenameQueue.dequeue();
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
    }
}

void
SavingImageThread::
add_new_image( cv::Mat & image, QString filename )
{
    cv::Mat clone = image.clone();
    QString dst_filename = mqDirname.absoluteFilePath(filename);
    mImageQueue.enqueue(clone);
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
    PathPropertyType pathPropertyType;
    pathPropertyType.msPath = msDirname;
    QString propId = "dirname";
    auto propDirname = std::make_shared< TypedProperty< PathPropertyType > >( "Saving Directory", propId, QtVariantPropertyManager::pathTypeId(), pathPropertyType);
    mvProperty.push_back( propDirname );
    mMapIdToProperty[ propId ] = propDirname;
}

unsigned int
SaveImageModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 2;
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
        else if( portIndex == 1)
        {
            return InformationData().type();
        }
    }
    return NodeDataType();
}

void
SaveImageModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() || !nodeData )
        return;

    if(portIndex == 0)
    {
        mpNodeData = nodeData;
        if( !mbUseProvidedFilename )
        {
            QString filename = "image-" + QString::number(miCounter++) + ".jpg";
            auto image_nd = std::dynamic_pointer_cast< CVImageData >( mpNodeData );
            if( !image_nd->image().empty() )
            {
                mpSavingImageThread->add_new_image( image_nd->image(), filename );
                mpNodeData = nullptr;
            }
        }
        else if( mpFilenameData )
        {
            auto filename_nd = std::dynamic_pointer_cast< InformationData >( mpFilenameData );
            auto image_nd = std::dynamic_pointer_cast< CVImageData >( mpNodeData );
            if( !image_nd->image().empty() && filename_nd )
            {
                mpSavingImageThread->add_new_image( image_nd->image(), filename_nd->info() );
                mpNodeData = nullptr;
                mpFilenameData = nullptr;
            }
        }
    }
    else if(portIndex == 1)
    {
        mpFilenameData = nodeData;
        auto filename_nd = std::dynamic_pointer_cast< InformationData >( mpFilenameData );
        if( mpNodeData )
        {
            auto image_nd = std::dynamic_pointer_cast< CVImageData >( mpNodeData );
            if( !image_nd->image().empty() && filename_nd )
            {
                mpSavingImageThread->add_new_image( image_nd->image(), filename_nd->info() );
                mpNodeData = nullptr;
                mpFilenameData = nullptr;
            }
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
    }
}

void
SaveImageModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    if( id == "dirname" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > >( prop );
        typedProp->getData().msPath = value.toString();

        msDirname = value.toString();
        mpSavingImageThread->set_saving_directory( msDirname );
    }
}

void
SaveImageModel::
inputConnectionCreated(QtNodes::Connection const& conx)
{
    if( conx.getPortIndex(PortType::In) == 1 )
        mbUseProvidedFilename = true;
}

void
SaveImageModel::
inputConnectionDeleted(QtNodes::Connection const& conx)
{
    if( conx.getPortIndex(PortType::In) == 1 )
        mbUseProvidedFilename = false;
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
