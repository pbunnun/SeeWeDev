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

#include "MathConditionModel.hpp"
#include "qtvariantproperty_p.h"
#include "IntegerData.hpp"

const QString MathConditionModel::_category = QString( "Math Operation" );

const QString MathConditionModel::_model_name = QString( "Condition" );

MathConditionModel::
MathConditionModel()
    : PBNodeDelegateModel( _model_name ),
      // PBNodeDataModel( model's name, is it enable at start? )
      mpEmbeddedWidget( new MathConditionEmbeddedWidget( qobject_cast<QWidget *>(this) ) ),
    _minPixmap(":/Condition.png")
{
    connect( mpEmbeddedWidget, &MathConditionEmbeddedWidget::condition_changed_signal, this, &MathConditionModel::em_changed );
    mpSyncData = std::make_shared< SyncData >( false );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = mpEmbeddedWidget->get_condition_string_list();
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "cond_combobox_id";
    auto propComboBox = std::make_shared< TypedProperty< EnumPropertyType > >("Condition", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propComboBox );
    mMapIdToProperty[ propId ] = propComboBox;

    propId = "cond_number_id";
    auto propDisplayText = std::make_shared< TypedProperty< QString > >("Number", propId, QMetaType::QString, msConditionNumber );
    mvProperty.push_back( propDisplayText );
    mMapIdToProperty[ propId ] = propDisplayText;
}

unsigned int
MathConditionModel::
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
MathConditionModel::
dataType(PortType portType, PortIndex) const
{
    if( portType == PortType::Out )
        return SyncData().type();
    else if( portType == PortType::In )
        return IntegerData().type();
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
MathConditionModel::
outData(PortIndex)
{
    if( isEnable() )
        return mpSyncData;
    else
        return nullptr;
}

void
MathConditionModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if( nodeData )
    {
        auto d = std::dynamic_pointer_cast< IntegerData >( nodeData );
        if( d )
        {
            auto value = d->data();
            if( miConditionIndex == 0 ) // >
            {
                if( value > mdConditionNumber )
                    mpSyncData->data() = true;
                else
                    mpSyncData->data() = false;
            }
            else if( miConditionIndex == 1 ) // >=
            {
                if( value >= mdConditionNumber )
                    mpSyncData->data() = true;
                else
                    mpSyncData->data() = false;
            }
            else if( miConditionIndex == 2 ) // <
            {
                if( value < mdConditionNumber )
                    mpSyncData->data() = true;
                else
                    mpSyncData->data() = false;
            }
            else if( miConditionIndex == 3 ) // <=
            {
                if( value <= mdConditionNumber )
                    mpSyncData->data() = true;
                else
                    mpSyncData->data() = false;
            }
            else if( miConditionIndex == 4 ) // =
            {
                if( value == mdConditionNumber )
                    mpSyncData->data() = true;
                else
                    mpSyncData->data() = false;
            }
            else if( miConditionIndex == 5 ) // !=
            {
                if( value != mdConditionNumber )
                    mpSyncData->data() = true;
                else
                    mpSyncData->data() = false;
            }
        }
        Q_EMIT dataUpdated( 0 );
    }
}

QJsonObject
MathConditionModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDelegateModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "cond_combobox_id" ] = miConditionIndex;
    cParams[ "cond_number_id" ] = msConditionNumber;

    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
MathConditionModel::
load(const QJsonObject &p)
{
    /*
     * If load() was overridden, PBNodeDelegateModel::load() must be called explicitely.
     */
    PBNodeDelegateModel::load(p);
    late_constructor();

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "cond_combobox_id" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "cond_combobox_id" ];
            /* Restore internal property */
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            miConditionIndex = v.toInt();
            typedProp->getData().miCurrentIndex = miConditionIndex;
            /* Restore mpEmbeddedWidget */
            mpEmbeddedWidget->set_condition_text_index( miConditionIndex );
        }

        v = paramsObj[ "cond_number_id" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "cond_number_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = v.toString();

            msConditionNumber = v.toString();
            mdConditionNumber = v.toDouble();
            mpEmbeddedWidget->set_condition_number( msConditionNumber );
        }
    }
}

void
MathConditionModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "cond_combobox_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType> >( prop );
        miConditionIndex = value.toInt();
        typedProp->getData().miCurrentIndex = miConditionIndex;

        mpEmbeddedWidget->set_condition_text_index( miConditionIndex );
    }
    else if( id == "cond_number_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();

        msConditionNumber = value.toString();
        mdConditionNumber = value.toDouble();
        mpEmbeddedWidget->set_condition_number( msConditionNumber );
    }
}

void
MathConditionModel::
em_changed( int cond_idx, QString number )
{
    miConditionIndex = cond_idx;
    msConditionNumber = number;
    mdConditionNumber = number.toDouble();

    auto prop = mMapIdToProperty[ "cond_combobox_id" ];
    auto enumTypedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
    if( enumTypedProp )
    {
        enumTypedProp->getData().miCurrentIndex = miConditionIndex;
        Q_EMIT property_changed_signal( prop );
    }

    prop = mMapIdToProperty[ "cond_number_id" ];
    auto stringTypedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
    if( stringTypedProp )
    {
        stringTypedProp->getData() = msConditionNumber;
        Q_EMIT property_changed_signal( prop );
    }
    Q_EMIT embeddedWidgetSizeUpdated();
}


