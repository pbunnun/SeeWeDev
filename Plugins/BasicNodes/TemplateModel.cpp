#include "TemplateModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>
#include "qtvariantproperty.h"

TemplateModel::
TemplateModel()
    : PBNodeDataModel( _model_name ),
      // PBNodeDataModel( model's name, is it enable at start? )
      mpEmbeddedWidget( new TemplateEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &TemplateEmbeddedWidget::button_clicked_signal, this, &TemplateModel::em_button_clicked );
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = mpEmbeddedWidget->get_combobox_string_list();
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "combobox_id";
    auto propComboBox = std::make_shared< TypedProperty< EnumPropertyType > >("ComboBox", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propComboBox );
    mMapIdToProperty[ propId ] = propComboBox;

    IntPropertyType intPropertyType;
    intPropertyType.miMax = mpEmbeddedWidget->get_spinbox()->maximum();
    intPropertyType.miMin = mpEmbeddedWidget->get_spinbox()->minimum();
    intPropertyType.miValue = mpEmbeddedWidget->get_spinbox()->value();
    propId = "spinbox_id";
    auto propSpinBox = std::make_shared< TypedProperty< IntPropertyType > >("SpinBox", propId, QVariant::Int, intPropertyType, "SubProp0" );
    mvProperty.push_back( propSpinBox );
    mMapIdToProperty[ propId ] = propSpinBox;

    propId = "checkbox_id";
    auto propCheckBox = std::make_shared< TypedProperty< bool > >("CheckBox", propId, QVariant::Bool, mbCheckBox, "SubProp1" );
    mvProperty.push_back( propCheckBox );
    mMapIdToProperty[ propId ] = propCheckBox;

    propId = "display_id";
    auto propDisplayText = std::make_shared< TypedProperty< QString > >("Text", propId, QVariant::String, msDisplayText, "SubProp1" );
    mvProperty.push_back( propDisplayText );
    mMapIdToProperty[ propId ] = propDisplayText;

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = mSize.width();
    sizePropertyType.miHeight = mSize.height();
    propId = "size_id";
    auto propSize = std::make_shared< TypedProperty< SizePropertyType > >("Size", propId, QVariant::Size, sizePropertyType );
    mvProperty.push_back( propSize );
    mMapIdToProperty[ propId ] = propSize;

    PointPropertyType pointPropertyType;
    pointPropertyType.miXPosition = mPoint.x();
    pointPropertyType.miYPosition = mPoint.y();
    propId = "point_id";
    auto propPoint = std::make_shared< TypedProperty< PointPropertyType > >("Point", propId, QVariant::Point, pointPropertyType );
    mvProperty.push_back( propPoint );
    mMapIdToProperty[ propId ] = propPoint;
}

unsigned int
TemplateModel::
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
TemplateModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::Out && portIndex == 0 )
        return CVImageData().type();
    else if( portType == PortType::In && portIndex == 0 )
        return CVImageData().type();
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
TemplateModel::
outData(PortIndex)
{
    if( isEnable() && mpCVImageData->image().data != nullptr )
        return mpCVImageData;
    else
        return nullptr;
}

void
TemplateModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if( nodeData )
    {
        /*
         * Do something with an incoming data.
         */
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
        {
            mpCVImageData->set_image( d->image() );
        }
    }

    /*
     * Emit dataUpdated( _data_out_channel_no_ ) to tell other models who link with the model's output channel
     * that there is data ready to read out.
     */
    Q_EMIT dataUpdated( 0 );
}

QJsonObject
TemplateModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDataModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams[ "combobox_text" ] = mpEmbeddedWidget->get_combobox_text();
    cParams[ "spinbox_value" ] = mpEmbeddedWidget->get_spinbox()->value();
    cParams[ "checkbox_value" ] = mbCheckBox;
    cParams[ "display_text"] = msDisplayText;
    cParams[ "size_width" ] = mSize.width();
    cParams[ "size_height" ] = mSize.height();
    cParams[ "point_x" ] = mPoint.x();
    cParams[ "point_y" ] = mPoint.y();

    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
TemplateModel::
restore(const QJsonObject &p)
{
    /*
     * If restore() was overrided, PBNodeDataModel::restore() must be called explicitely.
     */
    PBNodeDataModel::restore(p);
    late_constructor();

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "combobox_text" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "combobox_id" ];
            /* Restore internal property */
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( v.toString() );
            /* Restore mpEmbeddedWidget */
            mpEmbeddedWidget->set_combobox_value( v.toString() );
        }
        v = paramsObj[ "spinbox_value" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "spinbox_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mpEmbeddedWidget->set_spinbox_value( v.toInt() );
        }
        v = paramsObj[ "checkbox_value" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "checkbox_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mbCheckBox = v.toBool();
        }
        v = paramsObj[ "display_text" ];
        if( !v.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "display_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = v.toString();

            msDisplayText = v.toString();
            mpEmbeddedWidget->set_display_text( msDisplayText );
        }
        QJsonValue qjWidth = paramsObj[ "size_width" ];
        QJsonValue qjHeight = paramsObj[ "size_height" ];
        if( !qjWidth.isUndefined() && !qjHeight.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "size_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );

            typedProp->getData().miWidth = qjWidth.toInt();
            typedProp->getData().miHeight = qjHeight.toInt();

            mSize = QSize( qjWidth.toInt(), qjHeight.toInt() );
        }

        QJsonValue qjXPos = paramsObj[ "point_x" ];
        QJsonValue qjYPos = paramsObj[ "point_y" ];
        if( !qjXPos.isUndefined() && !qjYPos.isUndefined() )
        {
            auto prop = mMapIdToProperty[ "point_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );

            typedProp->getData().miXPosition = qjXPos.toInt();
            typedProp->getData().miYPosition = qjYPos.toInt();

            mPoint = QPoint( qjXPos.toInt(), qjYPos.toInt() );
        }
    }
}

void
TemplateModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "combobox_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( value.toString() );

        mpEmbeddedWidget->set_combobox_value( value.toString() );
    }
    else if( id == "spinbox_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();

        mpEmbeddedWidget->set_spinbox_value( value.toInt() );
    }
    else if( id == "checkbox_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = value.toBool();

        mbCheckBox = value.toBool();
    }
    else if( id == "display_id" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();

        msDisplayText = value.toString();
        mpEmbeddedWidget->set_display_text( msDisplayText );
    }
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
}

void
TemplateModel::
enable_changed(bool enable)
{
    PBNodeDataModel::enable_changed( enable );
    mpEmbeddedWidget->set_active_button( enable );
    if( enable )
    {
        qDebug() << "Enable";
    }
    else
        qDebug() << "Disable";
}

void
TemplateModel::
late_constructor()
{
    qDebug() << "Automatically call this function only after creating this node by adding it into a working scene!";
}

void
TemplateModel::
em_button_clicked( int button )
{
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
}

const QString TemplateModel::_category = QString( "Template Category" );

const QString TemplateModel::_model_name = QString( "Template Model" );
