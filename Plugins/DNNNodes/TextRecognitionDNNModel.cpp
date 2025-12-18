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

#include "TextRecognitionDNNModel.hpp"


#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

#include "qtvariantproperty_p.h"
#include <QFile>
#include <fstream>

#include <QIcon>

const QString TextRecognitionDNNModel::_category = QString("DNN");

const QString TextRecognitionDNNModel::_model_name = QString( "Text Recognition Model" );

TextRecognitionThread::TextRecognitionThread( QObject * parent )
    : QThread(parent)
{

}


TextRecognitionThread::
~TextRecognitionThread()
{
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}


void
TextRecognitionThread::
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
        std::string recognitionResult = mTextRecognitionDNN.recognize(mCVImage);
        cv::putText(mCVImage, recognitionResult, cv::Point(mCVImage.cols/2, mCVImage.rows/2), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2 );
        //qDebug() << "Prediction : " << QString::fromStdString(recognitionResult);
        Q_EMIT result_ready( mCVImage, QString::fromStdString(recognitionResult) );
        mLockMutex.unlock();
    }
}


void
TextRecognitionThread::
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
TextRecognitionThread::
readNet( QString & model )
{
    try {
        mTextRecognitionDNN = cv::dnn::TextRecognitionModel(model.toStdString());
        //mTextRecognitionDNN.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        //mTextRecognitionDNN.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);

        double scale = 1.0/255.0;
        cv::Scalar mean = cv::Scalar(127.5, 127.5, 127.5);
        cv::Size inputSize = cv::Size(100, 32);
        mTextRecognitionDNN.setInputParams(scale, inputSize, mean);

        if( !msVocabulary_Filename.isEmpty() )
            setParams(msVocabulary_Filename);

        mbModelReady = true;
    }  catch ( cv::Exception & e ) {
        mbModelReady = false;
    }
    return mbModelReady;
}

void
TextRecognitionThread::
setParams(QString & vocabulary_filename)
{
    if( vocabulary_filename.isEmpty() )
        return;
    if( QFile::exists(vocabulary_filename) )
    {
        msVocabulary_Filename = vocabulary_filename;
        if( mbModelReady )
        {
            std::ifstream voc_file;
            voc_file.open(vocabulary_filename.toStdString());
            if( voc_file.is_open() )
            {
                cv::String voc_line;
                std::vector<cv::String> vocabulary;
                while( std::getline(voc_file, voc_line))
                    vocabulary.push_back(voc_line);
                mTextRecognitionDNN.setVocabulary(vocabulary);
                mTextRecognitionDNN.setDecodeType("CTC-greedy");
            }
        }
    }
}

TextRecognitionDNNModel::
TextRecognitionDNNModel()
    : PBNodeDelegateModel( _model_name )
{
    QIcon icon(":/TextRecognitionDNNModel.svg");
    _minPixmap = icon.pixmap(108,108);

    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >(true);
    mpInformationData = std::make_shared< InformationData >();

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msModel_Filename;
    filePathPropertyType.msFilter = "*.onnx";
    filePathPropertyType.msMode = "open";
    QString propId = "model_filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Model Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    filePathPropertyType.msFilename = msVocabulary_Filename;
    filePathPropertyType.msFilter = "*.txt";
    filePathPropertyType.msMode = "open";
    propId = "vocabulary_filename";
    propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Vocabulary Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

}

unsigned int
TextRecognitionDNNModel::
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
TextRecognitionDNNModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if( portIndex == 0 )
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
TextRecognitionDNNModel::
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
TextRecognitionDNNModel::
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
TextRecognitionDNNModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["model_filename"] = msModel_Filename;
    cParams["vocabulary_filename"] = msVocabulary_Filename;
    modelJson["cParams"] = cParams;
    return modelJson;
}


void
TextRecognitionDNNModel::
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
            msModel_Filename = v.toString();
        }

        v = paramsObj["vocabulary_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["vocabulary_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
            typedProp->getData() = v.toString();
            msVocabulary_Filename = v.toString();
        }

        load_model();
        mpTextRecognitionDNNThread->setParams(msVocabulary_Filename);
    }
}


void
TextRecognitionDNNModel::
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
        msModel_Filename = value.toString();
        load_model();
    }
    else if( id == "vocabulary_filename" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >(prop);
        typedProp->getData() = value.toString();
        msVocabulary_Filename = value.toString();
        mpTextRecognitionDNNThread->setParams(msVocabulary_Filename);
    }
}


void
TextRecognitionDNNModel::
late_constructor()
{
    if( !mpTextRecognitionDNNThread )
    {
        mpTextRecognitionDNNThread = new TextRecognitionThread(this);
        connect( mpTextRecognitionDNNThread, &TextRecognitionThread::result_ready, this, &TextRecognitionDNNModel::received_result );
        load_model();
        mpTextRecognitionDNNThread->start();
    }
}


void
TextRecognitionDNNModel::
received_result( cv::Mat & result, QString text )
{
    mpCVImageData->set_image( result );
    mpInformationData->set_information(text);
    mpSyncData->data() = true;

    updateAllOutputPorts();
}

void
TextRecognitionDNNModel::
load_model()
{
    if( msModel_Filename.isEmpty())
        return;
    if( QFile::exists(msModel_Filename) )
    {
        mpTextRecognitionDNNThread->readNet( msModel_Filename );
    }
}

void
TextRecognitionDNNModel::
processData(const std::shared_ptr< CVImageData > & in)
{
    cv::Mat& in_image = in->data();
    if( !in_image.empty() )
        mpTextRecognitionDNNThread->detect( in_image );
}


