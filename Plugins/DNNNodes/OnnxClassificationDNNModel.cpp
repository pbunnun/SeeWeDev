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

#include "OnnxClassificationDNNModel.hpp"

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

#include "qtvariantproperty.h"
#include <QFile>

OnnxClassificationDNNThread::OnnxClassificationDNNThread( QObject * parent )
    : QThread(parent)
{

}


OnnxClassificationDNNThread::
~OnnxClassificationDNNThread()
{
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}


void
OnnxClassificationDNNThread::
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
        cv::dnn::blobFromImage( mCVImage, blob, 1./mParams.mdInvScaleFactor, mParams.mCVSize, mParams.mCVScalarMean, true );
        mOnnxClassificationDNN.setInput(blob);
        cv::Mat out = mOnnxClassificationDNN.forward();
        double min, max;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(out, &min, &max, &minLoc, &maxLoc);
        float sumScores = 0;
        for( int idx = 0; idx < out.cols; ++idx )
            sumScores += exp(out.at<float>(idx));
        float confidence = exp(max)/sumScores;
        if( maxLoc.x < mvStrClasses.size() )
        {
            QString out_text = "Class : " + QString::fromStdString(mvStrClasses[maxLoc.x]);
            cv::putText(mCVImage, out_text.toStdString(), cv::Point(25, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
            out_text = "Prob. : " + QString::number(confidence);
            cv::putText(mCVImage, out_text.toStdString(), cv::Point(25, 100), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
        }
        Q_EMIT result_ready( mCVImage );
        mLockMutex.unlock();
    }
}


void
OnnxClassificationDNNThread::
detect( const cv::Mat & in_image )
{
    if( mLockMutex.tryLock() )
    {
        in_image.copyTo( mCVImage );
        mWaitingSemaphore.release();
        mLockMutex.unlock();
    }
}


bool
OnnxClassificationDNNThread::
readNet( QString & model, QString & classes )
{
    try {
        mOnnxClassificationDNN = cv::dnn::readNetFromONNX(model.toStdString());
        mOnnxClassificationDNN.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        mOnnxClassificationDNN.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);

        cv::FileStorage fs;
        fs.open(classes.toStdString(), cv::FileStorage::READ);
        if( fs.isOpened() )
        {
            mvStrClasses.clear();
            fs["classes"] >> mvStrClasses;
            mbModelReady = true;
            fs.release();
        }
    }  catch ( cv::Exception & e ) {
        mbModelReady = false;
    }
    return mbModelReady;
}

void
OnnxClassificationDNNThread::
setParams(OnnxClassificationDNNBlobImageParameters & params)
{
    mParams = params;
}

OnnxClassificationDNNModel::
OnnxClassificationDNNModel()
    : PBNodeDataModel( _model_name )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >();

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msDNNModel_Filename;
    filePathPropertyType.msFilter = "*.onnx";
    filePathPropertyType.msMode = "open";
    QString propId = "model_filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Model Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    filePathPropertyType.msFilename = msClasses_Filename;
    filePathPropertyType.msFilter = "*.yaml";
    filePathPropertyType.msMode = "open";
    propId = "classes_filename";
    propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Classes Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdMin = 0.00001;
    doublePropertyType.mdMax = 10000.0;
    doublePropertyType.mdValue = 255.;
    propId = "inv_scale_factor";
    auto propInvScaleFactor = std::make_shared< TypedProperty< DoublePropertyType > >("Inverse Scale Factor", propId, QVariant::Double, doublePropertyType, "Blob Image" );
    mvProperty.push_back( propInvScaleFactor );
    mMapIdToProperty[ propId ] = propInvScaleFactor;

    doublePropertyType.mdValue = 127.5;
    propId = "mean_r";
    auto propMean = std::make_shared< TypedProperty< DoublePropertyType > >("Mean R", propId, QVariant::Double, doublePropertyType, "Blob Image" );
    mvProperty.push_back( propMean );
    mMapIdToProperty[ propId ] = propMean;

    propId = "mean_g";
    propMean = std::make_shared< TypedProperty< DoublePropertyType > >("Mean G", propId, QVariant::Double, doublePropertyType, "Blob Image" );
    mvProperty.push_back( propMean );
    mMapIdToProperty[ propId ] = propMean;

    propId = "mean_b";
    propMean = std::make_shared< TypedProperty< DoublePropertyType > >("Mean B", propId, QVariant::Double, doublePropertyType, "Blob Image" );
    mvProperty.push_back( propMean );
    mMapIdToProperty[ propId ] = propMean;

    SizePropertyType sizePropertyType;
    sizePropertyType.miHeight = 300;
    sizePropertyType.miWidth = 300;
    propId = "size";
    auto propBlobSize = std::make_shared< TypedProperty< SizePropertyType > >("Size", propId, QVariant::Size, sizePropertyType, "Blob Image");
    mvProperty.push_back( propBlobSize );
    mMapIdToProperty[ propId ] = propBlobSize;
}

unsigned int
OnnxClassificationDNNModel::
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
OnnxClassificationDNNModel::
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
OnnxClassificationDNNModel::
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
OnnxClassificationDNNModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;
    if( nodeData && mpSyncData->state() == true )
    {
        mpSyncData->state() = false;
        Q_EMIT dataUpdated(1);
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
            processData( d );
    }
}


