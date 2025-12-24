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

#include "CVMatSumModel.hpp"
#include "CVImageData.hpp"
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>

const QString CVMatSumModel::_category = QString( "cv::Mat Operations" );

const QString CVMatSumModel::_model_name = QString( "CV Sum" );

CVMatSumModel::
CVMatSumModel()
    : PBNodeDelegateModel( _model_name ),
      // PBNodeDataModel( model's name, is it enable at start? )
    _minPixmap(":/Sum.png")
{
    mpIntegerData = std::make_shared< IntegerData >();
}

unsigned int
CVMatSumModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 1 );
    case PortType::Out:
        return( 1 );
    default:
        return( 0 );
    }
}

NodeDataType
CVMatSumModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In && portIndex == 0 )
        return CVImageData().type();
    else if( portType == PortType::Out && portIndex == 0 )
        return IntegerData().type();
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
CVMatSumModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpIntegerData;
    else
        return nullptr;
}

QJsonObject
CVMatSumModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDelegateModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDelegateModel::save();

    return modelJson;
}

void
CVMatSumModel::
load(const QJsonObject &p)
{
    /*
     * If load() was overridden, PBNodeDelegateModel::load() must be called explicitely.
     */
    PBNodeDelegateModel::load(p);

}

void
CVMatSumModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );
}

void
CVMatSumModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if( !isEnable() )
        return;
    
    if( nodeData )
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if( d )
        {
            mpIntegerData->data() = cv::sum( d->data() )[0];
            mpIntegerData->set_information();
            Q_EMIT dataUpdated( 0 );
        }
    }
}


