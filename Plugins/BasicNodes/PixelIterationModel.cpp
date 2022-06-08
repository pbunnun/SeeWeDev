#include "PixelIterationModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"


void PixIter::Iterate(cv::Mat &image, const cv::Scalar &inColors, const cv::Scalar &outColors, int* const number, const double alpha, const double beta) const
{
    cv::Vec3b in_color(inColors[0],inColors[1],inColors[2]);
    cv::Vec3b out_color(outColors[0],outColors[1],outColors[2]);
    if(miIterKey == COUNT)
    {
        if(image.channels()==3)
        {
            *number = 0;
            for(int i=0; i<image.rows; i++)
            {
                for(int j=0; j<image.cols; j++)
                {
                    if(image.at<cv::Vec3b>(i,j) == in_color)
                    {
                        (*number)++;
                    }
                }
            }
        }
        else if(image.channels()==1)
        {
            *number = 0;
            if(image.depth() == CV_8U)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        if(image.at<int32_t>(i,j) == inColors[0])
                        {
                            (*number)++;
                        }
                    }
                }
            }
            else if(image.depth() == CV_32F)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        if(image.at<float>(i,j) == inColors[0])
                        {
                            (*number)++;
                        }
                    }
                }
            }
            else if(image.depth() == CV_32S)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        if(image.at<int32_t>(i,j) == inColors[0])
                        {
                            (*number)++;
                        }
                    }
                }
            }
        }
    }
    else if(miIterKey == REPLACE)
    {
        if(image.channels()==3)
        {
            *number = 0;
            for(int i=0; i<image.rows; i++)
            {
                for(int j=0; j<image.cols; j++)
                {
                    if(image.at<cv::Vec3b>(i,j) == in_color)
                    {
                        image.at<cv::Vec3b>(i,j) = out_color;
                        (*number)++;
                    }
                }
            }
        }
        else if(image.channels()==1)
        {
            *number = 0;
            if(image.depth() == CV_8U)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        if(image.at<uchar>(i,j) == static_cast<uchar>(inColors[0]))
                        {
                            image.at<uchar>(i,j) = static_cast<uchar>(outColors[0]);
                            (*number)++;
                        }
                    }
                }
            }
            if(image.depth() == CV_32F)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        if(image.at<float>(i,j) == static_cast<float>(inColors[0]))
                        {
                            image.at<float>(i,j) = static_cast<float>(outColors[0]);
                            (*number)++;
                        }
                    }
                }
            }
            else if(image.depth() == CV_32S)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        if(image.at<int32_t>(i,j) == static_cast<int32_t>(inColors[0]))
                        {
                            image.at<int32_t>(i,j) = static_cast<int32_t>(outColors[0]);
                            (*number)++;
                        }
                    }
                }
            }
        }
    }
    else if(miIterKey == LINEAR)
    {
        if(image.channels()==3)
        {
            for(int i=0; i<image.rows; i++)
            {
                for(int j=0; j<image.cols; j++)
                {
                    image.at<cv::Vec3b>(i,j)
                    = cv::Vec3b(alpha*image.at<cv::Vec3b>(i,j)[0]+beta,
                                alpha*image.at<cv::Vec3b>(i,j)[1]+beta,
                                alpha*image.at<cv::Vec3b>(i,j)[2]+beta);
                }
            }
        }
        else if(image.channels()==1)
        {
            if(image.depth() == CV_8U)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        image.at<uchar>(i,j) = alpha*image.at<uchar>(i,j)+beta;
                    }
                }
            }
            if(image.depth() == CV_32F)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        image.at<float>(i,j) = alpha*image.at<float>(i,j)+beta;
                    }
                }
            }
            else if(image.depth() == CV_32S)
            {
                for(int i=0; i<image.rows; i++)
                {
                    for(int j=0; j<image.cols; j++)
                    {
                        image.at<int32_t>(i,j) = alpha*image.at<int32_t>(i,j)+beta;
                    }
                }
            }
        }
    }
    else if(miIterKey == INVERSE)
    {
        if(image.channels()==3)
        {
            for(int i=0; i<image.rows; i++)
            {
                for(int j=0; j<image.cols; j++)
                {
                    for(int k=0; k<3; k++)
                    {
                        image.at<cv::Vec3b>(i,j)[k] = 255 - image.at<cv::Vec3b>(i,j)[k];
                    }
                }
            }
        }
    }
}


PixelIterationModel::
PixelIterationModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":PixelIteration.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpIntegerData = std::make_shared< IntegerData >( int() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"COUNT", "REPLACE", "LINEAR", "INVERSE"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "operation";
    auto propOperation = std::make_shared< TypedProperty< EnumPropertyType > >( "Operation", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propOperation );
    mMapIdToProperty[ propId ] = propOperation;

    UcharPropertyType ucharPropertyType;
    for(int i=0; i<3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucColorInput[i];
        ucharPropertyType.mucMin = -255;
        QString colorInput = QString::fromStdString("Input Color "+color[i]);
        propId = QString("color_input_%1").arg(i);
        auto propColorInput = std::make_shared< TypedProperty< UcharPropertyType > >( colorInput, propId, QVariant::Int , ucharPropertyType, "Operation" );
        mvProperty.push_back( propColorInput );
        mMapIdToProperty[ propId ] = propColorInput;
    }

    for(int i=0; i<3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucColorOutput[i];
        ucharPropertyType.mucMin = -255;
        QString colorInput = QString::fromStdString("Output Color "+color[i]);
        propId = QString("color_output_%1").arg(i);
        auto propColorOutput = std::make_shared< TypedProperty< UcharPropertyType > >( colorInput, propId, QVariant::Int , ucharPropertyType, "Operation" );
        mvProperty.push_back( propColorOutput );
        mMapIdToProperty[ propId ] = propColorOutput;
    }

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdAlpha;
    propId = "alpha";
    auto propAlpha = std::make_shared< TypedProperty< DoublePropertyType > >( "Alpha", propId, QVariant::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propAlpha );
    mMapIdToProperty[ propId ] = propAlpha;

    doublePropertyType.mdValue = mParams.mdBeta;
    propId = "beta";
    auto propBeta = std::make_shared< TypedProperty< DoublePropertyType > >( "Beta", propId, QVariant::Double, doublePropertyType, "Operation");
    mvProperty.push_back( propBeta );
    mMapIdToProperty[ propId ] = propBeta;
}

