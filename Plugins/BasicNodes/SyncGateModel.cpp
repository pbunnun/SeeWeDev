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

#include "SyncGateModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include "qtvariantproperty.h"

SyncGateModel::
SyncGateModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget( new SyncGateEmbeddedWidget ),
      _minPixmap( ":SyncGate.png" )
{
    for(std::shared_ptr<SyncData>& mp : mapSyncData)
    {
        mp = std::make_shared<SyncData>();
    }
    for(std::shared_ptr<BoolData>& mp : mapBoolData)
    {
        mp = std::make_shared<BoolData>( bool() );
    }

    connect(mpEmbeddedWidget,&SyncGateEmbeddedWidget::checkbox_checked_signal, this, &SyncGateModel::em_checkbox_checked);

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"EQUAL","AND","OR","XOR","NOR","NAND","DIRECT","DIRECT_NOT"});
    enumPropertyType.miCurrentIndex = 1;
    QString propId = "operation";
    auto propOperation = std::make_shared< TypedProperty< EnumPropertyType > >( "Operator", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType, "Operation");
    mvProperty.push_back( propOperation );
    mMapIdToProperty[ propId ] = propOperation;

    propId = "checkbox_in0";
    auto propCheckboxIn0 = std::make_shared<TypedProperty<bool>>("",propId,QMetaType::Bool,mpEmbeddedWidget->get_in0_Checkbox());
    mMapIdToProperty[ propId ] = propCheckboxIn0;

    propId = "checkbox_in1";
    auto propCheckboxIn1 = std::make_shared<TypedProperty<bool>>("",propId,QMetaType::Bool,mpEmbeddedWidget->get_in1_Checkbox());
    mMapIdToProperty[ propId ] = propCheckboxIn1;

    propId = "checkbox_out0";
    auto propCheckboxOut0 = std::make_shared<TypedProperty<bool>>("",propId,QMetaType::Bool,mpEmbeddedWidget->get_out0_Checkbox());
    mMapIdToProperty[ propId ] = propCheckboxOut0;

    propId = "checkbox_out1";
    auto propCheckboxOut1 = std::make_shared<TypedProperty<bool>>("",propId,QMetaType::Bool,mpEmbeddedWidget->get_out1_Checkbox());
    mMapIdToProperty[ propId ] = propCheckboxOut1;
}

unsigned int
SyncGateModel::
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
SyncGateModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if(portType == PortType::In)
    {
        if(portIndex == 0)
        {
            return mpEmbeddedWidget->get_in0_Checkbox()?
                   BoolData().type() : SyncData().type();
        }
        else if(portIndex == 1)
        {
            return mpEmbeddedWidget->get_in1_Checkbox()?
                   BoolData().type() : SyncData().type();
        }
    }
    else if(portType == PortType::Out)
    {
        if(portIndex == 0)
        {
            return mpEmbeddedWidget->get_out0_Checkbox()?
                   BoolData().type() : SyncData().type();
        }
        else if(portIndex == 1)
        {
            return mpEmbeddedWidget->get_out1_Checkbox()?
                   BoolData().type() : SyncData().type();
        }
    }
    return NodeDataType();
}


std::shared_ptr<NodeData>
SyncGateModel::
outData(PortIndex I)
{
    if( isEnable() )
    {
        if(I == 0)
        {
            if(mpEmbeddedWidget->get_out0_Checkbox())
                return mapBoolData[0];
            else
                return mapSyncData[0];
        }
        else if(I == 1)
        {
            if(mpEmbeddedWidget->get_out1_Checkbox())
                return mapBoolData[1];
            else
                return mapSyncData[1];
        }
    }
    return nullptr;
}

void
SyncGateModel::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        if(portIndex == 0)
        {
            if(mpEmbeddedWidget->get_in0_Checkbox())
            {
                auto d = std::dynamic_pointer_cast<BoolData>(nodeData);
                if(d)
                {
                    mapBoolInData[0] = d;
                    mapSyncInData[0].reset();
                }
            }
            else
            {
                auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
                if(d)
                {
                    mapSyncInData[0] = d;
                    mapBoolInData[0].reset();
                }
            }
        }
        else if(portIndex == 1)
        {
            if(mpEmbeddedWidget->get_in1_Checkbox())
            {
                auto d = std::dynamic_pointer_cast<BoolData>(nodeData);
                if(d)
                {
                    mapBoolInData[1] = d;
                    mapSyncInData[1].reset();
                }
            }
            else
            {
                auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
                if(d)
                {
                    mapSyncInData[1] = d;
                    mapBoolInData[1].reset();
                }
            }
        }
        if((mapBoolInData[0]||mapSyncInData[0])&&(mapBoolInData[1]||mapSyncInData[1]))
        {
            processData(mapSyncInData,mapBoolInData,mapSyncData,mapBoolData,mParams);
        }
    }

    updateAllOutputPorts();
}

