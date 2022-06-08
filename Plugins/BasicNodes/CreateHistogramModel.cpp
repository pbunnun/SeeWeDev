#include "CreateHistogramModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include "qtvariantproperty.h"
#include <math.h>

CreateHistogramModel::
CreateHistogramModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":CreateHistogram.png" )
{ //ucharbin(mod(range)==0),ucharrange_max,ucharrange_min,intthic,intlinetype
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat(256,256,CV_8UC3,cv::Scalar::all(0)) );

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miBinCount;
    intPropertyType.miMax = 256;
    QString propId = "bin_count";
    auto propBinCount = std::make_shared< TypedProperty< IntPropertyType > >( "Bin Count", propId, QVariant::Int, intPropertyType, "Operation" );
    mvProperty.push_back( propBinCount );
    mMapIdToProperty[ propId ] = propBinCount;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdIntensityMax;
    doublePropertyType.mdMax = 255;
    propId = "intensity_max";
    auto propIntensityMax = std::make_shared< TypedProperty< DoublePropertyType > >( "Maximum Intensity", propId, QVariant::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propIntensityMax );
    mMapIdToProperty[ propId ] = propIntensityMax;

    doublePropertyType.mdValue= mParams.mdIntensityMin;
    doublePropertyType.mdMax = 255;
    propId = "intensity_min";
    auto propIntensityMin = std::make_shared< TypedProperty< DoublePropertyType > >( "Minimum Intensity", propId, QVariant::Double, doublePropertyType, "Operation" );
    mvProperty.push_back( propIntensityMin );
    mMapIdToProperty[ propId ] = propIntensityMin;

    EnumPropertyType enumPropertyType; //9
    enumPropertyType.mslEnumNames = QStringList({"NORM_L1", "NORM_L2", "NORM_INF", "NORM_L2SQR", "NORM_MINMAX", "NORM_HAMMING", "NORM_HAMMING2", "NORM_RELATIVE", "NORM_TYPE_MASK"});
    enumPropertyType.miCurrentIndex = 4;
    propId = "norm_type";
    auto propNormType = std::make_shared< TypedProperty< EnumPropertyType >>("Norm Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propNormType );
    mMapIdToProperty[ propId ] = propNormType;

    intPropertyType.miValue = mParams.miLineThickness;
    propId = "line_thickness";
    auto propLineThickness = std::make_shared< TypedProperty< IntPropertyType > >( "Line Thickness", propId, QVariant::Int, intPropertyType , "Display");
    mvProperty.push_back( propLineThickness );
    mMapIdToProperty[ propId ] = propLineThickness;

    enumPropertyType.mslEnumNames = QStringList({"LINE_8", "LINE_4", "LINE_AA"});
    enumPropertyType.miCurrentIndex = 0;
    propId = "line_type";
    auto propLineType = std::make_shared< TypedProperty< EnumPropertyType >>("Line Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display");
    mvProperty.push_back( propLineType );
    mMapIdToProperty[ propId ] = propLineType;

    propId = "draw_endpoints";
    auto propDrawEndpoints= std::make_shared<TypedProperty<bool>>("Draw Endpoints", propId, QVariant::Bool, mParams.mbDrawEndpoints, "Display");
    mvProperty.push_back( propDrawEndpoints );
    mMapIdToProperty[ propId ] = propDrawEndpoints;

    propId = "enable_b";
    auto propEnableB = std::make_shared<TypedProperty<bool>>("Enable B", propId, QVariant::Bool, mParams.mbEnableB, "Display");
    mvProperty.push_back( propEnableB );
    mMapIdToProperty[ propId ] = propEnableB;

    propId = "enable_g";
    auto propEnableG= std::make_shared<TypedProperty<bool>>("Enable G", propId, QVariant::Bool, mParams.mbEnableG, "Display");
    mvProperty.push_back( propEnableG );
    mMapIdToProperty[ propId ] = propEnableG;

    propId = "enable_r";
    auto propEnableR= std::make_shared<TypedProperty<bool>>("Enable R", propId, QVariant::Bool, mParams.mbEnableR, "Display");
    mvProperty.push_back( propEnableR );
    mMapIdToProperty[ propId ] = propEnableR;
}

