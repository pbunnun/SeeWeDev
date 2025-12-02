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

#include "CVMorphologicalTransformationModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QDebug>

#include <QtWidgets/QFileDialog>


#include <opencv2/imgproc.hpp>
#include "qtvariantproperty_p.h"

const QString CVMorphologicalTransformationModel::_category = QString( "Image Modification" );

const QString CVMorphologicalTransformationModel::_model_name = QString( "CV Morph Transformation" );

void CVMorphologicalTransformationWorker::processFrame(cv::Mat input,
                                                       MorphologicalTransformationParameters params,
                                                       FrameSharingMode mode,
                                                       std::shared_ptr<CVImagePool> pool,
                                                       long frameId,
                                                       QString producerId)
{
    if (input.empty())
    {
        Q_EMIT frameReady(nullptr);
        return;
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    cv::Size ksize(params.mCVSizeKernel.width, params.mCVSizeKernel.height);
    cv::Point anchor(params.mCVPointAnchor.x, params.mCVPointAnchor.y);
    cv::Mat kernel = cv::getStructuringElement(params.miKernelShape, ksize, anchor);

    if (mode == FrameSharingMode::PoolMode && pool)
    {
        auto handle = pool->acquire(1, metadata);
        if (handle)
        {
            cv::morphologyEx(input, handle.matrix(), params.miMorphMethod, kernel, anchor, params.miIteration, params.miBorderType);
            if (!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if (!pooled)
    {
        cv::Mat result;
        cv::morphologyEx(input, result, params.miMorphMethod, kernel, anchor, params.miIteration, params.miBorderType);
        if (result.empty())
        {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}

CVMorphologicalTransformationModel::
CVMorphologicalTransformationModel()
    : PBAsyncDataModel( _model_name ),
      _minPixmap( ":MorphologicalTransformation.png" )
{
    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"MORPH_OPEN", "MORPH_CLOSE", "MORPH_GRADIENT", "MORPH_TOPHAT", "MORPH_BLACKHAT"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "morph_method";
    auto propMorphMethod = std::make_shared<TypedProperty<EnumPropertyType>>("Iterations", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back(propMorphMethod);
    mMapIdToProperty[propId] = propMorphMethod;

    enumPropertyType.mslEnumNames = QStringList( {"MORPH_RECT", "MORPH_CROSS", "MORTH_ELLIPSE"} );
    enumPropertyType.miCurrentIndex = 0;
    propId = "kernel_shape";
    auto propKernelShape = std::make_shared< TypedProperty< EnumPropertyType > >( "Kernel Shape", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation" );
    mvProperty.push_back( propKernelShape );
    mMapIdToProperty[ propId ] = propKernelShape;

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = mParams.mCVSizeKernel.width;
    sizePropertyType.miHeight = mParams.mCVSizeKernel.height;
    propId = "kernel_size";
    auto propKernelSize = std::make_shared< TypedProperty< SizePropertyType > >( "Kernel Size", propId, QMetaType::QSize, sizePropertyType, "Operation" );
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    PointPropertyType pointPropertyType; //need additional type support from the function displaying properties in the UI.
    pointPropertyType.miXPosition = mParams.mCVPointAnchor.x;
    pointPropertyType.miYPosition = mParams.mCVPointAnchor.y;
    propId = "anchor_point";
    auto propAnchorPoint = std::make_shared< TypedProperty< PointPropertyType > >( "Anchor Point", propId, QMetaType::QPoint, pointPropertyType ,"Operation");
    mvProperty.push_back( propAnchorPoint );
    mMapIdToProperty[ propId ] = propAnchorPoint;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miIteration;
    propId = "iteration";
    auto propIteration = std::make_shared<TypedProperty<IntPropertyType>>("Iterations", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propIteration);
    mMapIdToProperty[propId] = propIteration;

    enumPropertyType.mslEnumNames = QStringList( {"DEFAULT", "CONSTANT", "REPLICATE", "REFLECT", "WRAP", "TRANSPARENT", "ISOLATED"} );
    enumPropertyType.miCurrentIndex = 0;
    propId = "border_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Border Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;

    qRegisterMetaType<MorphologicalTransformationParameters>("MorphologicalTransformationParameters");
}

QObject*
CVMorphologicalTransformationModel::
createWorker()
{
    return new CVMorphologicalTransformationWorker();
}

void
CVMorphologicalTransformationModel::
connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVMorphologicalTransformationWorker*>(worker);
    if (w) {
        connect(w, &CVMorphologicalTransformationWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}

void
CVMorphologicalTransformationModel::
dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;

    cv::Mat input = mPendingFrame;
    MorphologicalTransformationParameters params = mPendingParams;
    setPendingWork(false);

    ensure_frame_pool(input.cols, input.rows, input.type());

    long frameId = getNextFrameId();
    QString producerId = getNodeId();

    std::shared_ptr<CVImagePool> poolCopy = getFramePool();

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input.clone()),
                              Q_ARG(MorphologicalTransformationParameters, params),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
}

QJsonObject
CVMorphologicalTransformationModel::
save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["morphMethod"] = mParams.miMorphMethod;
    cParams["kernelShape"] = mParams.miKernelShape;
    cParams["kernelWidth"] = mParams.mCVSizeKernel.width;
    cParams["kernelHeight"] = mParams.mCVSizeKernel.height;
    cParams["anchorX"] = mParams.mCVPointAnchor.x;
    cParams["anchorY"] = mParams.mCVPointAnchor.y;
    cParams["iteration"] = mParams.miIteration;
    cParams["borderType"] = mParams.miBorderType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVMorphologicalTransformationModel::
load(QJsonObject const& p)
{
    PBAsyncDataModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "morphMethod" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "morph_method" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miMorphMethod = v.toInt();
        }

        v = paramsObj[ "kernelShape" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "kernel_shape" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miKernelShape = v.toInt();
        }

        QJsonValue argX = paramsObj[ "kernelWidth" ];
        QJsonValue argY = paramsObj[ "kernelHeight" ];
        if( !argX.isNull() && !argY.isNull() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = argX.toInt();
            typedProp->getData().miHeight = argY.toInt();

            mParams.mCVSizeKernel = cv::Size( argX.toInt(), argY.toInt() );
        }
        argX = paramsObj[ "anchorX" ];
        argY = paramsObj[ "anchorY" ];
        if( !argX.isNull() && ! argY.isNull() )
        {
            auto prop = mMapIdToProperty[ "anchor_point" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = argX.toInt();
            typedProp->getData().miYPosition = argY.toInt();

            mParams.mCVPointAnchor = cv::Point(argX.toInt(),argY.toInt());
        }
        v = paramsObj[ "iteration" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["iteration"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miIteration = v.toInt();
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
CVMorphologicalTransformationModel::
setModelProperty( QString & id, const QVariant & value )
{
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "morph_method" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < EnumPropertyType >>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        switch( value.toInt() )
        {
        case 0:
            mParams.miMorphMethod = cv::MORPH_OPEN;
            break;
        case 1:
            mParams.miMorphMethod = cv::MORPH_CLOSE;
            break;
        case 2:
            mParams.miMorphMethod = cv::MORPH_GRADIENT;
            break;
        case 3:
            mParams.miMorphMethod = cv::MORPH_TOPHAT;
            break;
        case 4:
            mParams.miMorphMethod = cv::MORPH_BLACKHAT;
            break;
        }
    }
    else if( id == "kernel_shape" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <EnumPropertyType > >(prop);
        typedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt())
        {
        case 0:
            mParams.miKernelShape = cv::MORPH_RECT;
            break;

        case 1:
            mParams.miKernelShape = cv::MORPH_CROSS;
            break;

        case 2:
            mParams.miKernelShape = cv::MORPH_ELLIPSE;
            break;
        }
    }
    else if( id == "kernel_size" )
    {
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
        if( adjValue )
        {
            typedProp->getData().miWidth = kSize.width();
            typedProp->getData().miHeight = kSize.height();

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = kSize.width();
            typedProp->getData().miHeight = kSize.height();

            mParams.mCVSizeKernel = cv::Size( kSize.width(), kSize.height() );
        }
    }
    else if( id == "anchor_point" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        QPoint aPoint =  value.toPoint();
        bool adjValue = false;
        if( aPoint.x() > (mParams.mCVSizeKernel.width-1)/2 ) //Size members are gauranteed to be odd numbers.
        {
            aPoint.setX((mParams.mCVSizeKernel.width-1)/2);
            adjValue = true;
        }
        else if( aPoint.x() < -(mParams.mCVSizeKernel.width-1)/2)
        {
            aPoint.setX(-(mParams.mCVSizeKernel.width-1)/2);
            adjValue = true;
        }
        if( aPoint.y() > (mParams.mCVSizeKernel.height-1)/2 )
        {
            aPoint.setY((mParams.mCVSizeKernel.height-1)/2);
            adjValue = true;
        }
        else if( aPoint.y() < -(mParams.mCVSizeKernel.height-1)/2)
        {
            aPoint.setY(-(mParams.mCVSizeKernel.height-1)/2);
            adjValue = true;
        }
        if( adjValue )
        {
            typedProp->getData().miXPosition = aPoint.x();
            typedProp->getData().miYPosition = aPoint.y();

            Q_EMIT property_changed_signal( prop );
            return;
        }
        else
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = aPoint.x();
            typedProp->getData().miYPosition = aPoint.y();

            mParams.mCVPointAnchor = cv::Point( aPoint.x(), aPoint.y() );
        }
    }
    else if( id == "iterations" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < IntPropertyType >>(prop);
        typedProp->getData().miValue = value.toInt();

        mParams.miIteration = value.toInt();
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
            break;
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

void CVMorphologicalTransformationModel::process_cached_input()
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

        MorphologicalTransformationParameters params = mParams;
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                  Qt::QueuedConnection,
                                  Q_ARG(cv::Mat, input.clone()),
                                  Q_ARG(MorphologicalTransformationParameters, params),
                                  Q_ARG(FrameSharingMode, getSharingMode()),
                                  Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                                  Q_ARG(long, frameId),
                                  Q_ARG(QString, producerId));
    }
}


