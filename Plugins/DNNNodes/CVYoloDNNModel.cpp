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

#include "CVYoloDNNModel.hpp"

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

#include "qtvariantproperty.h"
#include <QFile>
#include <QMessageBox>
#include <fstream>

CVYoloDNNThread::CVYoloDNNThread( QObject * parent )
    : QThread(parent)
{

}


CVYoloDNNThread::
~CVYoloDNNThread()
{
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}


void
CVYoloDNNThread::
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
        cv::dnn::blobFromImage( mCVImage, blob, 1.0, mParams.mCVSize, cv::Scalar(), mParams.mbSwapRB, false, CV_8U );
        mCVYoloDNN.setInput(blob, "", 1./mParams.mdInvScaleFactor);
        std::vector<cv::Mat> outs;
        mCVYoloDNN.forward(outs, mvStrOutNames);

        static std::vector<int> outLayers = mCVYoloDNN.getUnconnectedOutLayers();
        static std::string outLayerType = mCVYoloDNN.getLayer(outLayers[0])->type;
        std::vector<int> classIds;
        std::vector<float> confidences;
        std::vector<cv::Rect> boxes;
        if( outLayerType == "Region" )
        {
            for( size_t i = 0; i < outs.size(); ++i )
            {
                float* data = (float*)outs[i].data;
                for( int j = 0; j < outs[i].rows; ++j, data += outs[i].cols )
                {
                    cv::Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
                    cv::Point classIdPoint;
                    double confidence;
                    cv::minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
                    if( confidence > 0.7 )
                    {
                        int centerX = (int)(data[0] * mCVImage.cols );
                        int centerY = (int)(data[1] * mCVImage.rows );
                        int width = (int)(data[2] * mCVImage.cols );
                        int height = (int)(data[3] * mCVImage.rows );
                        int left = centerX - width/2;
                        int top = centerY - height/2;

                        classIds.push_back( classIdPoint.x );
                        confidences.push_back((float)confidence);
                        boxes.push_back(cv::Rect(left, top, width, height));
                    }
                }
            }
            if( outLayers.size() > 1 )
            {
                std::map< int, std::vector<size_t> > class2indices;
                for(size_t i = 0; i < classIds.size(); i++ )
                {
                    if( confidences[i] >= 0.7 )
                        class2indices[classIds[i]].push_back(i);
                }
                std::vector<cv::Rect> nmsBoxes;
                std::vector<float> nmsConfidences;
                std::vector<int> nmsClassIds;
                for( std::map<int, std::vector<size_t> >::iterator it = class2indices.begin(); it != class2indices.end(); ++ it )
                {
                    std::vector< cv::Rect > localBoxes;
                    std::vector< float > localConfidences;
                    std::vector< size_t > classIndices = it->second;
                    for( size_t i = 0; i < classIndices.size(); i++ )
                    {
                        localBoxes.push_back(boxes[classIndices[i]]);
                        localConfidences.push_back(confidences[classIndices[i]]);
                    }
                    std::vector<int> nmsIndices;
                    cv::dnn::NMSBoxes(localBoxes, localConfidences, 0.7, 0.4, nmsIndices);
                    for( size_t i = 0; i < nmsIndices.size(); i++ )
                    {
                        size_t idx = nmsIndices[i];
                        nmsBoxes.push_back(localBoxes[idx]);
                        nmsConfidences.push_back(localConfidences[idx]);
                        nmsClassIds.push_back(it->first);
                    }
                }
                boxes = nmsBoxes;
                classIds = nmsClassIds;
                confidences = nmsConfidences;
            }
            for( size_t idx = 0; idx < boxes.size(); ++idx )
            {
                cv::Rect box = boxes[idx];
                drawPrediction(classIds[idx], confidences[idx], box.x, box.y, box.x + box.width, box.y + box.height);
            }
        }
        /*double min, max;
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
        */
        Q_EMIT result_ready( mCVImage );
        mLockMutex.unlock();
    }
}


