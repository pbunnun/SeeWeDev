#include "Filter2DModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

const cv::Mat MatKernel::image() const//All kernels for Filter2D are defined here.
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

Filter2DModel::
Filter2DModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":Filter2D.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

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
    auto propKernelSize = std::make_shared< TypedProperty< IntPropertyType > >( "Kernel Size", propId, QVariant::Int, intPropertyType , "Operation");
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdDelta;
    propId = "delta";
    auto propDelta = std::make_shared< TypedProperty < DoublePropertyType > > ("Delta", propId, QVariant::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propDelta );
    mMapIdToProperty[ propId ] = propDelta;

    enumPropertyType.mslEnumNames = QStringList( {"DEFAULT", "CONSTANT", "REPLICATE", "REFLECT", "WRAP", "TRANSPARENT", "ISOLATED"} );
    enumPropertyType.miCurrentIndex = 0;
    propId = "border_type";
    auto propBorderType = std::make_shared< TypedProperty< EnumPropertyType > >( "Border Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display" );
    mvProperty.push_back( propBorderType );
    mMapIdToProperty[ propId ] = propBorderType;
}

unsigned int
Filter2DModel::
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
Filter2DModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
Filter2DModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
Filter2DModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mpCVImageData, mParams );
        }
    }

    Q_EMIT dataUpdated(0);
}

QJsonObject
Filter2DModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

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
Filter2DModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "imageDepth" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "image_depth" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miImageDepth = v.toInt();
        }
        v =  paramsObj[ "kernelType" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "kernel_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.mMKKernel.miKernelType = v.toInt();
        }
        v =  paramsObj[ "kernelSize" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.mMKKernel.miKernelSize = v.toInt();
        }
        v = paramsObj[ "delta" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "delta" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < DoublePropertyType > > (prop);
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdDelta = v.toDouble();
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
Filter2DModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

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

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }
}

void
Filter2DModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr<CVImageData> & out,
            const Filter2DParameters & params )
{
    if(!in->image().empty())
    {
        cv::filter2D(in->image(),
                     out->image(),
                     params.miImageDepth,
                     params.mMKKernel.image(),
                     cv::Point(-1,-1),
                     params.mdDelta,
                     params.miBorderType);
        cv::convertScaleAbs(out->image(),out->image());
    }
}

const QString Filter2DModel::_category = QString( "Image Modification" );

const QString Filter2DModel::_model_name = QString( "Filter 2D" );
