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

#include "CombineSyncModel.hpp"

#include "nodes/DataModelRegistry"
#include "SyncData.hpp"
#include "SyncData.hpp"
#include "qtvariantproperty.h"

CombineSyncModel::
CombineSyncModel()
    : PBNodeDataModel( _model_name ),
    mpEmbeddedWidget( new QComboBox(qobject_cast<QWidget *>(this)) )
{
    mpSyncData = std::make_shared< SyncData >( );
    mpSyncData_1 = std::make_shared< SyncData >( );
    mpSyncData_2 = std::make_shared< SyncData >( );

    mpEmbeddedWidget->addItem("AND");
    mpEmbeddedWidget->addItem("OR");
    connect(mpEmbeddedWidget, &QComboBox::currentTextChanged, this, &CombineSyncModel::combine_operation_changed);

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"AND", "OR"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "combine_cond";
    auto propComboBox = std::make_shared< TypedProperty< EnumPropertyType > >("Condition", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propComboBox );
    mMapIdToProperty[ propId ] = propComboBox;
}

unsigned int
CombineSyncModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 3;
    else if( portType == PortType::Out )
        return 1;
    else
        return 0;
}

NodeDataType
CombineSyncModel::
dataType( PortType, PortIndex ) const
{
    return SyncData().type();
}

std::shared_ptr<NodeData>
CombineSyncModel::
outData(PortIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
        result = mpSyncData;
    return result;
}

void
CombineSyncModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;
    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( d )
        {
            mpSyncData_1 = d;
            mbReady_1 = true;
//            qDebug() << " Port 0 " << d->data();
        }
    }
    else if(portIndex == 1)
    {
        auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( d )
        {
            mpSyncData_2 = d;
            mbReady_2 = true;
//            qDebug() << " Port 1 " << d->data();
        }
    }

    if(portIndex == 2)
    {
        auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( d && d->data() )
        //if( mpSyncData_1 && mpSyncData_2 )
        {
            mbReady_1 = mbReady_2 = false;
            if( mCombineCondition == CombineCondition_OR )
                mpSyncData->data() = mpSyncData_1->data() || mpSyncData_2->data();
            else if( mCombineCondition == CombineCondition_AND )
                mpSyncData->data() = mpSyncData_1->data() && mpSyncData_2->data();
            updateAllOutputPorts();
        }
    }
    else if( mbReady_1 && mbReady_2 )
    {
        mbReady_1 = mbReady_2 = false;
        if( mCombineCondition == CombineCondition_OR )
            mpSyncData->data() = mpSyncData_1->data() || mpSyncData_2->data();
        else if( mCombineCondition == CombineCondition_AND )
            mpSyncData->data() = mpSyncData_1->data() && mpSyncData_2->data();
        updateAllOutputPorts();
    }
}

void
CombineSyncModel::
combine_operation_changed(const QString & cond)
{
    auto prop = mMapIdToProperty["combine_cond"];
    auto typedProp = std::static_pointer_cast< TypedProperty<EnumPropertyType> >(prop);
    if( cond == "AND" )
    {
        mCombineCondition = CombineCondition_AND;
        typedProp->getData().miCurrentIndex = 0;
    }
    else if( cond == "OR" )
    {
        mCombineCondition = CombineCondition_OR;
        typedProp->getData().miCurrentIndex = 1;
    }

    Q_EMIT property_changed_signal( prop );
}

QJsonObject
CombineSyncModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["combine_cond"] = mCombineCondition;

    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CombineSyncModel::
restore(const QJsonObject &p)
{
    PBNodeDataModel::restore(p);
    late_constructor();

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj["combine_cond"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty["combine_cond"];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mCombineCondition = (CombineCondition)v.toInt();
            mpEmbeddedWidget->setCurrentIndex( v.toInt() );
        }
    }
}

void
CombineSyncModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    auto prop = mMapIdToProperty[ id ];
    if( id == "combine_cond" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > > (prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        mpEmbeddedWidget->setCurrentIndex( value.toInt() );
    }
}

const QString CombineSyncModel::_category = QString( "Utility" );

const QString CombineSyncModel::_model_name = QString( "Combine Sync" );
