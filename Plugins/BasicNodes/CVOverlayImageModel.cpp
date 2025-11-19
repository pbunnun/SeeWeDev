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

#include "CVOverlayImageModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>
#include <QtNodes/internal/ConnectionIdUtils.hpp>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "qtvariantproperty_p.h"

const QString CVOverlayImageModel::_category = QString( "Image Operation" );

const QString CVOverlayImageModel::_model_name = QString( "CV Overlay Image" );

CVOverlayImageModel::
CVOverlayImageModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":OverlayImage.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mvCVImageInData.resize(2);

    // Property for X offset
    IntPropertyType intPropertyType;
    intPropertyType.miValue = miOffsetX;
    QString propId = "offset_x";
    auto propOffsetX = std::make_shared< TypedProperty< IntPropertyType > >( "X Offset", propId, QMetaType::Int, intPropertyType, "Position" );
    mvProperty.push_back( propOffsetX );
    mMapIdToProperty[ propId ] = propOffsetX;

    // Property for Y offset
    intPropertyType.miValue = miOffsetY;
    propId = "offset_y";
    auto propOffsetY = std::make_shared< TypedProperty< IntPropertyType > >( "Y Offset", propId, QMetaType::Int, intPropertyType, "Position" );
    mvProperty.push_back( propOffsetY );
    mMapIdToProperty[ propId ] = propOffsetY;
}

unsigned int
CVOverlayImageModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 2;
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
CVOverlayImageModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVOverlayImageModel::
outData(PortIndex)
{
    if( isEnable() && mpCVImageData->data().empty() == false )
        return mpCVImageData;
    else
        return nullptr;
}

void
CVOverlayImageModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            d->data().copyTo( mvCVImageInData[portIndex] );
            if( !mvCVImageInData[0].empty() && !mvCVImageInData[1].empty() )
            {
                processData(mvCVImageInData, mpCVImageData);

                Q_EMIT dataUpdated(0);
            }
        }
    }
}

QJsonObject
CVOverlayImageModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["offset_x"] = miOffsetX;
    cParams["offset_y"] = miOffsetY;
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CVOverlayImageModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "offset_x" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["offset_x"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            miOffsetX = v.toInt();
        }

        v = paramsObj[ "offset_y" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["offset_y"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = v.toInt();
            miOffsetY = v.toInt();
        }
    }
}

void
CVOverlayImageModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "offset_x" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();
        miOffsetX = value.toInt();
    }
    else if( id == "offset_y" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();
        miOffsetY = value.toInt();
    }

    if( !mvCVImageInData[0].empty() && !mvCVImageInData[1].empty() )
    {
        processData(mvCVImageInData, mpCVImageData);

        Q_EMIT dataUpdated(0);
    }
}

void
CVOverlayImageModel::
processData(const std::vector< cv::Mat >&in, std::shared_ptr<CVImageData> &out)
{
    // Get base and overlay images
    const cv::Mat& base = in[0];
    const cv::Mat& overlay = in[1];

    // Check if images have compatible types
    if(base.type() != overlay.type())
    {
        // Type mismatch - cannot overlay
        return;
    }

    // Create a copy of the base image as output
    cv::Mat result;
    base.copyTo(result);

    // Calculate the valid region for overlay
    // Start position on base image
    int start_x = miOffsetX;
    int start_y = miOffsetY;

    // Start position on overlay image (if offset is negative, we start from inside overlay)
    int overlay_start_x = 0;
    int overlay_start_y = 0;

    // Adjust if offset is negative
    if (start_x < 0)
    {
        overlay_start_x = -start_x;
        start_x = 0;
    }
    if (start_y < 0)
    {
        overlay_start_y = -start_y;
        start_y = 0;
    }

    // Calculate the width and height of the region to copy
    int copy_width = overlay.cols - overlay_start_x;
    int copy_height = overlay.rows - overlay_start_y;

    // Clip to base image boundaries
    if (start_x + copy_width > base.cols)
    {
        copy_width = base.cols - start_x;
    }
    if (start_y + copy_height > base.rows)
    {
        copy_height = base.rows - start_y;
    }

    // Check if there's a valid region to copy
    if (copy_width > 0 && copy_height > 0 && 
        start_x < base.cols && start_y < base.rows &&
        overlay_start_x < overlay.cols && overlay_start_y < overlay.rows)
    {
        // Define ROI on the result (base) image
        cv::Rect roi_base(start_x, start_y, copy_width, copy_height);
        
        // Define ROI on the overlay image
        cv::Rect roi_overlay(overlay_start_x, overlay_start_y, copy_width, copy_height);

        // Copy the overlay region to the base region
        overlay(roi_overlay).copyTo(result(roi_base));
    }

    // Copy result to output
    result.copyTo( out->data() );
}

void
CVOverlayImageModel::
inputConnectionDeleted(QtNodes::ConnectionId const& conx)
{
    auto idx = QtNodes::getPortIndex(PortType::In, conx);
    mvCVImageInData[idx].release();
}
