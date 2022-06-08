#include "MorphologicalTransformationModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

MorphologicalTransformationModel::
MorphologicalTransformationModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":MorphologicalTransformation.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());

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
    auto propKernelSize = std::make_shared< TypedProperty< SizePropertyType > >( "Kernel Size", propId, QVariant::Size, sizePropertyType, "Operation" );
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    PointPropertyType pointPropertyType; //need additional type support from the function displaying properties in the UI.
    pointPropertyType.miXPosition = mParams.mCVPointAnchor.x;
    pointPropertyType.miYPosition = mParams.mCVPointAnchor.y;
    propId = "anchor_point";
    auto propAnchorPoint = std::make_shared< TypedProperty< PointPropertyType > >( "Anchor Point", propId, QVariant::Point, pointPropertyType ,"Operation");
    mvProperty.push_back( propAnchorPoint );
    mMapIdToProperty[ propId ] = propAnchorPoint;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miIteration;
    propId = "iteration";
    auto propIteration = std::make_shared<TypedProperty<IntPropertyType>>("Iterations", propId, QVariant::Int, intPropertyType, "Operation");
    mvProperty.push_back(propIteration);
    mMapIdToProperty[propId] = propIteration;

    enumPropertyType.mslEnumNames = QStringList( {"DEFAULT", "CONSTANT", "REPLICATE", "REFLECT", "WRAP", "TRANSPARENT", "ISOLATED"} );
    enumPropertyType.miCurrentIndex = 0;
    propId = "border_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Border Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;
}

unsigned int
MorphologicalTransformationModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 1;
        break;

    default:
        break;
    }

    return result;
}


NodeDataType
MorphologicalTransformationModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
MorphologicalTransformationModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
MorphologicalTransformationModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData(mpCVImageInData,mpCVImageData,mParams);
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
MorphologicalTransformationModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

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
MorphologicalTransformationModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "morphMethod" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "morph_method" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miMorphMethod = v.toInt();
        }

        v = paramsObj[ "kernelShape" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "kernel_shape" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miKernelShape = v.toInt();
        }

        QJsonValue argX = paramsObj[ "kernelWidth" ];
        QJsonValue argY = paramsObj[ "kernelHeight" ];
        if( !argX.isUndefined() && !argY.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = argX.toInt();
            typedProp->getData().miHeight = argY.toInt();

            mParams.mCVSizeKernel = cv::Size( argX.toInt(), argY.toInt() );
        }
        argX = paramsObj[ "anchorX" ];
        argY = paramsObj[ "anchorY" ];
        if( !argX.isUndefined() && ! argY.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "anchor_point" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
            typedProp->getData().miXPosition = argX.toInt();
            typedProp->getData().miYPosition = argY.toInt();

            mParams.mCVPointAnchor = cv::Point(argX.toInt(),argY.toInt());
        }
        v = paramsObj[ "iteration" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty["iteration"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miIteration = v.toInt();
        }
        v = paramsObj[ "borderType" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "border_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            mParams.miBorderType = v.toInt();
        }
    }
}

void
MorphologicalTransformationModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

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
    if( mpCVImageInData )
    {
        processData(mpCVImageInData,mpCVImageData,mParams);

        Q_EMIT dataUpdated(0);
    }
}

void MorphologicalTransformationModel::processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<CVImageData> &out, const MorphologicalTransformationParameters &params)
{
    cv::Mat& in_image = in->image();
    if(!in_image.empty() && (in_image.depth()==CV_8U || in_image.depth()==CV_16U || in_image.depth()==CV_16S || in_image.depth()==CV_32F || in_image.depth()==CV_64F))
    {
        cv::Mat Kernel = cv::getStructuringElement(params.miKernelShape,params.mCVSizeKernel,params.mCVPointAnchor);
        cv::morphologyEx(in_image,out->image(),params.miMorphMethod,Kernel,params.mCVPointAnchor,params.miIteration,params.miBorderType);
    }
}

const QString MorphologicalTransformationModel::_category = QString( "Image Modification" );

const QString MorphologicalTransformationModel::_model_name = QString( "Morph Transformation" );
