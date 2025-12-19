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

#include "CVGaussianBlurModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QDebug>

#include <QtWidgets/QFileDialog>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVGaussianBlurModel::_category = QString( "Image Modification" );

const QString CVGaussianBlurModel::_model_name = QString( "CV Gaussian Blur" );

void CVGaussianBlurWorker::processFrame(cv::Mat input,
                                         CVGaussianBlurParameters params,
                                         FrameSharingMode mode,
                                         std::shared_ptr<CVImagePool> pool,
                                         long frameId,
                                         QString producerId)
{
    if(input.empty() || !(input.depth()==CV_8U || input.depth()==CV_16U || input.depth()==CV_16S || input.depth()==CV_32F || input.depth()==CV_64F))
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
            cv::GaussianBlur(input, handle.matrix(), params.mCVSizeKernel, params.mdSigmaX, params.mdSigmaY, params.miBorderType);
            if(!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if(!pooled)
    {
        cv::Mat result;
        cv::GaussianBlur(input, result, params.mCVSizeKernel, params.mdSigmaX, params.mdSigmaY, params.miBorderType);
        if(result.empty())
        {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}

CVGaussianBlurModel::
CVGaussianBlurModel()
    : PBAsyncDataModel( _model_name ),
      _minPixmap( ":/CVGaussianBlurModel.png" )
{
    qRegisterMetaType<CVGaussianBlurParameters>("CVGaussianBlurParameters");

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = mParams.mCVSizeKernel.width;
    sizePropertyType.miHeight = mParams.mCVSizeKernel.height;
    QString propId = "kernel_size";
    auto propKernelSize = std::make_shared< TypedProperty< SizePropertyType > >( "Kernel Size", propId, QMetaType::QSize, sizePropertyType , "Operation");
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdSigmaX;
    propId = "sigma_x";
    auto propSigmaX = std::make_shared< TypedProperty< DoublePropertyType > >( "Sigma X", propId, QMetaType::Double, doublePropertyType, "Operation" );
    mvProperty.push_back( propSigmaX );
    mMapIdToProperty[ propId ] = propSigmaX;

    doublePropertyType.mdValue = mParams.mdSigmaY;
    propId = "sigma_y";
    auto propSigmaY = std::make_shared< TypedProperty< DoublePropertyType > >( "Sigma Y", propId, QMetaType::Double, doublePropertyType, "Operation" );
    mvProperty.push_back( propSigmaY );
    mMapIdToProperty[ propId ] = propSigmaY;

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList( {"DEFAULT", "CONSTANT", "REPLICATE", "REFLECT", "WRAP", "TRANSPARENT", "ISOLATED"} );
    enumPropertyType.miCurrentIndex = 0;
    propId = "border_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Border Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;

}

QObject*
CVGaussianBlurModel::
createWorker()
{
    return new CVGaussianBlurWorker();
}

void
CVGaussianBlurModel::
connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVGaussianBlurWorker*>(worker);
    if (w) {
        connect(w, &CVGaussianBlurWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void
CVGaussianBlurModel::
dispatchPendingWork()
{
    if(!hasPendingWork())
        return;
    
    cv::Mat input = mPendingFrame;
    CVGaussianBlurParameters params = mPendingParams;
    setPendingWork(false);
    
    ensure_frame_pool(input.cols, input.rows, input.type());
    
    long frameId = getNextFrameId();
    QString producerId = getNodeId();
    
    std::shared_ptr<CVImagePool> poolCopy = getFramePool();
    
    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
        Qt::QueuedConnection,
        Q_ARG(cv::Mat, input.clone()),
        Q_ARG(CVGaussianBlurParameters, params),
        Q_ARG(FrameSharingMode, getSharingMode()),
        Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
        Q_ARG(long, frameId),
        Q_ARG(QString, producerId));
}

QJsonObject
CVGaussianBlurModel::
save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["kernelWidth"] = mParams.mCVSizeKernel.width;
    cParams["kernelHeight"] = mParams.mCVSizeKernel.height;
    cParams["sigmaX"] = mParams.mdSigmaX;
    cParams["sigmaY"] = mParams.mdSigmaY;
    cParams["borderType"] = mParams.miBorderType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVGaussianBlurModel::
load(QJsonObject const& p)
{
    PBAsyncDataModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue width = paramsObj[ "kernelWidth" ];
        QJsonValue height = paramsObj[ "kernelHeight" ];
        if( !width.isNull() && !height.isNull() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = width.toInt();
            typedProp->getData().miHeight = height.toInt();

            mParams.mCVSizeKernel = cv::Size( width.toInt(), height.toInt() );
        }
        QJsonValue v =  paramsObj[ "sigmaX" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "sigma_x" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdSigmaX = v.toDouble();
        }
        v = paramsObj[ "sigmaY" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "sigma_y" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdSigmaY = v.toDouble();
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
CVGaussianBlurModel::
setModelProperty( QString & id, const QVariant & value )
{
    if( !mMapIdToProperty.contains( id ) )
        return;

    if( id == "kernel_size" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
        QSize kSize =  value.toSize();
        bool adjValue = false;
        if( kSize.width()%2 != 1 )
        {
            kSize.setWidth( kSize.width() + 1 );
            adjValue = true;
        }
        if( kSize.height()%2 != 1 )
        {
            kSize.setHeight( kSize.height() + 1 );
            adjValue = true;
        }
        typedProp->getData().miWidth = kSize.width();
        typedProp->getData().miHeight = kSize.height();
        if( adjValue )
        {
            // Notify listeners that the entered kernel size was coerced
            // (made odd). Downstream UI will refresh to the corrected
            // value via this signal.
            Q_EMIT property_changed_signal( prop );
        }
        mParams.mCVSizeKernel = cv::Size( kSize.width(), kSize.height() );
    }
    else if( id == "sigma_x" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdSigmaX = value.toDouble();
    }
    else if( id == "sigma_y" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdSigmaY = value.toDouble();
    }
    else if( id == "border_type" )
    {
        auto prop = mMapIdToProperty[ id ];
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
        PBAsyncDataModel::setModelProperty( id, value );
        // No need to process_cached_input() here
        return;
    }
    // Process cached input if available
    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}
void
CVGaussianBlurModel::
process_cached_input()
{
    if( !mpCVImageInData || mpCVImageInData->data().empty() )
        return;
    
    cv::Mat input = mpCVImageInData->data();
    
    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });
    
    if(isWorkerBusy())
    {
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
            Q_ARG(CVGaussianBlurParameters, mParams),
            Q_ARG(FrameSharingMode, getSharingMode()),
            Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
            Q_ARG(long, frameId),
            Q_ARG(QString, producerId));
    }
}
