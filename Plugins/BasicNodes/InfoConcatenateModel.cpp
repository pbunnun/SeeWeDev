#include "InfoConcatenateModel.hpp"

#include "nodes/DataModelRegistry"
#include "InformationData.hpp"
#include "SyncData.hpp"

InfoConcatenateModel::
InfoConcatenateModel()
    : PBNodeDataModel( _model_name )
{
    mpInformationData = std::make_shared< InformationData >( );
}

unsigned int
InfoConcatenateModel::
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
InfoConcatenateModel::
dataType( PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In )
    {
        if(portIndex == 0 || portIndex == 1 )
        {
            return InformationData().type();
        }
        else if(portIndex == 2)
        {
            return SyncData().type();
        }
    }
    else if( portType == PortType::Out )
    {
        return InformationData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData>
InfoConcatenateModel::
outData(PortIndex portIndex)
{
    std::shared_ptr<NodeData> result;
    if( isEnable() )
        result = mpInformationData;
    return result;
}

void
InfoConcatenateModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex)
{
    if( !isEnable() )
        return;
    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< InformationData >( nodeData );
        if( d )
            mpInformationData_1 = d;
    }
    else if(portIndex == 1)
    {
        auto d = std::dynamic_pointer_cast< InformationData >( nodeData );
        if( d )
            mpInformationData_2 = d;
    }
    else if(portIndex == 2)
    {
        if( mpInformationData_1 && mpInformationData_2 )
        {
            QString result = mpInformationData_1->info() + ", " + mpInformationData_2->info();
            mpInformationData->set_information( result );
            //qDebug() << result;
            updateAllOutputPorts();
        }
    }
}

const QString InfoConcatenateModel::_category = QString( "Utility" );

const QString InfoConcatenateModel::_model_name = QString( "Info Concatenate" );
