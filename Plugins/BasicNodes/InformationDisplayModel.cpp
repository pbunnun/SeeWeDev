#include "InformationDisplayModel.hpp"

#include "nodes/DataModelRegistry"
#include "InformationData.hpp"
#include "SyncData.hpp"

InformationDisplayModel::
InformationDisplayModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget( new InformationDisplayEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{

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
            mpInformationData = d;
            d->set_information();
            mpEmbeddedWidget->appendPlainText( d->info() );
        }
    }
    if(portIndex == 1)
    {
        auto d = std::dynamic_pointer_cast< SyncData >( nodeData );
        if( d )
        {
            mpEmbeddedWidget->appendPlainText(d->state_str() + "\n");
        }
    }
}

const QString InformationDisplayModel::_category = QString( "Output" );

const QString InformationDisplayModel::_model_name = QString( "Info Display" );
