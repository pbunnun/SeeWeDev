#include "CVImagePropertiesModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

CVImagePropertiesModel::
CVImagePropertiesModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap(":ImageDisplay.png")
{
    QString propId = "image_name";
    QString imageName = QString::fromStdString(mProps.msImageName);
    auto propImageName = std::make_shared< TypedProperty< QString > >( "Name", propId, QVariant::String, imageName, "Properties");
    mvProperty.push_back( propImageName );
    mMapIdToProperty[ propId ] = propImageName;

    propId = "image_channels";
    auto propChannels = std::make_shared< TypedProperty< QString > >( "Channels", propId, QVariant::String, QString("%1").arg(mProps.miChannels), "Properties");
    mvProperty.push_back( propChannels );
    mMapIdToProperty[ propId ] = propChannels;

    propId = "image_size";
    QString imageSize = QString("%1 px x %2 px").arg(mProps.mCVMSizeImage.height).arg(mProps.mCVMSizeImage.width);
    auto propImageSize = std::make_shared< TypedProperty <QString>>("Size", propId, QVariant::String, imageSize, "Properties");
    mvProperty.push_back(propImageSize);
    mMapIdToProperty[propId] = propImageSize;

    propId = "is_binary";
    QString isBinary = mProps.mbIsBinary? "Yes" : "No" ;
    auto propIsBinary = std::make_shared< TypedProperty <QString>>("Binary", propId, QVariant::String, isBinary, "Properties");
    mvProperty.push_back(propIsBinary);
    mMapIdToProperty[propId] = propIsBinary;

    propId = "is_bandw";
    QString isBAndW = mProps.mbIsBAndW? "Yes" : "No" ;
    auto propIsBAndW = std::make_shared< TypedProperty <QString>>("Black and White", propId, QVariant::String, isBAndW, "Properties");
    mvProperty.push_back(propIsBAndW);
    mMapIdToProperty[propId] = propIsBAndW;

    propId = "is_continuous";
    QString isContinuous = mProps.mbIsContinuous? "Yes" : "No" ;
    auto propIsContinuous = std::make_shared< TypedProperty < QString >>("Continuous", propId, QVariant::String, isContinuous, "Properties");
    mvProperty.push_back(propIsContinuous);
    mMapIdToProperty[propId] = propIsContinuous;

    propId = "description";
    QString description = QString::fromStdString(mProps.msDescription);
    auto propDescription = std::make_shared< TypedProperty <QString>>("Description", propId, QVariant::String, description, "Properties");
    mvProperty.push_back(propDescription);
    mMapIdToProperty[propId] = propDescription;
}

unsigned int
CVImagePropertiesModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 1;
    else
        return 0;
}

NodeDataType
CVImagePropertiesModel::
dataType( PortType, PortIndex ) const
{
    return CVImageData().type();
}

void
CVImagePropertiesModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mProps);
        }
    }
}

QJsonObject
CVImagePropertiesModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["imageName"] = QString::fromStdString(mProps.msImageName);
    cParams["description"] = QString::fromStdString(mProps.msDescription);
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVImagePropertiesModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "imageName" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "image_name" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = v.toString();

            mProps.msImageName = v.toString().toStdString();
        }
        v =  paramsObj[ "description" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "description" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = v.toString();

            mProps.msDescription = v.toString().toStdString();
        }
    }
}

void
CVImagePropertiesModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "image_name" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();

        mProps.msImageName = value.toString().toStdString();
    }
    else if( id == "description" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();

        mProps.msDescription = value.toString().toStdString();
    }
    //no parameters, and therfore no processData() called here
}

void
CVImagePropertiesModel::
processData(const std::shared_ptr< CVImageData > & in, CVImagePropertiesProperties &props )
{
    cv::Mat& in_image = in->image();
    props.miChannels = in_image.channels();
    props.mCVMSizeImage = cv::Size(in_image.cols, in_image.rows);
    bool binary_bandw[2] = {true,true};
    bool checkBin = true;
    bool checkBW = true;
    if(in_image.channels()!=1)
    {
        checkBin = false;
        checkBW = false;
    }
    double arr[2] = {0,0};
    cv::minMaxLoc(in_image,&arr[0],&arr[1]);
    if((arr[0]!=0 && arr[0]!=255) || (arr[1]!=0 && arr[1]!= 255))
    {
        checkBW = false;
    }
    for(int i=0; i<in_image.rows; i++)
    {
        for(int j=0; j<in_image.cols; j++)
        {
            double val = static_cast<double>(in_image.at<uchar>(i,j));
            if(checkBin && val!=arr[0] && val!=arr[1])
            {
                checkBin = false;
            }
            if(checkBW && val!=0 && val!=255)
            {
                checkBW = false;
            }
            if(!checkBin && !checkBW)
            {
                break;
            }
        }
        if(!checkBin && !checkBW)
        {
            break;
        }
    }
    binary_bandw[0] = checkBin? true : false ;
    binary_bandw[1] = checkBW? true : false ;
    props.mbIsBinary = binary_bandw[0];
    props.mbIsBAndW = binary_bandw[1];
    props.mbIsContinuous = in_image.isContinuous();

    auto prop = mMapIdToProperty["image_channels"];
    auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedProp->getData() = QString("%1").arg(props.miChannels);

    prop = mMapIdToProperty["image_size"];
    QString imageSize = QString("%1 px x %2 px").arg(props.mCVMSizeImage.height).arg(props.mCVMSizeImage.width);
    typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedProp->getData() = QString("%1").arg(imageSize);

    prop = mMapIdToProperty["is_binary"];
    QString isBinary = props.mbIsBinary? "Yes" : "No" ;
    typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedProp->getData() = QString("%1").arg(isBinary);

    prop = mMapIdToProperty["is_bandw"];
    QString isBAndW = props.mbIsBAndW? "Yes" : "No" ;
    typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedProp->getData() = QString("%1").arg(isBAndW);

    prop = mMapIdToProperty["is_continuous"];
    QString isContinuous = props.mbIsContinuous? "Yes" : "No" ;
    typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
    typedProp->getData() = QString("%1").arg(isContinuous);
}

const QString CVImagePropertiesModel::_category = QString( "Output" );

const QString CVImagePropertiesModel::_model_name = QString( "CV Image Properties" );
