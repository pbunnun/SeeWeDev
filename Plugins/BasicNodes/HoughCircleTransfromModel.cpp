#include "HoughCircleTransfromModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "qtvariantproperty.h"

HoughCircleTransformModel::
HoughCircleTransformModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":HoughCircleTransform.png" )
{
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpIntegerData = std::make_shared< IntegerData >( int() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"HOUGH_GRADIENT", "HOUGH_STANDARD", "HOUGH_MULTI_SCALE", "HOUGH_GRADIENT_ALT", "HOUGH_PROBABILISTIC"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "hough_method";
    auto propHoughMethod = std::make_shared< TypedProperty< EnumPropertyType > >( "Method", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propHoughMethod );
    mMapIdToProperty[ propId ] = propHoughMethod;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mParams.mdInverseRatio;
    propId = "inverse_ratio";
    auto propInverseRatio = std::make_shared< TypedProperty< DoublePropertyType > >( "Resolution Inverse Ratio", propId, QVariant::Double, doublePropertyType, "Operation" );
    mvProperty.push_back( propInverseRatio );
    mMapIdToProperty[ propId ] = propInverseRatio;

    doublePropertyType.mdValue = mParams.mdCenterDistance;
    propId = "center_distance";
    auto propCenterDistance = std::make_shared< TypedProperty< DoublePropertyType > >( "Minimum Center Distance", propId, QVariant::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propCenterDistance );
    mMapIdToProperty[ propId ] = propCenterDistance;

    doublePropertyType.mdValue = mParams.mdThresholdU;
    propId = "th_u";
    auto propThresholdU = std::make_shared< TypedProperty< DoublePropertyType > >( "Upper Threshold", propId, QVariant::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propThresholdU );
    mMapIdToProperty[ propId ] = propThresholdU;

    doublePropertyType.mdValue = mParams.mdThresholdL;
    propId = "th_l";
    auto propThresholdL = std::make_shared< TypedProperty< DoublePropertyType > >( "Lower Threshold", propId, QVariant::Double, doublePropertyType , "Operation");
    mvProperty.push_back( propThresholdL );
    mMapIdToProperty[ propId ] = propThresholdL;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miRadiusMin;
    propId = "radius_min";
    auto propRadiusMin = std::make_shared<TypedProperty<IntPropertyType>>("Minimum Radius", propId, QVariant::Int, intPropertyType, "Operation");
    mvProperty.push_back(propRadiusMin);
    mMapIdToProperty[ propId ] = propRadiusMin;

    intPropertyType.miValue = mParams.miRadiusMax;
    propId = "radius_max";
    auto propRadiusMax = std::make_shared<TypedProperty<IntPropertyType>>("Maximum Radius", propId, QVariant::Int, intPropertyType, "Operation");
    mvProperty.push_back(propRadiusMax);
    mMapIdToProperty[ propId ] = propRadiusMax;

    propId = "display_point";
    auto propDisplayPoint = std::make_shared< TypedProperty < bool > > ("Display Points", propId, QVariant::Bool, mParams.mbDisplayPoint, "Display");
    mvProperty.push_back( propDisplayPoint );
    mMapIdToProperty[ propId ] = propDisplayPoint;

    UcharPropertyType ucharPropertyType;
    for(int i=0; i<3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucPointColor[i];
        propId = QString("point_color_%1").arg(i);
        QString pointColor = QString::fromStdString("Point Color "+color[i]);
        auto propPointColor = std::make_shared<TypedProperty<UcharPropertyType>>(pointColor, propId, QVariant::Int, ucharPropertyType, "Display");
        mvProperty.push_back(propPointColor);
        mMapIdToProperty[ propId ] = propPointColor;
    }

    intPropertyType.miValue = mParams.miPointSize;
    propId = "point_size";
    auto propPointSize = std::make_shared<TypedProperty<IntPropertyType>>("Point Size", propId, QVariant::Int, intPropertyType, "Display");
    mvProperty.push_back( propPointSize );
    mMapIdToProperty[ propId ] = propPointSize;

    propId = "display_circle";
    auto propDisplayCircle = std::make_shared<TypedProperty<bool>>("Display Circle", propId, QVariant::Bool, mParams.mbDisplayCircle, "Display");
    mvProperty.push_back(propDisplayCircle);
    mMapIdToProperty[ propId ] = propDisplayCircle;

    for(int i=0; i<3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucCircleColor[i];
        propId = QString("circle_color_%1").arg(i);
        QString circleColor = QString::fromStdString("Circle Color "+color[i]);
        auto propCircleColor = std::make_shared<TypedProperty<UcharPropertyType>>(circleColor, propId, QVariant::Int, ucharPropertyType, "Display");
        mvProperty.push_back(propCircleColor);
        mMapIdToProperty[ propId ] = propCircleColor;
    }

    intPropertyType.miValue = mParams.miCircleThickness;
    propId = "circle_thickness";
    auto propCircleThickness = std::make_shared<TypedProperty<IntPropertyType>>("Circle Thickness", propId, QVariant::Int, intPropertyType, "Display");
    mvProperty.push_back( propCircleThickness );
    mMapIdToProperty[ propId ] = propCircleThickness;

    enumPropertyType.mslEnumNames = QStringList({"LINE_8", "LINE_4", "LINE_AA"});
    enumPropertyType.miCurrentIndex = 2;
    propId = "circle_type";
    auto propCircleType = std::make_shared<TypedProperty<EnumPropertyType>>("Circle Type", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Display");
    mvProperty.push_back( propCircleType );
    mMapIdToProperty[ propId ] = propCircleType;
}

unsigned int
HoughCircleTransformModel::
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
HoughCircleTransformModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 1)
    {
        return IntegerData().type();
    }
    return CVImageData().type();
}