void
CVYoloDNNThread::
drawPrediction(int classId, float conf, int left, int top, int right, int bottom )
{
    cv::rectangle(mCVImage, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(0, 255, 0));

    std::string label = QString::number(conf).toStdString();
    if (!mvStrClasses.empty())
    {
        CV_Assert(classId < (int)mvStrClasses.size());
        label = mvStrClasses[classId] + ": " + label;
    }

    int baseLine;
    cv::Size labelSize = getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 1, 2, &baseLine);

    top = std::max(top, labelSize.height);
    cv::rectangle(mCVImage, cv::Point(left, top - labelSize.height),
              cv::Point(left + labelSize.width, top + baseLine), cv::Scalar::all(255), cv::FILLED);
    putText(mCVImage, label, cv::Point(left, top), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(), 2);
}

void
CVYoloDNNThread::
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
CVYoloDNNThread::
readNet( QString & model, QString & classes, QString & config )
{
    try {
        mCVYoloDNN = cv::dnn::readNet(model.toStdString(), config.toStdString());
        //mCVYoloDNN.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        //mCVYoloDNN.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        mCVYoloDNN.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        mCVYoloDNN.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

        mvStrOutNames = mCVYoloDNN.getUnconnectedOutLayersNames();

        std::ifstream ifs(classes.toStdString().c_str());
        if( ifs.is_open() )
        {
            mvStrClasses.clear();
            std::string line;
            while( std::getline( ifs, line ) )
                mvStrClasses.push_back(line);
            mbModelReady = true;
        }
    }  catch ( cv::Exception & e ) {
        mbModelReady = false;
    }
    return mbModelReady;
}

void
CVYoloDNNThread::
setParams(CVYoloDNNImageParameters & params)
{
    mParams = params;
}

CVYoloDNNModel::
CVYoloDNNModel()
    : PBNodeDataModel( _model_name )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >();
    mpSyncData->state() = true;

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msWeights_Filename;
    filePathPropertyType.msFilter = "*.weights";
    filePathPropertyType.msMode = "open";
    QString propId = "weights_filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Weight Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    filePathPropertyType.msFilename = msClasses_Filename;
    filePathPropertyType.msFilter = "*.txt";
    filePathPropertyType.msMode = "open";
    propId = "classes_filename";
    propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Classes Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    filePathPropertyType.msFilename = msConfig_Filename;
    filePathPropertyType.msFilter = "*.cfg";
    filePathPropertyType.msMode = "open";
    propId = "config_filename";
    propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Configuration Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdMin = 0.00001;
    doublePropertyType.mdMax = 10000.0;
    doublePropertyType.mdValue = 255.;
    propId = "inv_scale_factor";
    auto propInvScaleFactor = std::make_shared< TypedProperty< DoublePropertyType > >("Inverse Scale Factor", propId, QVariant::Double, doublePropertyType, "Image" );
    mvProperty.push_back( propInvScaleFactor );
    mMapIdToProperty[ propId ] = propInvScaleFactor;

    SizePropertyType sizePropertyType;
    sizePropertyType.miHeight = 416;
    sizePropertyType.miWidth = 416;
    propId = "size";
    auto propSize = std::make_shared< TypedProperty< SizePropertyType > >("Size", propId, QVariant::Size, sizePropertyType, "Image");
    mvProperty.push_back( propSize );
    mMapIdToProperty[ propId ] = propSize;

    propId = "swap_rb";
    auto propSwapRB = std::make_shared< TypedProperty< bool > >( "Swap RB", propId, QVariant::Bool, true, "Image" );
    mvProperty.push_back( propSwapRB );
    mMapIdToProperty[ propId ] = propSwapRB;
}

unsigned int
CVYoloDNNModel::
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
CVYoloDNNModel::
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
CVYoloDNNModel::
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
CVYoloDNNModel::
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
CVYoloDNNModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();
    QJsonObject cParams;
    cParams["weights_filename"] = msWeights_Filename;
    cParams["classes_filename"] = msClasses_Filename;
    cParams["config_filename"] = msConfig_Filename;
    auto params = mpCVYoloDNNThread->getParams();
    cParams["inv_scale_factor"] = params.mdInvScaleFactor;
    cParams["size_width"] = params.mCVSize.width;
    cParams["size_height"] = params.mCVSize.height;
    cParams["swab_rb"] = params.mbSwapRB;
    modelJson["cParams"] = cParams;
    return modelJson;
}


