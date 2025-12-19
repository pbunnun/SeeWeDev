#include "PushButtonModel.hpp"
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>

const QString PushButtonModel::_category = QString( "GUI" );

const QString PushButtonModel::_model_name = QString( "Push Button" );

PushButtonModel::
PushButtonModel()
    : PBNodeDelegateModel( _model_name, true ),
    mpEmbeddedWidget( new QPushButton( qobject_cast<QWidget *>(this) ) ),
    _minPixmap(":/PushButtonModel.png")
{
    mpEmbeddedWidget->setStyleSheet(
                "QPushButton { "
                "  background-color: yellow; "
                "  border: 1px solid #8f8f91; "
                "  border-radius: 3px; "
                "}"
                "QPushButton:disabled { "
                "  background-color: red; "
                "}"
                "QPushButton:pressed { "
                "  background-color: green; "
                "}"
                "QPushButton:checked { "
                "  background-color: green; "
                "  border: 1px solid #5a5a5c; "
                "}"
                );
    
    connect( mpEmbeddedWidget, &QPushButton::clicked, this, &PushButtonModel::em_button_clicked );
    mpSyncData = std::make_shared< SyncData >();
    mpIntData = std::make_shared< IntegerData >();

    mpEmbeddedWidget->setText("OK");
    QFont font = mpEmbeddedWidget->font();
    font.setPointSize(12);
    mpEmbeddedWidget->setFont(font);
    
    // Calculate minimum size based on caption and ports
    // This ensures the widget fits properly within the node boundaries
    QSize minSize = calculateMinimumWidgetSize(_model_name, 0, 2);
    mpEmbeddedWidget->setMinimumSize(minSize);

    QString propId = "label";
    auto propLabel = std::make_shared< TypedProperty< QString > > ("Label", propId, QMetaType::QString, "OK");
    mvProperty.push_back( propLabel );
    mMapIdToProperty[ propId ] = propLabel;

    IntPropertyType intPropertyType;
    intPropertyType.miMax = 300;
    intPropertyType.miMin = 1;
    intPropertyType.miValue = 12;
    propId = "fontsize";
    auto propFontSize = std::make_shared< TypedProperty < IntPropertyType > > ("Font Size", propId, QMetaType::Int, intPropertyType );
    mvProperty.push_back( propFontSize );
    mMapIdToProperty[ propId ] = propFontSize;

    propId = "checkable";
    auto propCheckable = std::make_shared< TypedProperty < bool > >("Checkable", propId, QMetaType::Bool, false);
    mvProperty.push_back( propCheckable );
    mMapIdToProperty[ propId ] = propCheckable;

    propId = "int_out";
    intPropertyType.miMax = 10000;
    intPropertyType.miMin = 0;
    intPropertyType.miValue = mpIntData->data();
    auto propIntOut = std::make_shared< TypedProperty < IntPropertyType > >("Int Out", propId, QMetaType::Int, intPropertyType );
    mvProperty.push_back( propIntOut );
    mMapIdToProperty[ propId ] = propIntOut;
}

unsigned int
PushButtonModel::
nPorts( PortType portType ) const
{
    switch( portType )
    {
    case PortType::In:
        return( 0 );
    case PortType::Out:
        return( 2 );
    default:
        return( 0 );
    }
}

NodeDataType
PushButtonModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::Out )
    {
        if( portIndex == 0 )
            return SyncData().type();
        else if( portIndex == 1 )
            return IntegerData().type();
        else
            return NodeDataType();
    }
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
PushButtonModel::
outData(PortIndex idx)
{
    if( isEnable() )
    {
        if( idx == 0 )
            return mpSyncData;
        else if( idx == 1 )
            return mpIntData;
    }
    return nullptr;
}

QJsonObject
PushButtonModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDelegateModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "label" ] = mpEmbeddedWidget->text();
    cParams[ "fontsize" ] = mpEmbeddedWidget->font().pointSize();
    cParams[ "int_out" ] = mpIntData->data();
    cParams[ "checkable" ] = mpEmbeddedWidget->isCheckable();
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
PushButtonModel::
load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();

    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "label" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "label" ];
            /* Restore internal property */
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = v.toString();
            /* Restore mpEmbeddedWidget */
            mpEmbeddedWidget->setText( v.toString() );
        }
        v = paramsObj[ "fontsize" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "fontsize" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            QFont font = mpEmbeddedWidget->font();
            font.setPointSize( v.toInt() );
            mpEmbeddedWidget->setFont(font);
        }
        v = paramsObj[ "int_out" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "int_out" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mpIntData->data() = v.toInt();
        }
        v = paramsObj["checkable"];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "checkable" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();
            mpEmbeddedWidget->setCheckable( v.toBool() );
        }
    }
    mpEmbeddedWidget->setEnabled( isEnable() );
}

void
PushButtonModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "label" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();

        mpEmbeddedWidget->setText( value.toString() );
    }
    else if( id == "fontsize" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();
        QFont font = mpEmbeddedWidget->font();
        font.setPointSize( value.toInt() );
        mpEmbeddedWidget->setFont(font);
    }
    else if( id == "checkable" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mpEmbeddedWidget->setCheckable( value.toBool() );
    }
    else if( id == "int_out" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mpIntData->data() = value.toInt();
    }
#if 0
    else if( id == "size_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
        mSize = value.toSize();

        typedProp->getData().miWidth = mSize.width();
        typedProp->getData().miHeight = mSize.height();
    }
    else if( id == "point_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        mPoint = value.toPoint();

        typedProp->getData().miXPosition = mPoint.x();
        typedProp->getData().miYPosition = mPoint.y();
    }
#endif
}

void
PushButtonModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed( enable );

    mpEmbeddedWidget->setEnabled(enable);
}

void
PushButtonModel::
em_button_clicked( )
{
    if( mpEmbeddedWidget->isCheckable() )
        mpSyncData->data() = mpEmbeddedWidget->isChecked();
    else
        mpSyncData->data() = true;
    updateAllOutputPorts();
 //   Q_EMIT dataUpdated( 0 );
#if 0
    if( button == 0 ) //Start
    {
        auto prop = mMapIdToProperty[ "enable" ];
        /*
         * Update internal property.
         */
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = true;

        /*
         * Emiting property_changed_signal will send a signal to QtPropertyBrowser
         * and it will update its parameters accordingly.
         */
        Q_EMIT property_changed_signal( prop );

        enable_changed( true );
    }
    else if( button == 1 ) //Stop
    {
        auto prop = mMapIdToProperty[ "enable" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = false;
        Q_EMIT property_changed_signal( prop );

        enable_changed( false );
    }
    else if( button == 2 )
    {
        auto prop = mMapIdToProperty[ "spinbox_id" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = mpEmbeddedWidget->get_spinbox()->value();
        Q_EMIT property_changed_signal( prop );
    }
    else if( button == 3 )
    {
        auto prop = mMapIdToProperty[ "combobox_id" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( mpEmbeddedWidget->get_combobox_text() );
        Q_EMIT property_changed_signal( prop );
    }
    // Notify node's NodeGraphicsObject to redraw itself.
    Q_EMIT embeddedWidgetStatusUpdated();
#endif
}


