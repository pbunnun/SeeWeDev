#include "TextDetectionDNNModel.hpp"

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>

#include "qtvariantproperty.h"
#include <QFile>

TextDetectionDBThread::TextDetectionDBThread( QObject * parent )
    : QThread(parent)
{

}


TextDetectionDBThread::
~TextDetectionDBThread()
{
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}


void
TextDetectionDBThread::
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
        std::vector<std::vector<cv::Point>> results;
        mTextDetectionDNN.detect(mCVImage, results);
        cv::polylines(mCVImage, results, true, cv::Scalar(0, 255, 0), 2);
        Q_EMIT result_ready( mCVImage );
        mLockMutex.unlock();
    }
}


void
TextDetectionDBThread::
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
TextDetectionDBThread::
readNet( QString & model )
{
    try {
        mTextDetectionDNN = cv::dnn::TextDetectionModel_DB(model.toStdString());
        //mTextDetectionDNN.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        //mTextDetectionDNN.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);

        mTextDetectionDNN.setBinaryThreshold(mParams.mfBinaryThreshold)
            .setPolygonThreshold(mParams.mfPolygonThreshold)
            .setUnclipRatio(mParams.mdUnclipRatio)
            .setMaxCandidates(mParams.miMaxCandidate);
        double scale = 1.0/255.0;
        cv::Scalar mean = cv::Scalar(122.67891434, 166.66876762, 104.00698793);
        mTextDetectionDNN.setInputParams(scale, mParams.mCVSize, mean);

        mbModelReady = true;
    }  catch ( cv::Exception & e ) {
        mbModelReady = false;
    }
    return mbModelReady;
}

void
TextDetectionDBThread::
setParams(TextDetectionDBParameters & params)
{
    mParams = params;
    mTextDetectionDNN.setBinaryThreshold(mParams.mfBinaryThreshold)
            .setPolygonThreshold(mParams.mfPolygonThreshold)
            .setUnclipRatio(mParams.mdUnclipRatio)
            .setMaxCandidates(mParams.miMaxCandidate);
    double scale = 1.0/255.0;
    cv::Scalar mean = cv::Scalar(122.67891434, 166.66876762, 104.00698793);
    mTextDetectionDNN.setInputParams(scale, mParams.mCVSize, mean);
}

TextDetectionDNNModel::
TextDetectionDNNModel()
    : PBNodeDataModel( _model_name )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpSyncData = std::make_shared< SyncData >();

    FilePathPropertyType filePathPropertyType;
    filePathPropertyType.msFilename = msDBModel_Filename;
    filePathPropertyType.msFilter = "*.onnx";
    filePathPropertyType.msMode = "open";
    QString propId = "model_filename";
    auto propFileName = std::make_shared< TypedProperty<FilePathPropertyType> >("Model Filename", propId, QtVariantPropertyManager::filePathTypeId(), filePathPropertyType);
    mvProperty.push_back( propFileName );
    mMapIdToProperty[ propId ] = propFileName;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdMin = 0.00001;
    doublePropertyType.mdMax = 10000.0;
    doublePropertyType.mdValue = 0.3;
    propId = "binary_threshold";
    auto propBinaryThreshold = std::make_shared< TypedProperty< DoublePropertyType > >("Binary Threshold", propId, QVariant::Double, doublePropertyType );
    mvProperty.push_back( propBinaryThreshold );
    mMapIdToProperty[ propId ] = propBinaryThreshold;

    doublePropertyType.mdValue = 0.5;
    propId = "polygon_threshold";
    auto propPolygonThreshold = std::make_shared< TypedProperty< DoublePropertyType > >("Polygon Threshold", propId, QVariant::Double, doublePropertyType );
    mvProperty.push_back( propPolygonThreshold );
    mMapIdToProperty[ propId ] = propPolygonThreshold;

    doublePropertyType.mdValue = 2.0;
    propId = "unclip_ratio";
    auto propUnclipRatio = std::make_shared< TypedProperty< DoublePropertyType > >("Unclip Ratio", propId, QVariant::Double, doublePropertyType );
    mvProperty.push_back( propUnclipRatio );
    mMapIdToProperty[ propId ] = propUnclipRatio;

    SizePropertyType sizePropertyType;
    sizePropertyType.miHeight = 736;
    sizePropertyType.miWidth = 736;
    propId = "input_size";
    auto propInputSize = std::make_shared< TypedProperty< SizePropertyType > >("Input Size", propId, QVariant::Size, sizePropertyType );
    mvProperty.push_back( propInputSize );
    mMapIdToProperty[ propId ] = propInputSize;

    IntPropertyType intPropertyType;
    intPropertyType.miMin = 1;
    intPropertyType.miMax = 10000;
    intPropertyType.miValue = 200;
    propId = "max_candidate";
    auto propMaxCandidate = std::make_shared< TypedProperty< IntPropertyType > >("Max Candidate", propId, QVariant::Int, intPropertyType );
    mvProperty.push_back( propMaxCandidate );
    mMapIdToProperty[ propId ] = propMaxCandidate;
}

