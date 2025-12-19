#include "LCDNumberModel.hpp"
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>

const QString LCDNumberModel::_category = QString( "GUI" );

const QString LCDNumberModel::_model_name = QString( "LCD Number" );

LCDNumberModel::
LCDNumberModel()
    : PBNodeDelegateModel( _model_name ),
      // PBNodeDataModel( model's name, is it source data, is it enable at start? )
      mpEmbeddedWidget( new QLCDNumber( qobject_cast<QWidget *>(this) ) ),
    _minPixmap(":/LCDNumberModel.png")
{
    mpIntData = std::make_shared< IntegerData >();

    IntPropertyType intPropertyType;
    intPropertyType.miMax = 10;
    intPropertyType.miMin = 1;
    intPropertyType.miValue = miDigitCount;
    QString propId = "digitCount";
    auto propDigitCount = std::make_shared< TypedProperty < IntPropertyType > > ("Digit Count", propId, QMetaType::Int, intPropertyType );
    mvProperty.push_back( propDigitCount );
    mMapIdToProperty[ propId ] = propDigitCount;

    // Calculate minimum size based on caption and ports
    // This ensures the widget fits properly within the node boundaries
    QSize minSize = calculateMinimumWidgetSize(_model_name, 1, 0);
    mpEmbeddedWidget->setMinimumSize(minSize);
}

unsigned int
LCDNumberModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 1 );
    default:
        return( 0 );
    }
}

NodeDataType
LCDNumberModel::
dataType( PortType portType, PortIndex ) const
{
    if( portType == PortType::In )
        return IntegerData().type();
    else
        return NodeDataType();
}

QJsonObject
LCDNumberModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDelegateModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "digitCount" ] = miDigitCount;
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
LCDNumberModel::
load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();

    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "digitCount" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "digitCount" ];
            /* Restore internal property */
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();
            /* Restore mpEmbeddedWidget */
            miDigitCount = v.toInt();
            mpEmbeddedWidget->setDigitCount( v.toInt() );
        }
    }
    mpEmbeddedWidget->setEnabled( isEnable() );
}

void
LCDNumberModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "digitCount" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();
        miDigitCount = value.toInt();
        mpEmbeddedWidget->setDigitCount( value.toInt() );
    }
}

void
LCDNumberModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex )
{
    if( !isEnable() )
        return;
    if(portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast< IntegerData >( nodeData );
        if( d )
            mpEmbeddedWidget->display( d->data() );
    }
}

void
LCDNumberModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed( enable );

    mpEmbeddedWidget->setEnabled(enable);
}


