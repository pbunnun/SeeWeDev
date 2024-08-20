//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "TemplateMatchingModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include "qtvariantproperty.h"

TemplateMatchingModel::
TemplateMatchingModel()
    : PBNodeDataModel( _model_name ),
      _minPixmap( ":TemplateMatching.png" )
{
    for(std::shared_ptr<CVImageData>& mp : mapCVImageData)
    {
        mp = std::make_shared<CVImageData>( cv::Mat() );
    }

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"TM_SQDIFF", "TM_SQDIFF_NORMED", "TM_CCORR", "TM_CCORR_NORMED", "TM_CCOEFF", "TM_CCOEFF_NORMED"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "matching_method";
    auto propMatchingMethod = std::make_shared< TypedProperty < EnumPropertyType > > ("Matching Method", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propMatchingMethod );
    mMapIdToProperty[ propId ] = propMatchingMethod;

    UcharPropertyType ucharPropertyType;
    for(int i=0; i<3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucLineColor[i];
        QString propId = QString("line_color_%1").arg(i);
        QString lineColor = QString::fromStdString("Line Color "+color[i]);
        auto propLineColor = std::make_shared< TypedProperty < UcharPropertyType > > (lineColor , propId, QMetaType::Int, ucharPropertyType, "Display");
        mvProperty.push_back( propLineColor );
        mMapIdToProperty[ propId ] = propLineColor;
    }

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miLineThickness;
    propId = "line_thickness";
    auto propLineThickness = std::make_shared<TypedProperty<IntPropertyType>>("Line Thickness", propId,QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back(propLineThickness);
    mMapIdToProperty[ propId ] = propLineThickness;

    enumPropertyType.mslEnumNames = QStringList({"LINE_8", "LINE_4", "LINE_AA"});
    enumPropertyType.miCurrentIndex = 0;
    propId = "line_type";
    auto propLineType = std::make_shared<TypedProperty<EnumPropertyType>>("Line Type",propId,QtVariantPropertyManager::enumTypeId(),enumPropertyType, "Display");
    mvProperty.push_back(propLineType);
    mMapIdToProperty[propId] = propLineType;
}

unsigned int
TemplateMatchingModel::
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
TemplateMatchingModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
TemplateMatchingModel::
outData(PortIndex I)
{
    if( isEnable() )
        return mapCVImageData[I];
    else
        return nullptr;
}

void
TemplateMatchingModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            mapCVImageInData[portIndex] = d;
            if(mapCVImageInData[0] && mapCVImageInData[1])
            {
                processData( mapCVImageInData, mapCVImageData, mParams );
            }
        }
    }

    updateAllOutputPorts();
}

QJsonObject
TemplateMatchingModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["matchingMethod"] = mParams.miMatchingMethod;
    for(int i=0; i<3; i++)
    {
        cParams[QString("lineColor%1").arg(i)] = mParams.mucLineColor[i];
    }
    cParams["lineThickness"] = mParams.miLineThickness;
    cParams["lineType"] = mParams.miLineType;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
TemplateMatchingModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "matchingMethod" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "matching_method" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miMatchingMethod = v.toInt();
        }
        for(int i=0; i<3; i++)
        {
            v = paramsObj[QString("lineColor%1").arg(i)];
            if(!v.isNull())
            {
                auto prop = mMapIdToProperty[QString("line_color_%1").arg(i)];
                auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
                typedProp->getData().mucValue = v.toInt();

                mParams.mucLineColor[i] = v.toInt();
            }
        }
        v = paramsObj[ "lineThickness" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "line_thickness" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < IntPropertyType > >(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miLineThickness = v.toInt();
        }
        v = paramsObj[ "lineType" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "line_type" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < EnumPropertyType > >(prop);
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miLineType = v.toInt();
        }
    }
}

void
TemplateMatchingModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "matching_method" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();

        switch(value.toInt())
        {
        case 0:
            mParams.miMatchingMethod = cv::TM_SQDIFF;
            break;

        case 1:
            mParams.miMatchingMethod = cv::TM_SQDIFF_NORMED;
            break;

        case 2:
            mParams.miMatchingMethod = cv::TM_CCORR;
            break;

        case 3:
            mParams.miMatchingMethod = cv::TM_CCORR_NORMED;
            break;

        case 4:
            mParams.miMatchingMethod = cv::TM_CCOEFF;
            break;

        case 5:
            mParams.miMatchingMethod = cv::TM_CCOEFF_NORMED;
            break;
        }
    }
    for(int i=0; i<3; i++)
    {
        if( id == QString("line_color_%1").arg(i) )
        {
            auto typedProp = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typedProp->getData().mucValue = value.toInt();

            mParams.mucLineColor[i] = value.toInt();
        }
    }
    if(id=="line_thickness")
    {
        auto TypedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        TypedProp->getData().miValue = value.toInt();
        mParams.miLineThickness = value.toInt();
    }
    else if(id=="line_type")
    {
        auto TypedProp = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        TypedProp->getData().miCurrentIndex = value.toInt();
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

    if( mapCVImageInData[0]&&mapCVImageData[1] )
    {
        processData( mapCVImageInData, mapCVImageData, mParams );

        updateAllOutputPorts();
    }
}

void
TemplateMatchingModel::
processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr<CVImageData> (&out)[2],
            const TemplateMatchingParameters & params )
{
    cv::Mat& in_image = in[0]->data();
    cv::Mat& temp_image = in[1]->data();
    if(in_image.empty() || temp_image.empty() || in_image.depth()!=temp_image.depth() ||
       (in_image.depth()!=CV_8U && in_image.depth()!=CV_8S && in_image.depth()!=CV_32F) ||
       temp_image.rows>in_image.rows || temp_image.cols > in_image.cols)
    {
        return;
    }
    cv::Mat& out_image = out[0]->data();
    cv::matchTemplate(in_image,temp_image,out_image,params.miMatchingMethod);

    double minValue;
    double maxValue;
    cv::Point minLocation;
    cv::Point maxLocation;
    out[1]->set_image(in_image);
    cv::minMaxLoc(out_image,&minValue,&maxValue,&minLocation,&maxLocation);
    cv::Point& matchedLocation =
    (params.miMatchingMethod == cv::TM_SQDIFF || params.miMatchingMethod == cv::TM_SQDIFF_NORMED)?
    minLocation : maxLocation ;
    cv::rectangle(out[1]->data(),
                  matchedLocation,
                  cv::Point(matchedLocation.x + temp_image.cols,
                            matchedLocation.y + temp_image.rows),
                  cv::Scalar(static_cast<uchar>(params.mucLineColor[0]),
                             static_cast<uchar>(params.mucLineColor[1]),
                             static_cast<uchar>(params.mucLineColor[2])),
                  params.miLineThickness,
                  params.miLineType);
}

const std::string TemplateMatchingModel::color[3] = {"B", "G", "R"};

const QString TemplateMatchingModel::_category = QString( "Image Operation" );

const QString TemplateMatchingModel::_model_name = QString( "Template Matching" );
