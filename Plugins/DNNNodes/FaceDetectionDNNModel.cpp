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

#include "FaceDetectionDNNModel.hpp"


#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

#include "qtvariantproperty_p.h"
#include <QFile>

const QString FaceDetectionDNNModel::_category = QString("DNN");

const QString FaceDetectionDNNModel::_model_name = QString( "DNN Face Detector" );

FaceDetectorThread::FaceDetectorThread( QObject * parent )
    : QThread(parent)
{

}


FaceDetectorThread::
~FaceDetectorThread()
{
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}


void
FaceDetectorThread::
run()
{
    while( !mbAbort )
    {
        mWaitingSemaphore.acquire();
        if( !mbModelReady )
            continue;
        if( mbAbort )
            break;
        mLockMutex.lock();
        cv::Mat blob;
        auto blobSize = std::max(mCVImage.cols, mCVImage.rows);
        cv::dnn::blobFromImage( mCVImage, blob, 1.0, cv::Size(blobSize, blobSize), cv::Scalar(104, 177, 123));
        mFaceDetector.setInput(blob);
        cv::Mat out = mFaceDetector.forward();
        float * detections = (float*)out.data;
        cv::Scalar color(0, 0, 255);
        // every detection is a [batchId(0), classId(0), confidence, left, top, right, bottom] vector.
        for( int i = 0; i < static_cast<int>(out.total())/7; ++i )
        {
           float confidence = detections[ i*7 + 2 ];
           if( confidence < 0.7 )
               continue;
           int xmin = std::max(0.f, std::min(detections[i*7+3], 1.f)) * mCVImage.cols;
           int ymin = std::max(0.f, std::min(detections[i*7+4], 1.f)) * mCVImage.rows;
           int xmax = std::max(0.f, std::min(detections[i*7+5], 1.f)) * mCVImage.cols;
           int ymax = std::max(0.f, std::min(detections[i*7+6], 1.f)) * mCVImage.rows;
            cv::rectangle(mCVImage, cv::Point(xmin, ymin), cv::Point(xmax, ymax), color, 3);
        }
        Q_EMIT result_ready( mCVImage );
        mLockMutex.unlock();
    }
}


void
FaceDetectorThread::
detect( const cv::Mat & in_image )
{
    if( mLockMutex.tryLock() )
//    if( mWaitingSemaphore.available() == 0 )
    {
        in_image.copyTo( mCVImage );
        mWaitingSemaphore.release();
        mLockMutex.unlock();
    }
}


bool
FaceDetectorThread::
readNet( QString & model, QString & config )
{
    try {
        mFaceDetector = cv::dnn::readNet(model.toStdString(), config.toStdString());
        mFaceDetector.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        mFaceDetector.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        mbModelReady = true;
    }  catch ( cv::Exception & e ) {
        mbModelReady = false;
    }
    return mbModelReady;
}

FaceDetectionDNNModel::
FaceDetectionDNNModel()
    : PBNodeDelegateModel( _model_name ),
    _minPixmap(":/FaceDetection.png")
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >();
    mpSyncData->data() = true;

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msDNNModel_Filename;
    filePathPropertyType.msFilter = "*.caffemodel";
    filePathPropertyType.msMode = "open";
    QString propId = "model_filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Model Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    filePathPropertyType.msFilename = msDNNConfig_Filename;
    filePathPropertyType.msFilter = "*.prototxt";
    filePathPropertyType.msMode = "open";
    propId = "config_filename";
    propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Config Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;
}

unsigned int
FaceDetectionDNNModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 2;
        break;

    default:
        break;
    }

    return result;
}

NodeDataType
FaceDetectionDNNModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
    {
        return CVImageData().type();
    }
    else if(portIndex == 1)
    {
        return SyncData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
FaceDetectionDNNModel::
outData(PortIndex port)
{
    if( isEnable() )
    {
        if( port == 0 )
        {
            return mpCVImageData;
        }
        else if( port == 1 )
        {
            return mpSyncData;
        }
    }
    return nullptr;
}

void
FaceDetectionDNNModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;
    if( nodeData && mpSyncData->data() == true )
    {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
            processData( d );
    }
}


QJsonObject
FaceDetectionDNNModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["model_filename"] = msDNNModel_Filename;
    cParams["config_filename"] = msDNNConfig_Filename;
    modelJson["cParams"] = cParams;
    return modelJson;
}


void
FaceDetectionDNNModel::
restore( QJsonObject const &p )
{
    PBNodeDelegateModel::load( p );
    late_constructor();

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj["model_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["model_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
            typedProp->getData() = v.toString();
            msDNNModel_Filename = v.toString();
        }

        v = paramsObj["config_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["config_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
            typedProp->getData() = v.toString();
            msDNNConfig_Filename = v.toString();
        }
        load_model();
    }
}


void
FaceDetectionDNNModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "model_filename" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >(prop);
        typedProp->getData() = value.toString();
        msDNNModel_Filename = value.toString();
    }
    else if( id == "config_filename" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >(prop);
        typedProp->getData() = value.toString();
        msDNNConfig_Filename = value.toString();
    }
    load_model();
}


void
FaceDetectionDNNModel::
late_constructor()
{
    if( !mpFaceDetectorThread )
    {
        mpFaceDetectorThread = new FaceDetectorThread(this);
        connect( mpFaceDetectorThread, &FaceDetectorThread::result_ready, this, &FaceDetectionDNNModel::received_result );
        load_model();
        mpFaceDetectorThread->start();
    }
}


void
FaceDetectionDNNModel::
received_result( cv::Mat & result )
{
    mpCVImageData->set_image( result );
    mpSyncData->data() = true;

    updateAllOutputPorts();
}

void
FaceDetectionDNNModel::
load_model()
{
    if( msDNNConfig_Filename.isEmpty() || msDNNModel_Filename.isEmpty() )
        return;
    if( QFile::exists(msDNNConfig_Filename) && QFile::exists(msDNNModel_Filename) )
    {
        mpFaceDetectorThread->readNet( msDNNModel_Filename, msDNNConfig_Filename );
    }
}

void
FaceDetectionDNNModel::
processData(const std::shared_ptr< CVImageData > & in)
{
    cv::Mat& in_image = in->data();
    if( !in_image.empty() )
        mpFaceDetectorThread->detect( in_image );
}


