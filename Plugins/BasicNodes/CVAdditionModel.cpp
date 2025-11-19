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

#include "CVAdditionModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>
#include <QtNodes/internal/ConnectionIdUtils.hpp>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "qtvariantproperty_p.h"

const QString CVAdditionModel::_category = QString( "Image Operation" );

const QString CVAdditionModel::_model_name = QString( "CV Addition" );

CVAdditionModel::
CVAdditionModel()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":Addition.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mvCVImageInData.resize(3);
}

unsigned int
CVAdditionModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 3;
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
CVAdditionModel::
dataType(PortType, PortIndex) const
{
    return CVImageData().type();
}


std::shared_ptr<NodeData>
CVAdditionModel::
outData(PortIndex)
{
    if( isEnable() && mpCVImageData->data().empty() == false )
        return mpCVImageData;
    else
        return nullptr;
}

void
CVAdditionModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d)
        {
            d->data().copyTo( mvCVImageInData[portIndex] );
            if( !mvCVImageInData[0].empty() && !mvCVImageInData[1].empty() &&
                ( !mbMaskActive || !mvCVImageInData[2].empty() ) )
            {
                processData(mvCVImageInData, mpCVImageData);

                Q_EMIT dataUpdated(0);
            }
        }
    }
}

QJsonObject
CVAdditionModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    return modelJson;
}

void
CVAdditionModel::
load(QJsonObject const& p)
{
    PBNodeDelegateModel::load(p);
}

void
CVAdditionModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    // No additional properties for this model
}

void
CVAdditionModel::
processData(const std::vector< cv::Mat >&in, std::shared_ptr<CVImageData> &out)
{
    if(in[0].type() != in[1].type())
    {
        return;
    }

    cv::Mat temp;
    if(mbMaskActive && !in[2].empty() && in[2].type() == CV_8UC1)
    {
        cv::add(in[0], in[1], temp, in[2]);
    }
    else
    {
        cv::add(in[0], in[1], temp);
    }
    temp.copyTo( out->data() );
}

void
CVAdditionModel::
inputConnectionCreated(QtNodes::ConnectionId const& conx)
{
    auto idx = QtNodes::getPortIndex(PortType::In, conx);
    if( idx == 2 )
        mbMaskActive = true;
}

void
CVAdditionModel::
inputConnectionDeleted(QtNodes::ConnectionId const& conx)
{
    auto idx = QtNodes::getPortIndex(PortType::In, conx);
    mvCVImageInData[idx].release();
    if( idx == 2 )
    {
        mbMaskActive = false;
        if( !mvCVImageInData[0].empty() && !mvCVImageInData[1].empty() )
        {
            processData(mvCVImageInData, mpCVImageData);

            Q_EMIT dataUpdated(0);
        }
    }
}