unsigned int
CreateHistogramModel::
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
CreateHistogramModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CreateHistogramModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void
CreateHistogramModel::
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
CreateHistogramModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["binCount"] = mParams.miBinCount;
    cParams["intensityMax"] = mParams.mdIntensityMax;
    cParams["intensityMin"] = mParams.mdIntensityMin;
    cParams["normType"] = mParams.miNormType;
    cParams["lineThickness"] = mParams.miLineThickness;
    cParams["lineType"] = mParams.miLineType;
    cParams["drawEndpoints"] = mParams.mbDrawEndpoints;
    cParams["enableB"] = mParams.mbEnableB;
    cParams["enableG"] = mParams.mbEnableG;
    cParams["enableR"] = mParams.mbEnableR;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CreateHistogramModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "binCount" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "bin_count" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miBinCount = v.toInt();
        }
        v =  paramsObj[ "intensityMax" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "intensity_max" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdIntensityMax = v.toDouble();
        }
        v =  paramsObj[ "intensityMin" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "intensity_min" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdIntensityMin = v.toDouble();
        }
        v =  paramsObj[ "normType" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "norm_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miNormType = v.toInt();
        }
        v =  paramsObj[ "lineThickness" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "line_thickness" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miLineThickness = v.toInt();
        }
        v = paramsObj[ "lineType" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "line_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miLineType = v.toInt();
        }
        v = paramsObj[ "drawEndpoints" ];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty[ "draw_endpoints" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mParams.mbDrawEndpoints = v.toBool();
        }
        v = paramsObj[ "enableB" ];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty[ "enable_b" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mParams.mbEnableB = v.toBool();
        }
        v = paramsObj[ "enableG" ];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty[ "enable_g" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mParams.mbEnableG = v.toBool();
        }
        v = paramsObj[ "enableR" ];
        if(!v.isUndefined())
        {
            auto prop = mMapIdToProperty[ "enable_r" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = v.toBool();

            mParams.mbEnableR = v.toBool();
        }
    }
}

void
CreateHistogramModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "bin_count" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miBinCount = value.toInt();
    }
    else if( id == "intensity_max" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdIntensityMax = value.toDouble();
    }
    else if( id == "intensity_min" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdIntensityMin = value.toDouble();
    }
    else if( id == "norm_type" ) //{"NORM_L1", "NORM_L2", "NORM_INF", "NORM_L2SQR", "NORM_MINMAX", "NORM_HAMMING", "NORM_HAMMING2", "NORM_RELATIVE", "NORM_TYPE_MASK"}
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt()) //Only NORM_MINMAX is currently functional
        {
        case 0:
            mParams.miNormType = cv::NORM_L1;
            break;

        case 1:
            mParams.miNormType = cv::NORM_L2;
            break;

        case 2:
            mParams.miNormType = cv::NORM_INF;
            break;

        case 3:
            mParams.miNormType = cv::NORM_L2SQR;
            break;

        case 4:
            mParams.miNormType = cv::NORM_MINMAX;
            break;

        case 5:
            mParams.miNormType = cv::NORM_HAMMING;
            break;

        case 6:
            mParams.miNormType = cv::NORM_HAMMING2;
            break;

        case 7:
            mParams.miNormType = cv::NORM_RELATIVE;
            break;

        case 8:
            mParams.miNormType = cv::NORM_TYPE_MASK;
            break;
        }
    }
    else if( id == "line_thickness" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miLineThickness = value.toInt();
    }
    else if( id == "line_type" ) //{"NORM_L1", "NORM_L2", "NORM_INF", "NORM_L2SQR", "NORM_MINMAX", "NORM_HAMMING", "NORM_HAMMING2", "NORM_RELATIVE", "NORM_TYPE_MASK"}
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        switch(value.toInt())
        {
        case 0:
            mParams.miLineType = cv::LINE_8;
            break;

        case 1:
            mParams.miLineType = cv::LINE_4;
            break;

        case 2:
            mParams.miLineType = cv::LINE_AA;
            break;
        }
    }
    else if(id=="draw_endpoints")
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbDrawEndpoints = value.toBool();
    }
    else if(id=="enable_b")
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbEnableB = value.toBool();
    }
    else if(id=="enable_g")
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbEnableG = value.toBool();
    }
    else if(id=="enable_r")
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbEnableR = value.toBool();
    }
    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mParams );

        Q_EMIT dataUpdated(0);
    }
}

