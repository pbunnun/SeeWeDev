#include "TimerModel.hpp"

TimerModel::
TimerModel()
    : PBNodeDataModel( _model_name, true )
{
    mpSyncData = std::make_shared< SyncData >();

    mpTimer = new QTimer( this );
    connect( mpTimer, &QTimer::timeout, this, &TimerModel::timeout_function );

    IntPropertyType intPropertyType;
    QString propId = "interval";
    intPropertyType.miValue = miMillisecondInterval;
    mpTimer->setInterval( miMillisecondInterval );
    intPropertyType.miMin = 10;
    intPropertyType.miMax = 1000000000;
    auto propInterval = std::make_shared< TypedProperty< IntPropertyType > > ( "Interval (m)", propId, QVariant::Int, intPropertyType );
    mvProperty.push_back( propInterval );
    mMapIdToProperty[ propId ] = propInterval;
}

void
TimerModel::
timeout_function()
{
    Q_EMIT dataUpdated( 0 );
}

unsigned int
TimerModel::
nPorts( PortType portType ) const
{
    if( portType == PortType::Out )
        return 1;
    else
        return 0;
}

NodeDataType
TimerModel::
dataType( PortType, PortIndex portIndex ) const
{
    if( portIndex == 0 )
        return SyncData().type();
    return NodeDataType();
}

std::shared_ptr< NodeData >
TimerModel::
outData( PortIndex )
{
    if( isEnable() )
        return mpSyncData;
    return nullptr;
}

QJsonObject
TimerModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams[ "interval" ] = miMillisecondInterval;

    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
TimerModel::
restore( QJsonObject const & p )
{
    PBNodeDataModel::restore( p );

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "interval" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "interval" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            miMillisecondInterval = v.toInt();
            mpTimer->setInterval( miMillisecondInterval );
        }
    }
}

void
TimerModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "interval" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        miMillisecondInterval = value.toInt();
        mpTimer->setInterval( miMillisecondInterval );
    }
}

void
TimerModel::
enable_changed( bool enable )
{
    if( enable )
        mpTimer->start();
    else
        mpTimer->stop();
}

const QString TimerModel::_category = QString( "Utility" );

const QString TimerModel::_model_name = QString( "Timer" );