unsigned int
TextDetectionDNNModel::
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
TextDetectionDNNModel::
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
TextDetectionDNNModel::
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
TextDetectionDNNModel::
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
TextDetectionDNNModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();
    QJsonObject cParams;
    cParams["model_filename"] = msDBModel_Filename;
    auto params = mpTextDetectionDNNThread->getParams();
    cParams["binary_threshold"] = params.mfBinaryThreshold;
    cParams["polygon_threshold"] = params.mfPolygonThreshold;
    cParams["unclip_ratio"] = params.mdUnclipRatio;
    cParams["max_candidate"] = params.miMaxCandidate;
    cParams["size_width"] = params.mCVSize.width;
    cParams["size_height"] = params.mCVSize.height;
    modelJson["cParams"] = cParams;
    return modelJson;
}


void
TextDetectionDNNModel::
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
            msDBModel_Filename = v.toString();
        }

        TextDetectionDBParameters params;
        v = paramsObj["binary_threshold"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["binary_threshold"];
            auto typedProp = std::static_pointer_cast< TypedProperty<DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();
            params.mfBinaryThreshold = v.toDouble();
        }

        v = paramsObj["polygon_threshold"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["polygon_threshold"];
            auto typedProp = std::static_pointer_cast< TypedProperty<DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();
            params.mfPolygonThreshold = v.toDouble();
        }

        v = paramsObj["unclip_ratio"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["unclip_ratio"];
            auto typedProp = std::static_pointer_cast< TypedProperty<DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();
            params.mdUnclipRatio = v.toDouble();
        }

        v = paramsObj["max_candidate"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["max_candidate"];
            auto typedProp = std::static_pointer_cast< TypedProperty<IntPropertyType> >( prop );
            typedProp->getData().miValue = v.toInt();
            params.miMaxCandidate = v.toDouble();
        }

        auto width = paramsObj["size_width"];
        auto height = paramsObj["size_height"];
        if( !width.isNull() && !height.isNull() )
        {
            auto prop = mMapIdToProperty["input_size"];
            auto typedProp = std::static_pointer_cast< TypedProperty<SizePropertyType> >( prop );
            typedProp->getData().miWidth = width.toInt();
            typedProp->getData().miHeight = height.toInt();

            params.mCVSize = cv::Size( width.toInt(), height.toInt() );
        }

        mpTextDetectionDNNThread->setParams( params );

        load_model();
    }
}


void
TextDetectionDNNModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "model_filename" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >(prop);
        typedProp->getData() = value.toString();
        msDBModel_Filename = value.toString();
        load_model();
    }
    else
    {
        if( id == "binary_threshold" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = value.toDouble();

            auto params = mpTextDetectionDNNThread->getParams();
            params.mfBinaryThreshold = value.toDouble();
            mpTextDetectionDNNThread->setParams(params);
        }
        else if( id == "polygon_threshold" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = value.toDouble();

            auto params = mpTextDetectionDNNThread->getParams();
            params.mfPolygonThreshold = value.toDouble();
            mpTextDetectionDNNThread->setParams(params);
        }
        else if( id == "unclip_ratio" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = value.toDouble();

            auto params = mpTextDetectionDNNThread->getParams();
            params.mdUnclipRatio = value.toDouble();
            mpTextDetectionDNNThread->setParams(params);
        }
        else if( id == "max_candidate" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = value.toInt();

            auto params = mpTextDetectionDNNThread->getParams();
            params.miMaxCandidate = value.toDouble();
            mpTextDetectionDNNThread->setParams(params);
        }
        else if( id == "input_size" )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = value.toSize().width();
            typedProp->getData().miHeight = value.toSize().height();

            auto params = mpTextDetectionDNNThread->getParams();
            params.mCVSize = cv::Size( value.toSize().width(), value.toSize().height() );
            mpTextDetectionDNNThread->setParams(params);
        }
    }
}


void
TextDetectionDNNModel::
late_constructor()
{
    if( !mpTextDetectionDNNThread )
    {
        mpTextDetectionDNNThread = new TextDetectionDBThread(this);
        connect( mpTextDetectionDNNThread, &TextDetectionDBThread::result_ready, this, &TextDetectionDNNModel::received_result );
        load_model();
        mpTextDetectionDNNThread->start();
    }
}


void
TextDetectionDNNModel::
received_result( cv::Mat & result )
{
    mpCVImageData->set_image( result );
    mpSyncData->state() = true;

    updateAllOutputPorts();
}

void
TextDetectionDNNModel::
load_model()
{
    if( msDBModel_Filename.isEmpty() )
        return;
    if( QFile::exists(msDBModel_Filename) )
    {
        qDebug() <<"Load Model";
        mpTextDetectionDNNThread->readNet( msDBModel_Filename );
    }
}

void
TextDetectionDNNModel::
processData(const std::shared_ptr< CVImageData > & in)
{
    cv::Mat& in_image = in->image();
    if( !in_image.empty() )
        mpTextDetectionDNNThread->detect( in_image );
}

const QString TextDetectionDNNModel::_category = QString("DNN");

const QString TextDetectionDNNModel::_model_name = QString( "Text Detection Model" );
