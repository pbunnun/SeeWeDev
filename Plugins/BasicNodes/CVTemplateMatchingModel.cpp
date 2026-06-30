//Copyright © 2020 - 2026, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "CVTemplateMatchingModel.hpp"

#include <QDebug> //for debugging using qDebug()


#include "qtvariantproperty_p.h"

const QString CVTemplateMatchingModel::_category = QString( "Image Operation" );

const QString CVTemplateMatchingModel::_model_name = QString( "CV Template Matching" );

CVTemplateMatchingModel::
CVTemplateMatchingModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":TemplateMatching.png" )
{
    mpCroppedImageData = std::make_shared<CVImageData>( cv::Mat() );
    mpInfoData = std::make_shared<InformationData>();

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"TM_SQDIFF", "TM_SQDIFF_NORMED", "TM_CCORR", "TM_CCORR_NORMED", "TM_CCOEFF", "TM_CCOEFF_NORMED"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "matching_method";
    auto propMatchingMethod = std::make_shared< TypedProperty < EnumPropertyType > > ("Matching Method", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propMatchingMethod );
    mMapIdToProperty[ propId ] = propMatchingMethod;

    IntPropertyType intPropertyType;
    intPropertyType.miValue = mParams.miMaxMatches;
    intPropertyType.miMin = 1;
    propId = "max_matches";
    auto propMaxMatches = std::make_shared<TypedProperty<IntPropertyType>>("Max Matches", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propMaxMatches);
    mMapIdToProperty[propId] = propMaxMatches;
}

unsigned int
CVTemplateMatchingModel::
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
CVTemplateMatchingModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out && portIndex == 1)
    {
        return InformationData().type();
    }
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVTemplateMatchingModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        if (I == 0)
            return mpCroppedImageData;
        else if (I == 1)
            return mpInfoData;
    }
    return nullptr;
}

void
CVTemplateMatchingModel::
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
                processData( mapCVImageInData, mpCroppedImageData, mpInfoData, mParams );
            }
        }
    }

    updateAllOutputPorts();
}

QJsonObject
CVTemplateMatchingModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["matchingMethod"] = mParams.miMatchingMethod;
    cParams["maxMatches"] = mParams.miMaxMatches;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVTemplateMatchingModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

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
        v = paramsObj[ "maxMatches" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "max_matches" ];
            auto typedProp = std::static_pointer_cast< TypedProperty < IntPropertyType > >(prop);
            typedProp->getData().miValue = v.toInt();

            mParams.miMaxMatches = v.toInt();
        }
    }
}

void
CVTemplateMatchingModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

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
    else if(id=="max_matches")
    {
        auto TypedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        TypedProp->getData().miValue = value.toInt();
        mParams.miMaxMatches = value.toInt();
    }

    if( mapCVImageInData[0] && mapCVImageInData[1] )
    {
        processData( mapCVImageInData, mpCroppedImageData, mpInfoData, mParams );

        updateAllOutputPorts();
    }
}

void
CVTemplateMatchingModel::
processData(const std::shared_ptr< CVImageData > (&in)[2],
            std::shared_ptr< CVImageData > & outCrop,
            std::shared_ptr< InformationData > & outInfo,
            const TemplateMatchingParameters & params )
{
    cv::Mat& in_image = in[0]->data();
    cv::Mat& temp_image = in[1]->data();
    if(in_image.empty() || temp_image.empty() || in_image.depth()!=temp_image.depth() ||
       (in_image.depth()!=CV_8U && in_image.depth()!=CV_8S && in_image.depth()!=CV_32F) ||
       temp_image.rows>in_image.rows || temp_image.cols > in_image.cols)
    {
        outCrop->set_image(cv::Mat());
        outInfo->set_information("");
        return;
    }

    cv::Mat result_map;
    cv::matchTemplate(in_image, temp_image, result_map, params.miMatchingMethod);

    // Multi-match extraction logic
    std::vector<std::pair<double, cv::Rect>> matches;
    bool isMinMethod = (params.miMatchingMethod == cv::TM_SQDIFF || params.miMatchingMethod == cv::TM_SQDIFF_NORMED);
    int maxDetections = params.miMaxMatches;
    
    cv::Mat scoreMap = result_map.clone();
    
    for (int i = 0; i < maxDetections; ++i)
    {
        double minVal, maxVal;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(scoreMap, &minVal, &maxVal, &minLoc, &maxLoc);
        
        double score = isMinMethod ? minVal : maxVal;
        cv::Point loc = isMinMethod ? minLoc : maxLoc;
        
        if (loc.x < 0 || loc.y < 0 || loc.x + temp_image.cols > in_image.cols || loc.y + temp_image.rows > in_image.rows)
            break;
            
        cv::Rect matchRect(loc.x, loc.y, temp_image.cols, temp_image.rows);
        matches.push_back({score, matchRect});
        
        // Zero-out region
        int x_start = std::max(0, loc.x - temp_image.cols / 2);
        int y_start = std::max(0, loc.y - temp_image.rows / 2);
        int x_end = std::min(scoreMap.cols, loc.x + temp_image.cols / 2);
        int y_end = std::min(scoreMap.rows, loc.y + temp_image.rows / 2);
        
        if (x_end <= x_start || y_end <= y_start)
            break;
            
        cv::Rect fillRect(x_start, y_start, x_end - x_start, y_end - y_start);
        if (isMinMethod) {
            scoreMap(fillRect).setTo(cv::Scalar(1e10));
        } else {
            scoreMap(fillRect).setTo(cv::Scalar(-1e10));
        }
    }

    if (matches.empty())
    {
        outCrop->set_image(cv::Mat());
        outInfo->set_information("");
        return;
    }

    // Output 0: cropped image of the maximum matching score
    cv::Rect bestRect = matches[0].second;
    cv::Mat crop = in_image(bestRect).clone();
    outCrop->set_image(crop);

    // Output 1: rect list sorted by score
    QString infoText;
    for (size_t idx = 0; idx < matches.size(); ++idx)
    {
        const auto& match = matches[idx];
        if (idx > 0)
            infoText += "\n";
        infoText += QString("[%1] Score: %2, Rect: [x:%3, y:%4, w:%5, h:%6]")
                    .arg(idx)
                    .arg(match.first, 0, 'f', 4)
                    .arg(match.second.x)
                    .arg(match.second.y)
                    .arg(match.second.width)
                    .arg(match.second.height);
    }
    outInfo->set_information(infoText);
}

QString
CVTemplateMatchingModel::
portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In)
    {
        if (portIndex == 0)
            return "Source Image (CVImageData): The main image to search inside.";
        else if (portIndex == 1)
            return "Template Image (CVImageData): The target sub-image to locate.";
    }
    else if (portType == QtNodes::PortType::Out)
    {
        if (portIndex == 0)
            return "Cropped Image (CVImageData): Bounding box region of the source image corresponding to the best template match (maximum score).";
        else if (portIndex == 1)
            return "Sorted Matches (InformationData): Bounding box coordinates list of candidates, sorted by matching score.";
    }
    return PBNodeDelegateModel::portToolTip(portType, portIndex);
}