std::shared_ptr<NodeData>
HoughCircleTransformModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        if(I == 0)
        {
            return mpCVImageData;
        }
        else if(I == 1)
        {
            return mpIntegerData;
        }
    }
    return nullptr;
}

void
HoughCircleTransformModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mpCVImageInData = d;
            processData( mpCVImageInData, mpCVImageData, mpIntegerData, mParams);
        }
    }

    updateAllOutputPorts();
}

QJsonObject
HoughCircleTransformModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["houghMethod"] = mParams.miHoughMethod;
    cParams["inverseRatio"] = mParams.mdInverseRatio;
    cParams["centerDistance"] = mParams.mdCenterDistance;
    cParams["thresholdU"] = mParams.mdThresholdU;
    cParams["thresholdL"] = mParams.mdThresholdL;
    cParams["radiusMin"] = mParams.miRadiusMin;
    cParams["radiusMax"] = mParams.miRadiusMax;
    cParams["displayPoint"] = mParams.mbDisplayPoint;
    for(int i=0; i<3; i++)
    {
        cParams[QString("pointColor%1").arg(i)] = mParams.mucPointColor[i];
    }
    cParams["pointSize"] = mParams.miPointSize;
    cParams["displayCircle"] = mParams.mbDisplayCircle;
    for(int i=0; i<3; i++)
    {
        cParams[QString("circleColor%1").arg(i)] = mParams.mucCircleColor[i];
    }
    cParams["circleThickness"] = mParams.miCircleThickness;
    cParams["circleType"] = mParams.miCircleType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
HoughCircleTransformModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "houghMethod" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "hough_method" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miHoughMethod = v.toInt();
        }
        v = paramsObj[ "inverseRatio" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "inverse_ratio" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdInverseRatio = v.toDouble();
        }
        v = paramsObj[ "centerDistance" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "center_distance" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdCenterDistance = v.toDouble();
        }
        v =  paramsObj[ "thresholdU" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "th_u" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdThresholdU = v.toDouble();
        }
        v =  paramsObj[ "thresholdL" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "th_l" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mParams.mdThresholdL = v.toDouble();
        }
        v =  paramsObj[ "radiusMin" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "radius_min" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miRadiusMin = v.toInt();
        }
        v =  paramsObj[ "radiusMax" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "radius_max" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mParams.miRadiusMax = v.toInt();
        }
        v = paramsObj[ "displayPoint" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "display_point" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < bool > > (prop);
            typedProp->getData() = v.toBool();

            mParams.mbDisplayPoint = v.toBool();
        }
        for(int i=0; i<3; i++)
        {
            v = paramsObj[QString("pointColor%1").arg(i)];
            if( !v.isUndefined() )
            {
                auto prop = mMapIdToProperty[QString("point_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
                typedProp->getData().mucValue = v.toInt();

                mParams.mucPointColor[i] = v.toInt();
            }
        }
        v = paramsObj[ "pointSize" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "point_size" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miPointSize = v.toInt();
        }
        v = paramsObj[ "displayCircle" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "display_circle" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < bool > > (prop);
            typedProp->getData() = v.toBool();

            mParams.mbDisplayCircle = v.toBool();
        }
        for(int i=0; i<3; i++)
        {
            v = paramsObj[QString("circleColor%1").arg(i)];
            if( !v.isUndefined() )
            {
                auto prop = mMapIdToProperty[QString("circle_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
                typedProp->getData().mucValue = v.toInt();

                mParams.mucCircleColor[i] = v.toInt();
            }
        }
        v = paramsObj[ "circleThickness" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "circle_thickness" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miCircleThickness = v.toInt();
        }
        v = paramsObj[ "circleType" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "circle_type" ];
            auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miCircleType = v.toInt();
        }
    }
}

void
HoughCircleTransformModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "hough_method" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miHoughMethod = cv::HOUGH_GRADIENT;
            break;

        case 1:
            mParams.miHoughMethod = cv::HOUGH_STANDARD;
            break;

        case 2:
            mParams.miHoughMethod = cv::HOUGH_MULTI_SCALE;
            break;

        case 3:
#if CV_MINOR_VERSION > 2
            mParams.miHoughMethod = cv::HOUGH_GRADIENT_ALT;
            break;
#endif
        case 4:
            mParams.miHoughMethod = cv::HOUGH_PROBABILISTIC;
            break;
        }
    }
    else if( id == "inverse_ratio" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdInverseRatio = value.toDouble();
    }
    else if( id == "center_distance" )
    {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdCenterDistance = value.toDouble();
    }
    else if( id == "th_u" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdThresholdU = value.toDouble();
    }
    else if( id == "th_l" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mParams.mdThresholdL = value.toDouble();
    }
    else if( id == "radius_min" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miRadiusMin = value.toInt();
    }
    else if( id == "radius_max" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mParams.miRadiusMax = value.toInt();
    }
    else if( id == "display_point" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbDisplayPoint = value.toBool();
    }
    for(int i=0; i<3; i++)
    {
        if(id==QString("point_color_%1").arg(i))
        {
            auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typedProp->getData().mucValue = value.toInt();

            mParams.mucPointColor[i] = value.toInt();
        }
    }
    if( id == "point_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();

        mParams.miPointSize = value.toInt();
    }
    else if( id == "display_circle" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mParams.mbDisplayCircle = value.toBool();
    }
    for(int i=0; i<3; i++)
    {
        if(id==QString("circle_color_%1").arg(i))
        {
            auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typedProp->getData().mucValue = value.toInt();

            mParams.mucCircleColor[i] = value.toInt();
        }
    }
    if( id == "circle_thickness" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();

        mParams.miCircleThickness = value.toInt();
    }
    else if( id == "circle_type" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miCircleType = cv::LINE_8;
            break;

        case 1:
            mParams.miCircleType = cv::LINE_4;
            break;

        case 2:
            mParams.miCircleType = cv::LINE_AA;
            break;
        }
    }

    if( mpCVImageInData )
    {
        processData( mpCVImageInData, mpCVImageData, mpIntegerData, mParams);

        updateAllOutputPorts();
    }
}

void
HoughCircleTransformModel::
processData(const std::shared_ptr< CVImageData > & in, std::shared_ptr<CVImageData> & outImage,
            std::shared_ptr<IntegerData> &outInt, const HoughCircleTransformParameters & params)
{
    cv::Mat& in_image = in->image();
    if(in_image.empty() || in_image.type()!=CV_8UC1)
    {
        return;
    }
    cv::Mat& out_image = outImage->image();   
    outImage->set_image(in_image);
    std::vector<cv::Vec3f> Circles;
    cv::HoughCircles(out_image,
                     Circles,
                     params.miHoughMethod,
                     params.mdInverseRatio,
                     params.mdCenterDistance,
                     200,
                     100,
                     25,
                     200);
    outInt->number() = static_cast<int>(Circles.size());
    cv::cvtColor(in_image,out_image,cv::COLOR_GRAY2BGR);
    for(cv::Vec3f& circle : Circles)
    {
        if(params.mbDisplayPoint)
        {
            cv::circle(out_image,
                       cv::Point(circle[0],circle[1]),
                       1,
                       cv::Scalar(static_cast<uchar>(params.mucPointColor[0]),
                                  static_cast<uchar>(params.mucPointColor[1]),
                                  static_cast<uchar>(params.mucPointColor[2])),
                       params.miPointSize,
                       cv::LINE_8);
        }
        if(params.mbDisplayCircle)
        {
            cv::circle(out_image,
                       cv::Point(circle[0],circle[1]),
                       circle[2],
                       cv::Scalar(static_cast<uchar>(params.mucCircleColor[0]),
                                  static_cast<uchar>(params.mucCircleColor[1]),
                                  static_cast<uchar>(params.mucCircleColor[2])),
                       params.miCircleThickness,
                       params.miCircleType);
        }
    }
}

const std::string HoughCircleTransformModel::color[3] = {"B", "G", "R"};

const QString HoughCircleTransformModel::_category = QString( "Image Processing" );

const QString HoughCircleTransformModel::_model_name = QString( "Hough Circle" );
