#include "GaussianBlurModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"

GaussianBlurModel::
GaussianBlurModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":GaussianBlur.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = mParams.mCVSizeKernel.width;
    sizePropertyType.miHeight = mParams.mCVSizeKernel.height;
    QString propId = "kernel_size";
    auto propKernelSize = std::make_shared< TypedProperty< SizePropertyType > >( "Kernel Size", propId, QVariant::Size, sizePropertyType , "Operation");
    mvProperty.push_back( propKernelSize );
    mMapIdToProperty[ propId ] = propKernelSize;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdSigmaX;
    propId = "sigma_x";
    auto propSigmaX = std::make_shared< TypedProperty< DoublePropertyType > >( "Sigma X", propId, QVariant::Double, doublePropertyType, "Operation" );
    mvProperty.push_back( propSigmaX );
    mMapIdToProperty[ propId ] = propSigmaX;

    doublePropertyType.mdValue = mParams.mdSigmaY;
    propId = "sigma_y";
    auto propSigmaY = std::make_shared< TypedProperty< DoublePropertyType > >( "Sigma Y", propId, QVariant::Double, doublePropertyType, "Operation" );
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

unsigned int
GaussianBlurModel::
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
GaussianBlurModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
GaussianBlurModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
GaussianBlurModel::
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
GaussianBlurModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

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
GaussianBlurModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue width = paramsObj[ "kernelWidth" ];
        QJsonValue height = paramsObj[ "kernelHeight" ];
        if( !width.isUndefined() && !height.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "kernel_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = width.toInt();
            typedProp->getData().miHeight = height.toInt();

            mParams.mCVSizeKernel = cv::Size( width.toInt(), height.toInt() );
        }
        QJsonValue v =  paramsObj[ "sigmaX" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "sigma_x" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdSigmaX = v.toDouble();
        }
        v = paramsObj[ "sigmaY" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "sigma_y" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdSigmaY = v.toDouble();
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
GaussianBlurModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "kernel_size" )
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
    else if( id == "sigma_x" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdSigmaX = value.toDouble();
    }
    else if( id == "sigma_y" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdSigmaY = value.toDouble();
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
        processData(mpCVImageInData,mpCVImageData,mParams);

        Q_EMIT dataUpdated(0);
    }
}

void GaussianBlurModel::processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<CVImageData> &out, const GaussianBlurParameters &params)
{
    cv::Mat& in_image = in->image();
    if(!in_image.empty() && (in_image.depth()==CV_8U || in_image.depth()==CV_16U || in_image.depth()==CV_16S || in_image.depth()==CV_32F || in_image.depth()==CV_64F))
    {
        cv::GaussianBlur(in->image(),out->image(),params.mCVSizeKernel,params.mdSigmaX,params.mdSigmaY,params.miBorderType);
    }
}

const QString GaussianBlurModel::_category = QString( "Image Modification" );

const QString GaussianBlurModel::_model_name = QString( "Gaussian Blur" );