void
CreateHistogramModel::
processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out,
             const CreateHistogramParameters & params )
{
    cv::Mat& in_image = in->image();
    cv::Mat& out_image = out->image();
    if(in_image.empty() || (in_image.depth()!=CV_8U && in_image.depth()!=CV_16U && in_image.depth()!=CV_32F))
    {
        return;
    }
    out->image() = cv::Scalar::all(0);
    float range[2] = { static_cast<float>( params.mdIntensityMin ),static_cast<float>( params.mdIntensityMax+1 ) }; //+1 to make it inclusive
    double binSize = static_cast<double>( (range[1]-range[0] )/params.miBinCount );
    const float* pRange = &range[0];
    if( in_image.channels() == 1 )
    {
        cv::Mat cvChannelSplit = in_image;
        cv::Mat cvHistogramSplit;
        if(out_image.channels()==3)
        {
            cv::cvtColor(out_image,out_image,cv::COLOR_BGR2GRAY);
        }
        cv::calcHist( &cvChannelSplit, 1, 0, cv::Mat(), cvHistogramSplit, 1, & params.miBinCount, &pRange, true, false );
        cv::normalize( cvHistogramSplit, cvHistogramSplit, 0, out_image.rows, params.miNormType, -1 );
        std::vector< cv::Point > vPoint = {};
        if(params.mbDrawEndpoints)
        {
            vPoint.push_back( cv::Point( static_cast<int>(mParams.mdIntensityMin), out_image.rows ) );
        }
        for(int j = 0; j < params.miBinCount; j++)
        {
            vPoint.push_back(cv::Point( static_cast<int>(mParams.mdIntensityMin)+(j+0.5)*binSize, out_image.rows - cvRound( cvHistogramSplit.at< float >( j ) ) ) );
        }
        if(params.mbDrawEndpoints)
        {
            vPoint.push_back( cv::Point( static_cast<int>(mParams.mdIntensityMax), out_image.rows ));
        }
        cv::Scalar color(255);
        std::vector< std::vector< cv::Point > > vvPoint = { vPoint };
        cv::polylines( out_image, vvPoint, false, color, params.miLineThickness, params.miLineType);
    }
    else if(in_image.channels()==3)
    {
        std::array< cv::Mat, 3 > cvBGRChannelSplit;
        std::array< cv::Mat, 3 > cvHistogramSplit;
        std::array< bool , 3 > enableDisplay = {params.mbEnableB, params.mbEnableG, params.mbEnableR};
        if(out_image.channels()==1)
        {
            cv::cvtColor(out_image,out_image,cv::COLOR_GRAY2BGR);
        }
        cv::split( in->image(), cvBGRChannelSplit );
        for( int i=0; i < static_cast< int >( cvBGRChannelSplit.size() ); i++ )
        {
            if(enableDisplay[i])
            {
                cv::calcHist( &cvBGRChannelSplit[i], 1, 0, cv::Mat(), cvHistogramSplit[i], 1, &params.miBinCount, &pRange, true, false );
                cv::normalize( cvHistogramSplit[i], cvHistogramSplit[i], 0, out_image.rows, params.miNormType, -1 );
                std::vector< cv::Point > vPoint = {};
                if(params.mbDrawEndpoints)
                {
                    vPoint.push_back(cv::Point( static_cast<int>(mParams.mdIntensityMin), out_image.rows ));
                }
                for( int j = 0; j < params.miBinCount; j++ )
                {
                    vPoint.push_back( cv::Point( static_cast<int>(mParams.mdIntensityMin)+(j+0.5)*binSize, out_image.rows - cvRound( cvHistogramSplit[i].at<float>(j) ) ) );
                }
                if(params.mbDrawEndpoints)
                {
                    vPoint.push_back( cv::Point( static_cast<int>(mParams.mdIntensityMax), out_image.rows ) );
                }
                cv::Scalar color( 0, 0, 0 );
                color[i] = 255;
                std::vector< std::vector< cv::Point > > vvPoint = { vPoint };
                cv::polylines( out_image, vvPoint, false, color, params.miLineThickness, params.miLineType );
            }
        }
    }
}

const QString CreateHistogramModel::_category = QString( "Image Analysis" );

const QString CreateHistogramModel::_model_name = QString( "Create Histogram" );
