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

#include "CVFilter2DModel.hpp"

#include <QtCore/QTimer>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QDebug> //for debugging using qDebug()


#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

void CVFilter2DWorker::processFrame(cv::Mat input,
                                     CVFilter2DParameters params,
                                     FrameSharingMode mode,
                                     std::shared_ptr<CVImagePool> pool,
                                     long frameId,
                                     QString producerId)
{
    if(input.empty())
    {
        Q_EMIT frameReady(nullptr);
        return;
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    if(mode == FrameSharingMode::PoolMode && pool)
    {
        auto handle = pool->acquire(1, metadata);
        if(handle)
        {
            // Write directly to pool buffer - zero extra allocation
            cv::Mat temp;
            cv::filter2D(input, temp, params.miImageDepth, params.mMKKernel.image(),
                         cv::Point(-1,-1), params.mdDelta, params.miBorderType);
            cv::convertScaleAbs(temp, handle.matrix());
            if(!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if(!pooled)
    {
        cv::Mat result;
        cv::filter2D(input, result, params.miImageDepth, params.mMKKernel.image(),
                     cv::Point(-1,-1), params.mdDelta, params.miBorderType);
        cv::convertScaleAbs(result, result);
        if(result.empty())
        {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}

const QString CVFilter2DModel::_category = QString( "Image Modification" );

const QString CVFilter2DModel::_model_name = QString( "CV Filter 2D" );

const cv::Mat MatKernel::image() const//All kernels for CVFilter2D are defined here.
{
    CV_Assert(miKernelSize%2==1);
    const int center = (miKernelSize-1)/2;
    cv::Mat Output;
    switch(miKernelType)
    {
    case KERNEL_NULL:
        Output = cv::Mat::zeros(miKernelSize,miKernelSize,CV_32FC1);
        break;

    case KERNEL_LAPLACIAN:
        Output = cv::Mat(miKernelSize,miKernelSize,CV_32FC1,-1);
        Output.at<float>(center,center) = 8;
        break;

    case KERNEL_AVERAGE:
        Output = cv::Mat(miKernelSize,miKernelSize,CV_32FC1,1);
        Output *= 1.0/(miKernelSize*miKernelSize);
    }
    return Output;
}

CVFilter2DModel::
CVFilter2DModel()
    : PBAsyncDataModel( _model_name ),
      _minPixmap( ":/CVFilter2DModel.png" )
{
    qRegisterMetaType<std::shared_ptr<CVImageData>>("std::shared_ptr<CVImageData>");
    qRegisterMetaType<std::shared_ptr<CVImagePool>>("std::shared_ptr<CVImagePool>");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<FrameSharingMode>("FrameSharingMode");
    qRegisterMetaType<CVFilter2DParameters>("CVFilter2DParameters");

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"CV_8U", "CV_32F"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "image_depth";
    auto propImageDepth = std::make_shared< TypedProperty< EnumPropertyType > >( "Image Depth", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propImageDepth );
    mMapIdToProperty[ propId ] = propImageDepth;

    enumPropertyType.mslEnumNames = QStringList({"KERNEL_NULL", "KERNEL_LAPLACIAN", "KERNEL_AVERAGE"});
    enumPropertyType.miCurrentIndex = 0;
    propId = "kernel_type";
    auto propKernelType = std::make_shared< TypedProperty< EnumPropertyType > >( "Kernel Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation" );
    mvProperty.push_back( propKernelType );
    mMapIdToProperty[ propId ] = propKernelType;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.mMKKernel.miKernelSize;
    propId = "kernel_size";
    auto propKernelSize = std::make_shared< TypedProperty< IntPropertyType > >( "Kernel Size", propId, QMetaType::Int, intPropertyType , "Operation");
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdDelta;
    propId = "delta";
    auto propDelta = std::make_shared< TypedProperty < DoublePropertyType > > ("Delta", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propDelta );
    mMapIdToProperty[ propId ] = propDelta;

    enumPropertyType.mslEnumNames = QStringList( {"DEFAULT", "CONSTANT", "REPLICATE", "REFLECT", "WRAP", "TRANSPARENT", "ISOLATED"} );
    enumPropertyType.miCurrentIndex = 0;
    propId = "border_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Border Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;
}

QObject*
CVFilter2DModel::
createWorker()
{
    return new CVFilter2DWorker();
}

void
CVFilter2DModel::
connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVFilter2DWorker*>(worker);
    if (w) {
        connect(w, &CVFilter2DWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void
CVFilter2DModel::
dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown() || mPendingFrame.empty())
        return;

    cv::Mat input = mPendingFrame;
    CVFilter2DParameters params = mPendingParams;
    setPendingWork(false);

    ensure_frame_pool(input.cols, input.rows, input.type());

    long frameId = getNextFrameId();
    QString producerId = getNodeId();
    std::shared_ptr<CVImagePool> poolCopy = getFramePool();

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(CVFilter2DParameters, params),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

void
CVFilter2DModel::
process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;

    cv::Mat input = mpCVImageInData->data();

    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });

    if (isWorkerBusy())
    {
        // Store as pending - will be processed when worker finishes
        mPendingFrame = input.clone();
        mPendingParams = mParams;
        setPendingWork(true);
    }
    else
    {
        setWorkerBusy(true);

        ensure_frame_pool(input.cols, input.rows, input.type());

        long frameId = getNextFrameId();
        QString producerId = getNodeId();
        std::shared_ptr<CVImagePool> poolCopy = getFramePool();

        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(CVFilter2DParameters, mParams),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}

QJsonObject
CVFilter2DModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["imageDepth"] = mParams.miImageDepth;
    cParams["kernelType"] = mParams.mMKKernel.miKernelType;
    cParams["kernelSize"] = mParams.mMKKernel.miKernelSize;
    cParams["delta"] = mParams.mdDelta;
    cParams["borderType"] = mParams.miBorderType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVFilter2DModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "imageDepth" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "image_depth" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miImageDepth = v.toInt();
        }
        v =  paramsObj[ "kernelType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "kernel_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.mMKKernel.miKernelType = v.toInt();
        }
        v =  paramsObj[ "kernelSize" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.mMKKernel.miKernelSize = v.toInt();
        }
        v = paramsObj[ "delta" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "delta" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < DoublePropertyType > > (prop);
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdDelta = v.toDouble();
        }
        v = paramsObj[ "borderType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "border_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miBorderType = v.toInt();
        }
    }
}

void
CVFilter2DModel::
setModelProperty( QString & id, const QVariant & value )
{
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    
    if( id == "image_depth" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miImageDepth = CV_8U;
            break;

        case 1:
            mParams.miImageDepth = CV_32F;
            break;
        }
    }
    else if( id == "kernel_type" )
    {
        auto typedprop = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedprop->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.mMKKernel.miKernelType = MatKernel::KERNEL_NULL;
            break;

        case 1:
            mParams.mMKKernel.miKernelType = MatKernel::KERNEL_LAPLACIAN;
            break;

        case 2:
            mParams.mMKKernel.miKernelType = MatKernel::KERNEL_AVERAGE;
            break;
        }
    }
    else if( id == "kernel_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        int kSize =  value.toInt();
        bool adjValue = false;
        if( kSize%2 != 1 )
        {
            kSize += 1;
            adjValue = true;
        }
        if( adjValue )
        {
            typedProp->getData().miValue = kSize;

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = value.toInt();

            mParams.mMKKernel.miKernelSize = value.toInt();
        }
    }
    else if( id == "delta" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdDelta = value.toDouble();
    }
    else if( id == "border_type" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        switch( value.toInt() )
        {
        case 0: // DEFAULT
            mParams.miBorderType = cv::BORDER_DEFAULT;
            break;
        case 1: // CONSTANT
            mParams.miBorderType = cv::BORDER_CONSTANT;
            break;
        case 2: // REPLICATE
            mParams.miBorderType = cv::BORDER_REPLICATE;
            break;
        case 3: // REFLECT
            mParams.miBorderType = cv::BORDER_REFLECT;
            break;
        case 4: // WRAP
            mParams.miBorderType = cv::BORDER_WRAP;
            break;
        case 5: // TRANSPARENT
            mParams.miBorderType = cv::BORDER_TRANSPARENT;
            break; //Bug occured when this case is executed
        case 6: // ISOLATED
            mParams.miBorderType = cv::BORDER_ISOLATED;
            break;
        }
    }
    else
    {
        // Base class handles pool_size and sharing_mode
        // Need to call base class to handle pool_size and sharing_mode
        PBAsyncDataModel::setModelProperty(id, value);
        // No need to process_cached_input() here
        return;
    }
    // Process cached input if available
    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}
