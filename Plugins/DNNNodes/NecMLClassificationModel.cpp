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

#include "NecMLClassificationModel.hpp"


#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

#include "qtvariantproperty_p.h"
#include <QFile>
#include <QElapsedTimer>

const QString NecMLClassificationModel::_category = QString("DNN");

const QString NecMLClassificationModel::_model_name = QString( "NecML Classification" );

NecMLClassificationThread::NecMLClassificationThread( QObject * parent )
    : QThread(parent)
{

}


NecMLClassificationThread::
~NecMLClassificationThread()
{
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}


void
NecMLClassificationThread::
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

        QElapsedTimer etimer;
        etimer.start();

        cv::Mat blob;
        cv::dnn::blobFromImage( mCVImage, blob, 1./mParams.mdInvScaleFactor, mParams.mCVSize, mParams.mdInvScaleFactor*mParams.mCVScalarMean, true );
        cv::divide(blob, mParams.mCVScalarStd, blob);
        mNecMLClassification.setInput(blob);
        cv::Mat out = mNecMLClassification.forward();
        double min, max;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(out, &min, &max, &minLoc, &maxLoc);
        float sumScores = 0;
        for( int idx = 0; idx < out.cols; ++idx )
            sumScores += exp(out.at<float>(idx));
        float confidence = exp(max)/sumScores;
        //qDebug() << "Got Confidence ... " << confidence << " " << maxLoc.x;
        qDebug() << "Elapsed Time : " << etimer.nsecsElapsed()/1000000.;

        QString result_information;
        if( maxLoc.x < static_cast<int>(mvStrClasses.size()) )
        {
            QString out_text = "\"Class\" : \"" + QString::fromStdString(mvStrClasses[maxLoc.x] + "\"");
            result_information = "{\n    " + out_text;
            cv::putText(mCVImage, out_text.toStdString(), cv::Point(5, 20), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
            out_text = "\"Prob.\" : \"" + QString::number(confidence) + "\"";
            result_information += ",\n    " + out_text + "\n}";
            cv::putText(mCVImage, out_text.toStdString(), cv::Point(5, 40), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        }
        Q_EMIT result_ready( mCVImage, result_information );
        mLockMutex.unlock();
    }
}


void
NecMLClassificationThread::
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
NecMLClassificationThread::
readNet( QString & model )
{
    try {
        mNecMLClassification = cv::dnn::readNetFromONNX(model.toStdString());
        //mNecMLClassification.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        //mNecMLClassification.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        if( mvStrClasses.size() != 0 )
            mbModelReady = true;
        qDebug() << "Read Model Success! Good to go...";
    }  catch ( cv::Exception & e ) {
        qDebug() << "Cannot Read Model!";
        mbModelReady = false;
    }
    return mbModelReady;
}

void
NecMLClassificationThread::
    setParams(NecMLClassificationBlobImageParameters & params, std::vector< std::string > & classes )
{
    mParams = params;
    mvStrClasses = classes;
}

