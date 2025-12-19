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

#include "CombineSyncModel.hpp"

#include "SyncData.hpp"
#include "CombineSyncEmbeddedWidget.hpp"
#include "qtvariantproperty_p.h"

const QString CombineSyncModel::_category = QString( "Utility" );

const QString CombineSyncModel::_model_name = QString( "Combine Sync" );

CombineSyncModel::
CombineSyncModel()
    : PBNodeDelegateModel( _model_name ),
    mpEmbeddedWidget( new CombineSyncEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{
    mpSyncData = std::make_shared< SyncData >( );
    
    // Initialize vectors with default size of 2
    miInputSize = 2;
    mvbReady.resize(miInputSize, false);
    mvSyncValues.resize(miInputSize, false);

    // Connect widget signals
    connect(mpEmbeddedWidget, &CombineSyncEmbeddedWidget::operation_changed_signal, 
            this, &CombineSyncModel::combine_operation_changed);
    connect(mpEmbeddedWidget, &CombineSyncEmbeddedWidget::input_size_changed_signal, 
            this, &CombineSyncModel::input_size_changed);
    connect(mpEmbeddedWidget, &CombineSyncEmbeddedWidget::reset_clicked_signal, 
            this, &CombineSyncModel::reset_clicked);

    // Properties
    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"AND", "OR"});
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "combine_cond";
    auto propComboBox = std::make_shared< TypedProperty< EnumPropertyType > >("Condition", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propComboBox );
    mMapIdToProperty[ propId ] = propComboBox;

    // Input size property
    IntPropertyType intPropertyType;
    intPropertyType.miValue = miInputSize;
    intPropertyType.miMin = 2;
    intPropertyType.miMax = 10;
    QString propId2 = "input_size";
    auto propInputSize = std::make_shared< TypedProperty< IntPropertyType > >("Input Size", propId2, QMetaType::Int, intPropertyType);
    mvProperty.push_back( propInputSize );
    mMapIdToProperty[ propId2 ] = propInputSize;
}

unsigned int
CombineSyncModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return miInputSize;
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
    std::shared_ptr<NodeData> result{nullptr};
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
    
    // Ensure portIndex is valid
    if( portIndex >= miInputSize )
        return;

    auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
    if( d )
    {
        // Store the bool value, not the shared pointer
        mvSyncValues[portIndex] = d->data();
        mvbReady[portIndex] = true;
    }

    // Check if all inputs are ready
    bool allReady = true;
    for( unsigned int i = 0; i < miInputSize; ++i )
    {
        if( !mvbReady[i] )
        {
            allReady = false;
            break;
        }
    }

    if( allReady )
    {
        // Reset ready flags
        std::fill(mvbReady.begin(), mvbReady.end(), false);

        // Compute the combined result
        bool result;
        if( mCombineCondition == CombineCondition_AND )
        {
            // AND: all must be true
            result = true;
            for( unsigned int i = 0; i < miInputSize; ++i )
            {
                if( !mvSyncValues[i] )
                {
                    result = false;
                    break;
                }
            }
        }
        else // CombineCondition_OR
        {
            // OR: at least one must be true
            result = false;
            for( unsigned int i = 0; i < miInputSize; ++i )
            {
                if( mvSyncValues[i] )
                {
                    result = true;
                    break;
                }
            }
        }

        mpSyncData->data() = result;
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

void
CombineSyncModel::
input_size_changed(int size)
{
    if( size < 2 )
        size = 2;
    
    unsigned int newSize = static_cast<unsigned int>(size);
    
    // Handle port count changes
    if( newSize > miInputSize )
    {
        // Adding ports
        portsAboutToBeInserted(PortType::In, miInputSize, newSize - 1);
        miInputSize = newSize;
        mvbReady.resize(miInputSize, false);
        mvSyncValues.resize(miInputSize, false);
        portsInserted();
    }
    else if( newSize < miInputSize )
    {
        // Removing ports
        portsAboutToBeDeleted(PortType::In, newSize, miInputSize - 1);
        miInputSize = newSize;
        mvbReady.resize(miInputSize, false);
        mvSyncValues.resize(miInputSize, false);
        portsDeleted();
    }

    // Update property
    auto prop = mMapIdToProperty["input_size"];
    auto typedProp = std::static_pointer_cast< TypedProperty<IntPropertyType> >(prop);
    typedProp->getData().miValue = size;

    Q_EMIT property_changed_signal( prop );
    
    // Notify that the embedded widget size may have changed (triggers geometry recalculation)
    Q_EMIT embeddedWidgetSizeUpdated();
}

void
CombineSyncModel::
reset_clicked()
{
    // Reset all ready states to false
    std::fill(mvbReady.begin(), mvbReady.end(), false);
    
    // Reset all sync values to false
    std::fill(mvSyncValues.begin(), mvSyncValues.end(), false);
}

QJsonObject
CombineSyncModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams["combine_cond"] = mCombineCondition;
    cParams["input_size"] = static_cast<int>(miInputSize);

    modelJson["cParams"] = cParams;

    return modelJson;
}

void
CombineSyncModel::
load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);

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
            mpEmbeddedWidget->set_operation(v.toInt());
        }

        v = paramsObj["input_size"];
        if( !v.isNull() )
        {
            int size = v.toInt();
            if( size < 2 )
                size = 2;

            miInputSize = static_cast<unsigned int>(size);
            mvbReady.resize(miInputSize, false);
            mvSyncValues.resize(miInputSize, false);

            auto prop = mMapIdToProperty["input_size"];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = size;

            mpEmbeddedWidget->set_input_size(size);
        }
    }
}

void
CombineSyncModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( id == "combine_cond" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > > (prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        mCombineCondition = (CombineCondition)value.toInt();
        mpEmbeddedWidget->set_operation(value.toInt());
    }
    else if( id == "input_size" )
    {
        int size = value.toInt();
        if( size < 2 )
            size = 2;

        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > > (prop);
        typedProp->getData().miValue = size;

        mpEmbeddedWidget->set_input_size(size);
        
        // Update internal state with proper port notifications
        unsigned int newSize = static_cast<unsigned int>(size);
        
        if( newSize > miInputSize )
        {
            // Adding ports
            portsAboutToBeInserted(PortType::In, miInputSize, newSize - 1);
            miInputSize = newSize;
            mvbReady.resize(miInputSize, false);
            mvSyncValues.resize(miInputSize, false);
            portsInserted();
        }
        else if( newSize < miInputSize )
        {
            // Removing ports
            portsAboutToBeDeleted(PortType::In, newSize, miInputSize - 1);
            miInputSize = newSize;
            mvbReady.resize(miInputSize, false);
            mvSyncValues.resize(miInputSize, false);
            portsDeleted();
        }
        
        // Notify that the embedded widget size may have changed
        Q_EMIT embeddedWidgetSizeUpdated();
    }
}