unsigned int
PixelIterationModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 2;
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
PixelIterationModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if(portIndex==0)
    {
        return CVImageData().type();
    }
    else if(portIndex==1)
    {
        if(portType==PortType::In)
        {
            return CVScalarData().type();
        }
        else if(portType==PortType::Out)
        {
            return IntegerData().type();
        }
    }
    return NodeDataType();
}


std::shared_ptr<NodeData>
PixelIterationModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        switch(I)
        {
        case 0:
            return mpCVImageData;

        case 1:
            return mpIntegerData;
        }
        return nullptr;
    }
    else
        return nullptr;
}

void
PixelIterationModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        if(portIndex == 0)
        {
            auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
            if (d)
            {
                mpCVImageInData = d;
            }
        }
        else if( portIndex == 1 )
        {
            auto d= std::dynamic_pointer_cast<CVScalarData>(nodeData);
            if (d)
            {
                mpCVScalarInData = d;
            }
        }
        if( mpCVImageInData )
        {
            if( mpCVScalarInData )
            {
                overwrite(mpCVScalarInData,mParams);
            }
            processData(mpCVImageInData,mpCVImageData,mpIntegerData,mParams);
        }
    }

    updateAllOutputPorts();
}

QJsonObject
PixelIterationModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["operation"] = mParams.miOperation;
    for(int i=0; i<3; i++)
    {
        cParams[QString("colorInput%1").arg(i)] = mParams.mucColorInput[i];
        cParams[QString("colorOutput%1").arg(i)] = mParams.mucColorOutput[i];
    }
    cParams["alpha"] = mParams.mdAlpha;
    cParams["beta"] = mParams.mdBeta;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
PixelIterationModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "operation" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "operation" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miOperation = v.toInt();
        }
        for(int i=0; i<3; i++)
        {
            v = paramsObj[QString("colorInput%1").arg(i)];
            if( !v.isUndefined() )
            {
                auto prop = mMapIdToProperty[QString("color_input_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucColorInput[i] = v.toInt();
            }
            v = paramsObj[QString("colorOutput%1").arg(i)];
            if( !v.isUndefined() )
            {
                auto prop = mMapIdToProperty[QString("color_output_%1").arg(i)];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = v.toInt();

                mParams.mucColorOutput[i] = v.toInt();
            }
        }
        v = paramsObj[ "alpha" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "alpha" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdAlpha = v.toDouble();
        }
        v = paramsObj[ "beta" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "beta" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdBeta = v.toDouble();
        }
    }
}

void
PixelIterationModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "operation" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        mParams.miOperation = value.toInt();
    }
    for(int i=0; i<3; i++)
    {
        if( id == QString("color_input_%1").arg(i) )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
            typedProp->getData().mucValue = value.toInt();

            mParams.mucColorInput[i] = value.toInt();
        }
        else if( id == QString("color_output_%1").arg(i) )
        {
            auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
            typedProp->getData().mucValue = value.toInt();

            mParams.mucColorOutput[i] = value.toInt();
        }
    }
    if( id == "alpha" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdAlpha = value.toDouble();
    }
    else if( id == "beta" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdBeta = value.toDouble();
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mpIntegerData, mParams );

        updateAllOutputPorts();
    }
}

void
PixelIterationModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr<CVImageData> & out,
            std::shared_ptr<IntegerData> &outInt, const PixelIterationParameters & params )
{
    if(in->image().empty())
    {
        return;
    }
    out->set_image(in->image());
    cv::Scalar inColors(params.mucColorInput[0],
                      params.mucColorInput[1],
                      params.mucColorInput[2]);
    cv::Scalar outColors(params.mucColorOutput[0],
                         params.mucColorOutput[1],
                         params.mucColorOutput[2]);
    PixIter It(params.miOperation);
    It.Iterate(out->image(),
               inColors,
               outColors,
               &(outInt->number()),
               params.mdAlpha,
               params.mdBeta);
}

void
PixelIterationModel::
overwrite(std::shared_ptr<CVScalarData> &in, PixelIterationParameters &params)
{
    cv::Scalar& in_scalar = in->scalar();
    for(int i=0; i<3; i++)
    {
        auto prop = mMapIdToProperty[QString("color_input_%1").arg(i)];
        auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        typedProp->getData().mucValue = in_scalar[i];
        params.mucColorInput[i] = in_scalar[i];
    }
    in.reset();
}

const std::string PixelIterationModel::color[3] = {"B", "G", "R"};

const QString PixelIterationModel::_category = QString( "Image Modification" );

const QString PixelIterationModel::_model_name = QString( "Pixel Iteration" );