QJsonObject
SyncGateModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["operation"] = mParams.miOperation;
    cParams["checkboxIn0"] = mpEmbeddedWidget->get_in0_Checkbox();
    cParams["checkboxIn1"] = mpEmbeddedWidget->get_in1_Checkbox();
    cParams["checkboxOut1"] = mpEmbeddedWidget->get_out0_Checkbox();
    cParams["checkboxOut1"] = mpEmbeddedWidget->get_out1_Checkbox();
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
SyncGateModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "operation" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "operation" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mParams.miOperation = v.toInt();
        }
        v = paramsObj[ "checkboxIn0" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "checkbox_in0" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool >>(prop);
            typedProp->getData() = v.toBool();

            mpEmbeddedWidget->set_in0_Checkbox(v.toBool());
        }
        v = paramsObj[ "checkboxIn1" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "checkbox_in1" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool >>(prop);
            typedProp->getData() = v.toBool();

            mpEmbeddedWidget->set_in1_Checkbox(v.toBool());
        }
        v = paramsObj[ "checkboxOut0" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "checkbox_out0" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool >>(prop);
            typedProp->getData() = v.toBool();

            mpEmbeddedWidget->set_out0_Checkbox(v.toBool());
        }
        v = paramsObj[ "checkboxOut0" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "checkbox_out1" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool >>(prop);
            typedProp->getData() = v.toBool();

            mpEmbeddedWidget->set_out1_Checkbox(v.toBool());
        }
    }
}

void
SyncGateModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "operation" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty <EnumPropertyType>>(prop);
        typedProp->getData().miCurrentIndex = value.toInt();

        mParams.miOperation = value.toInt();
    }
    else if( id == "checkbox_in0" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool >>(prop);
        typedProp->getData() = value.toBool();

        mpEmbeddedWidget->set_in0_Checkbox(value.toBool());
    }
    else if( id == "checkbox_in1" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool >>(prop);
        typedProp->getData() = value.toBool();

        mpEmbeddedWidget->set_in1_Checkbox(value.toBool());
    }
    else if( id == "checkbox_out0" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool >>(prop);
        typedProp->getData() = value.toBool();

        mpEmbeddedWidget->set_out0_Checkbox(value.toBool());
    }
    else if( id == "checkbox_out1" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool >>(prop);
        typedProp->getData() = value.toBool();

        mpEmbeddedWidget->set_out1_Checkbox(value.toBool());
    }

    if(true)
    {
        processData(mapSyncInData,mapBoolInData,mapSyncData,mapBoolData,mParams);
        updateAllOutputPorts();
    }
}

void SyncGateModel::em_checkbox_checked()
{
    if(mpEmbeddedWidget->get_in0_Checkbox())
    {
        if(mpEmbeddedWidget->get_in1_Checkbox())
        {
            mapSyncInData[0].reset();
            mapSyncInData[1].reset();
        }
        else
        {
            mapSyncInData[0].reset();
            mapBoolInData[1].reset();
        }
    }
    else
    {
        if(mpEmbeddedWidget->get_in1_Checkbox())
        {
            mapBoolInData[0].reset();
            mapSyncInData[1].reset();
        }
        else
        {
            mapBoolInData[0].reset();
            mapBoolInData[1].reset();
        }
    }
    if((mapBoolInData[0]||mapSyncInData[0])&&(mapBoolInData[1]||mapSyncInData[1]))
    {
        processData(mapSyncInData,mapBoolInData,mapSyncData,mapBoolData,mParams);
        updateAllOutputPorts();
    }
}

void
SyncGateModel::
processData(const std::shared_ptr<SyncData> (&inSync)[2], const std::shared_ptr<BoolData> (&inBool)[2],
                     std::shared_ptr<SyncData> (&outSync)[2], std::shared_ptr<BoolData> (&outBool)[2],
                     const SyncGateParameters & params)
{
    const bool& in0 = inSync[0]? inSync[0]->data() : inBool[0]->boolean();
    const bool& in1 = inSync[1]? inSync[1]->data() : inBool[1]->boolean();
    bool out0;
    bool out1;
    switch(params.miOperation)
    {
    case LogicGate::EQUAL :
        out0 = out1 = (in0 == in1);
        break;

    case LogicGate::AND :
        out0 = out1 = (in0 && in1);
        break;

    case LogicGate::OR :
        out0 = out1 = (in0 || in1);
        break;

    case LogicGate::XOR :
        out0 = out1 = !(in0 == in1);
        break;

    case LogicGate::NOR :
        out0 = out1 = !(in0 || in1);
        break;

    case LogicGate::NAND :
        out0 = out1 = !(in0 && in1);
        break;

    case LogicGate::DIRECT :
        out0 = in0;
        out1 = in1;
        break;

    case LogicGate::DIRECT_NOT :
        out0 = !in0;
        out1 = !in1;
        break;
    }
    if(mpEmbeddedWidget->get_out0_Checkbox())
    {
        if(mpEmbeddedWidget->get_out1_Checkbox())
        {
            outBool[0]->boolean() = out0;
            outBool[1]->boolean() = out1;
        }
        else
        {
            outBool[0]->boolean() = out0;
            outSync[0]->data() = out1;
        }
    }
    else
    {
        if(mpEmbeddedWidget->get_out1_Checkbox())
        {
            outSync[0]->data() = out0;
            outBool[1]->boolean() = out1;
        }
        else
        {
            outSync[0]->data() = out0;
            outSync[1]->data() = out1;
        }
    }
}

const QString SyncGateModel::_category = QString( "Number Operation" );

const QString SyncGateModel::_model_name = QString( "Sync Gate" );