void
CVYoloDNNModel::
restore( QJsonObject const &p )
{
    PBNodeDataModel::restore( p );
    late_constructor();

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj["weights_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["weights_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >(prop);
            typedProp->getData() = v.toString();
            msWeights_Filename = v.toString();
        }

        v = paramsObj["classes_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["classes_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >( prop );
            typedProp->getData() = v.toString();
            msClasses_Filename = v.toString();
        }

        v = paramsObj["config_filename"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["config_filename"];
            auto typedProp = std::static_pointer_cast< TypedProperty<QString> >( prop );
            typedProp->getData() = v.toString();
            msConfig_Filename = v.toString();
        }

        CVYoloDNNImageParameters params;
        v = paramsObj["inv_scale_factor"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["inv_scale_factor"];
            auto typedProp = std::static_pointer_cast< TypedProperty<DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();
            params.mdInvScaleFactor = v.toDouble();
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

        v = paramsObj["swap_rb"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["swap_rb"];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            params.mbSwapRB = v.toBool();
        }

        mpCVYoloDNNThread->setParams( params );

        load_model();
    }
}


void
CVYoloDNNModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "weights_filename" || id == "classes_filename" || id == "config_filename" )
    {
        if( id == "weights_filename" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >(prop);
            typedProp->getData() = value.toString();
            msWeights_Filename = value.toString();
        }
        else if( id == "classes_filename" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = value.toString();
            msClasses_Filename = value.toString();
        }
        else if( id == "config_filename" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = value.toString();
            msConfig_Filename = value.toString();
        }

        load_model();
    }
    else
    {
        if( id == "inv_scale_factor" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = value.toDouble();

            auto params = mpCVYoloDNNThread->getParams();
            params.mdInvScaleFactor = value.toDouble();
            mpCVYoloDNNThread->setParams(params);
        }
        else if( id == "size" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = value.toSize().width();
            typedProp->getData().miHeight = value.toSize().height();

            auto params = mpCVYoloDNNThread->getParams();
            params.mCVSize = cv::Size( value.toSize().width(), value.toSize().height() );
            mpCVYoloDNNThread->setParams(params);
        }
        else if( id == "swap_rb" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = value.toBool();

            auto params = mpCVYoloDNNThread->getParams();
            params.mbSwapRB = value.toBool();
            mpCVYoloDNNThread->setParams(params);
        }
    }
}


void
CVYoloDNNModel::
late_constructor()
{
    if( !mpCVYoloDNNThread )
    {
        mpCVYoloDNNThread = new CVYoloDNNThread(this);
        connect( mpCVYoloDNNThread, &CVYoloDNNThread::result_ready, this, &CVYoloDNNModel::received_result );
        load_model();
        mpCVYoloDNNThread->start();
    }
}


void
CVYoloDNNModel::
received_result( cv::Mat & result )
{
    mpCVImageData->set_image( result );
    mpSyncData->state() = true;

    updateAllOutputPorts();
}

void
CVYoloDNNModel::
load_model()
{
    if( msWeights_Filename.isEmpty() || msClasses_Filename.isEmpty() || msConfig_Filename.isEmpty() )
        return;
    if( QFile::exists(msWeights_Filename) && QFile::exists( msClasses_Filename) && QFile::exists( msConfig_Filename) )
    {
        mpCVYoloDNNThread->readNet( msWeights_Filename, msClasses_Filename, msConfig_Filename );
    }
    else
    {
        QMessageBox err;
        err.setWindowTitle("Yolo DNN Error!");
        err.setText(caption() + " : Missing Files");
        QString sInformativeText = "Cannot load the following files ... \n";
        if( !QFile::exists(msWeights_Filename) )
            sInformativeText += "  - Weight File is missing!\n";
        if( !QFile::exists(msClasses_Filename) )
            sInformativeText += "  - Classes File is missing!\n";
        if( !QFile::exists(msConfig_Filename) )
            sInformativeText += "  - Config File is missing!\n";
        err.setInformativeText(sInformativeText);
        err.exec();
    }
}

void
CVYoloDNNModel::
processData(const std::shared_ptr< CVImageData > & in)
{
    cv::Mat& in_image = in->image();
    if( !in_image.empty() )
        mpCVYoloDNNThread->detect( in_image );
}

const QString CVYoloDNNModel::_category = QString("DNN");

const QString CVYoloDNNModel::_model_name = QString( "Yolo Object Detection" );
