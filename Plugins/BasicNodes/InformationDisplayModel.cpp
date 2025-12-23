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

#include "InformationDisplayModel.hpp"

#include "InformationData.hpp"
#include "SyncData.hpp"

const QString InformationDisplayModel::_category = QString( "Output" );

const QString InformationDisplayModel::_model_name = QString( "Info Display" );

InformationDisplayModel::
InformationDisplayModel()
    : PBNodeDelegateModel( _model_name ),
      mpEmbeddedWidget( new InformationDisplayEmbeddedWidget( qobject_cast<QWidget *>(this) ) ),
    _minPixmap(":/Info Display.png")
{
    IntPropertyType intPropertyType;
    intPropertyType.miMax = 2000;
    intPropertyType.miMin = 10;
    intPropertyType.miValue = miMaxLineCount;
    QString propId = "max_line_count";
    auto propMaxLineCount = std::make_shared< TypedProperty< IntPropertyType > >( "Max Line Count", propId, QMetaType::Int, intPropertyType );
    mvProperty.push_back( propMaxLineCount );
    mMapIdToProperty[ propId ] = propMaxLineCount;

    mpEmbeddedWidget->setMaxLineCount(miMaxLineCount);

    connect( mpEmbeddedWidget, &InformationDisplayEmbeddedWidget::widgetClicked, 
        this, &InformationDisplayModel::selection_request_signal);
}

unsigned int
InformationDisplayModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 2;
    else
        return 0;
}

NodeDataType
InformationDisplayModel::
dataType( PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
    {
        return InformationData().type();
    }
    else if(portIndex == 1)
    {
        return SyncData().type();
    }
    return NodeDataType();
}

void
InformationDisplayModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;
    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< InformationData >( nodeData );
        if( d )
        {
            d->set_information();
            mpEmbeddedWidget->appendPlainText("............................................\n");
            mpEmbeddedWidget->appendPlainText( d->info() );
        }
    }
    else if(portIndex == 1)
    {
        auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( d )
        {
            mpEmbeddedWidget->appendPlainText(d->state_str() + "\n");
        }
    }
}

void
InformationDisplayModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( id == "max_line_count" )
    {
        auto prop = mMapIdToProperty[ id ];
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >(prop);
        typedProp->getData().miValue = value.toInt();

        miMaxLineCount = value.toInt();
        mpEmbeddedWidget->setMaxLineCount(miMaxLineCount);
    }
}