NecMLClassificationModel::
NecMLClassificationModel()
    : PBNodeDelegateModel( _model_name ),
    _minPixmap(":/NecML.png")
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >( true );
    mpInformationData = std::make_shared< InformationData >();

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msDNNModel_Filename;
    filePathPropertyType.msFilter = "*.onnx";
    filePathPropertyType.msMode = "open";
    QString propId = "model_filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Model Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    filePathPropertyType.msFilename = msConfig_Filename;
    filePathPropertyType.msFilter = "*.json";
    filePathPropertyType.msMode = "open";
    propId = "config_filename";
    propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Config Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdMin = 0.00001;
    doublePropertyType.mdMax = 10000.0;
    doublePropertyType.mdValue = 255.;
    propId = "inv_scale_factor";
    auto propInvScaleFactor = std::make_shared< TypedProperty< DoublePropertyType > >("Inverse Scale Factor", propId, QMetaType::Double, doublePropertyType, "Blob Image", true );
    mvProperty.push_back( propInvScaleFactor );
    mMapIdToProperty[ propId ] = propInvScaleFactor;

    doublePropertyType.mdValue = 0.485;
    propId = "mean_r";
    auto propMean = std::make_shared< TypedProperty< DoublePropertyType > >("Mean R", propId, QMetaType::Double, doublePropertyType, "Blob Image", true );
    mvProperty.push_back( propMean );
    mMapIdToProperty[ propId ] = propMean;

    doublePropertyType.mdValue = 0.456;
    propId = "mean_g";
    propMean = std::make_shared< TypedProperty< DoublePropertyType > >("Mean G", propId, QMetaType::Double, doublePropertyType, "Blob Image", true );
    mvProperty.push_back( propMean );
    mMapIdToProperty[ propId ] = propMean;

    doublePropertyType.mdValue = 0.406;
    propId = "mean_b";
    propMean = std::make_shared< TypedProperty< DoublePropertyType > >("Mean B", propId, QMetaType::Double, doublePropertyType, "Blob Image", true );
    mvProperty.push_back( propMean );
    mMapIdToProperty[ propId ] = propMean;

    doublePropertyType.mdValue = 0.229;
    propId = "std_r";
    auto propStd = std::make_shared< TypedProperty< DoublePropertyType > >("Std R", propId, QMetaType::Double, doublePropertyType, "Blob Image", true );
    mvProperty.push_back( propStd );
    mMapIdToProperty[ propId ] = propStd;

    doublePropertyType.mdValue = 0.224;
    propId = "std_g";
    propStd = std::make_shared< TypedProperty< DoublePropertyType > >("Std G", propId, QMetaType::Double, doublePropertyType, "Blob Image", true );
    mvProperty.push_back( propStd );
    mMapIdToProperty[ propId ] = propStd;

    doublePropertyType.mdValue = 0.225;
    propId = "std_b";
    propStd = std::make_shared< TypedProperty< DoublePropertyType > >("Std B", propId, QMetaType::Double, doublePropertyType, "Blob Image", true );
    mvProperty.push_back( propStd );
    mMapIdToProperty[ propId ] = propStd;

    SizePropertyType sizePropertyType;
    sizePropertyType.miHeight = 300;
    sizePropertyType.miWidth = 300;
    propId = "size";
    auto propBlobSize = std::make_shared< TypedProperty< SizePropertyType > >("Size", propId, QMetaType::QSize, sizePropertyType, "Blob Image", true );
    mvProperty.push_back( propBlobSize );
    mMapIdToProperty[ propId ] = propBlobSize;
}

unsigned int
NecMLClassificationModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 3;
        break;

    default:
        break;
    }

    return result;
}

NodeDataType
NecMLClassificationModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if(portIndex == 0)
            return CVImageData().type();
    }
    else if( portType == PortType::Out )
    {
        if(portIndex == 0)
            return CVImageData().type();
        else if(portIndex == 1)
            return InformationData().type();
        else if(portIndex == 2)
            return SyncData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
NecMLClassificationModel::
outData(PortIndex port)
{
    if( isEnable() )
    {
        if( port == 0 )
            return mpCVImageData;
        else if( port == 1 )
            return mpInformationData;
        else if( port == 2 )
            return mpSyncData;
    }
    return nullptr;
}

void
NecMLClassificationModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;
    if( nodeData && mpSyncData->data() == true )
    {
        mpSyncData->data() = false;
        //Q_EMIT dataUpdated(2);
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
            processData( d );
    }
}


QJsonObject
NecMLClassificationModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["model_filename"] = msDNNModel_Filename;
    cParams["config_filename"] = msConfig_Filename;
    modelJson["cParams"] = cParams;
    return modelJson;
}


void
NecMLClassificationModel::
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
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >( prop );
            typedProp->getData() = v.toString();
            msConfig_Filename = v.toString();
        }
        load_model();
    }
}


void
NecMLClassificationModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "model_filename" || id == "config_filename" )
    {
        if( id == "model_filename" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >(prop);
            typedProp->getData() = value.toString();
            msDNNModel_Filename = value.toString();
        }
        else if( id == "config_filename" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = value.toString();
            msConfig_Filename = value.toString();
        }
        load_model(true);
    }
}


void
NecMLClassificationModel::
late_constructor()
{
    if( !mpNecMLClassificationThread )
    {
        mpNecMLClassificationThread = new NecMLClassificationThread(this);
        connect( mpNecMLClassificationThread, &NecMLClassificationThread::result_ready, this, &NecMLClassificationModel::received_result );
        load_model();
        mpNecMLClassificationThread->start();
    }
}