QJsonObject
OnnxClassificationDNNModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();
    QJsonObject cParams;
    cParams["model_filename"] = msDNNModel_Filename;
    cParams["classes_filename"] = msClasses_Filename;
    auto params = mpOnnxClassificationDNNThread->getParams();
    cParams["inv_scale_factor"] = params.mdInvScaleFactor;
    cParams["mean_r"] = params.mCVScalarMean[0];
    cParams["mean_g"] = params.mCVScalarMean[1];
    cParams["mean_b"] = params.mCVScalarMean[2];
    cParams["size_width"] = params.mCVSize.width;
    cParams["size_height"] = params.mCVSize.height;
    modelJson["cParams"] = cParams;
    return modelJson;
}


void
OnnxClassificationDNNModel::
restore( QJsonObject const &p )
{
    PBNodeDataModel::restore( p );
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

        v = paramsObj["classes_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["classes_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >( prop );
            typedProp->getData() = v.toString();
            msClasses_Filename = v.toString();
        }

        OnnxClassificationDNNBlobImageParameters params;
        v = paramsObj["inv_scale_factor"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["inv_scale_factor"];
            auto typedProp = std::static_pointer_cast< TypedProperty<DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();
            params.mdInvScaleFactor = v.toDouble();
        }

        v = paramsObj["mean_r"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["mean_r"];
            auto typedProp = std::static_pointer_cast< TypedProperty<DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();
            params.mCVScalarMean[0] = v.toDouble();
        }

        v = paramsObj["mean_g"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["mean_g"];
            auto typedProp = std::static_pointer_cast< TypedProperty<DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();
            params.mCVScalarMean[1] = v.toDouble();
        }

        v = paramsObj["mean_b"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["mean_b"];
            auto typedProp = std::static_pointer_cast< TypedProperty<DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();
            params.mCVScalarMean[0] = v.toDouble();
        }

        auto width = paramsObj["size_width"];
        auto height = paramsObj["size_height"];
        if( !width.isNull() && !height.isNull() )
        {
            auto prop = mMapIdToProperty["size"];
            auto typedProp = std::static_pointer_cast< TypedProperty<SizePropertyType> >( prop );
            typedProp->getData().miWidth = width.toInt();
            typedProp->getData().miHeight = height.toInt();

            params.mCVSize = cv::Size( width.toInt(), height.toInt() );
        }
        mpOnnxClassificationDNNThread->setParams( params );

        load_model();
    }
}


void
OnnxClassificationDNNModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "model_filename" || id == "classes_filename" )
    {
        if( id == "model_filename" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >(prop);
            typedProp->getData() = value.toString();
            msDNNModel_Filename = value.toString();
        }
        else if( id == "classes_filename" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = value.toString();
            msClasses_Filename = value.toString();
        }
        load_model();
    }
    else
    {
        if( id == "inv_scale_factor" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = value.toDouble();

            auto params = mpOnnxClassificationDNNThread->getParams();
            params.mdInvScaleFactor = value.toDouble();
            mpOnnxClassificationDNNThread->setParams(params);
        }
        else if( id == "mean_r" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = value.toDouble();

            auto params = mpOnnxClassificationDNNThread->getParams();
            params.mCVScalarMean[0] = value.toDouble();
            mpOnnxClassificationDNNThread->setParams(params);
        }
        else if( id == "mean_g" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = value.toDouble();

            auto params = mpOnnxClassificationDNNThread->getParams();
            params.mCVScalarMean[1] = value.toDouble();
            mpOnnxClassificationDNNThread->setParams(params);
        }
        else if( id == "mean_b" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = value.toDouble();

            auto params = mpOnnxClassificationDNNThread->getParams();
            params.mCVScalarMean[2] = value.toDouble();
            mpOnnxClassificationDNNThread->setParams(params);
        }
        else if( id == "size" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = value.toSize().width();
            typedProp->getData().miHeight = value.toSize().height();

            auto params = mpOnnxClassificationDNNThread->getParams();
            params.mCVSize = cv::Size( value.toSize().width(), value.toSize().height() );
            mpOnnxClassificationDNNThread->setParams(params);
        }
    }
}


void
OnnxClassificationDNNModel::
late_constructor()
{
    if( !mpOnnxClassificationDNNThread )
    {
        mpOnnxClassificationDNNThread = new OnnxClassificationDNNThread(this);
        connect( mpOnnxClassificationDNNThread, &OnnxClassificationDNNThread::result_ready, this, &OnnxClassificationDNNModel::received_result );
        load_model();
        mpOnnxClassificationDNNThread->start();
    }
}


void
OnnxClassificationDNNModel::
received_result( cv::Mat & result )
{
    mpCVImageData->set_image( result );
    mpSyncData->state() = true;

    updateAllOutputPorts();
}

void
OnnxClassificationDNNModel::
load_model()
{
    if( msDNNModel_Filename.isEmpty() || msClasses_Filename.isEmpty() )
        return;
    if( QFile::exists(msDNNModel_Filename) && QFile::exists( msClasses_Filename) )
    {
        mpOnnxClassificationDNNThread->readNet( msDNNModel_Filename, msClasses_Filename );
    }
}

void
OnnxClassificationDNNModel::
processData(const std::shared_ptr< CVImageData > & in)
{
    cv::Mat& in_image = in->image();
    if( !in_image.empty() )
        mpOnnxClassificationDNNThread->detect( in_image );
}

const QString OnnxClassificationDNNModel::_category = QString("DNN");

const QString OnnxClassificationDNNModel::_model_name = QString( "Onnx Classification Model" );