void
NecMLClassificationModel::
received_result( cv::Mat & result, QString text )
{
    mpCVImageData->set_image( result );
    mpInformationData->set_information( text );
    mpSyncData->data() = true;

    updateAllOutputPorts();
}

void
NecMLClassificationModel::
load_model(bool bUpdateDisplayProperties)
{
    if( msDNNModel_Filename.isEmpty() || msConfig_Filename.isEmpty() )
        return;
    if( QFile::exists( msConfig_Filename) )
    {
        cv::FileStorage fs;
        fs.open(msConfig_Filename.toStdString(), cv::FileStorage::READ );
        if( fs.isOpened() )
        {
            NecMLClassificationBlobImageParameters params;
            params.mdInvScaleFactor = 255.;

            int image_size;
            fs["input_size"] >> image_size;
            if( image_size > 0 )
            {
                auto prop = mMapIdToProperty["size"];
                auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType> >( prop );
                typedProp->getData().miWidth = image_size;
                typedProp->getData().miHeight = image_size;
                params.mCVSize = cv::Size( image_size, image_size );
                if( bUpdateDisplayProperties )
                    Q_EMIT property_changed_signal(prop);
            }

            std::vector<float> vMean, vStd;
            fs["normalize"]["mean"] >> vMean;
            fs["normalize"]["std"] >> vStd;
            if( vMean.size() == 3 && vStd.size() == 3 )
            {
                auto prop = mMapIdToProperty["mean_r"];
                auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
                typedProp->getData().mdValue = vMean[0];
                params.mCVScalarMean[0] = vMean[0];
                if( bUpdateDisplayProperties )
                    Q_EMIT property_changed_signal(prop);
 
                prop = mMapIdToProperty["mean_g"];
                typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
                typedProp->getData().mdValue = vMean[1];
                params.mCVScalarMean[1] = vMean[1];
                if( bUpdateDisplayProperties )
                    Q_EMIT property_changed_signal(prop);
 
                prop = mMapIdToProperty["mean_b"];
                typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
                typedProp->getData().mdValue = vMean[2];
                params.mCVScalarMean[2] = vMean[2];
                if( bUpdateDisplayProperties )
                    Q_EMIT property_changed_signal(prop);
 
                prop = mMapIdToProperty["std_r"];
                typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
                typedProp->getData().mdValue = vStd[0];
                params.mCVScalarStd[0] = vStd[0];
                if( bUpdateDisplayProperties )
                    Q_EMIT property_changed_signal(prop);
 
                prop = mMapIdToProperty["std_g"];
                typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
                typedProp->getData().mdValue = vStd[1];
                params.mCVScalarStd[1] = vStd[1];
                if( bUpdateDisplayProperties )
                    Q_EMIT property_changed_signal(prop);
 
                prop = mMapIdToProperty["std_b"];
                typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
                typedProp->getData().mdValue = vStd[2];
                params.mCVScalarStd[2] = vStd[2];
                if( bUpdateDisplayProperties )
                    Q_EMIT property_changed_signal(prop);
            }

            std::vector< std::string > str_classes;
            fs["idx_to_class"] >> str_classes;

            mpNecMLClassificationThread->setParams( params, str_classes );
            if( QFile::exists(msDNNModel_Filename) )
            {
                if( mpNecMLClassificationThread->readNet( msDNNModel_Filename ) )
                {
                    auto prop = mMapIdToProperty["enable"];
                    auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
                    typedProp->getData() = true;
                    if( bUpdateDisplayProperties )
                        Q_EMIT property_changed_signal(prop);
                    return;
                }
            }
        }
    }
    auto prop = mMapIdToProperty["enable"];
    auto typedProp = std::static_pointer_cast< TypedProperty< bool > > (prop);
    typedProp->getData() = false;
    if( bUpdateDisplayProperties )
        Q_EMIT property_changed_signal(prop);
}

void
NecMLClassificationModel::
processData(const std::shared_ptr< CVImageData > & in)
{
    cv::Mat& in_image = in->data();
    if( !in_image.empty() )
        mpNecMLClassificationThread->detect( in_image );
}


