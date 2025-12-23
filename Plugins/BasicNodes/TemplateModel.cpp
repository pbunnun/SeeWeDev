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

#include "TemplateModel.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>
#include "qtvariantproperty_p.h"

const QString TemplateModel::_category = QString( "Template Category" );

const QString TemplateModel::_model_name = QString( "Template Model" );

TemplateModel::
TemplateModel()
    : PBNodeDelegateModel( _model_name ),
      // PBNodeDataModel( model's name, is it enable at start? )
      mpEmbeddedWidget( new TemplateEmbeddedWidget( qobject_cast<QWidget *>(this) ) ),
    _minPixmap(":/Template Model.png")
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &TemplateEmbeddedWidget::button_clicked_signal, this, &TemplateModel::em_button_clicked );
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
    mpCVImageData->data().create(320,240, CV_8UC3);

    mpStdVectorIntData = std::make_shared< StdVectorIntData >();
    auto & raw_data = mpStdVectorIntData->data();
    for( int idx = 0; idx < 10; ++idx )
        raw_data.push_back(idx);

    mpInformationData = std::make_shared< InformationData >();
    QString information = "{\n    \"register_type\" : 2, \n";
    information += "    \"start_address\" : 0, \n";
    information += "    \"number_of_entries\" : 4, \n";
    information += "    \"operation_mode\" : 1, \n";
    information += "    \"value0\" : 0, \n";
    information += "    \"value1\" : 1, \n";
    information += "    \"value2\" : 0, \n";
    information += "    \"value3\" : 1 \n";
    information += "}";
    mpInformationData->set_information( information );

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
    auto propSpinBox = std::make_shared< TypedProperty< IntPropertyType > >("SpinBox", propId, QMetaType::Int, intPropertyType, "SubProp0" );
    mvProperty.push_back( propSpinBox );
    mMapIdToProperty[ propId ] = propSpinBox;

    propId = "checkbox_id";
    auto propCheckBox = std::make_shared< TypedProperty< bool > >("CheckBox", propId, QMetaType::Bool, mbCheckBox, "SubProp1" );
    mvProperty.push_back( propCheckBox );
    mMapIdToProperty[ propId ] = propCheckBox;

    propId = "display_id";
    auto propDisplayText = std::make_shared< TypedProperty< QString > >("Text", propId, QMetaType::QString, msDisplayText, "SubProp1" );
    mvProperty.push_back( propDisplayText );
    mMapIdToProperty[ propId ] = propDisplayText;

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = mSize.width();
    sizePropertyType.miHeight = mSize.height();
    propId = "size_id";
    auto propSize = std::make_shared< TypedProperty< SizePropertyType > >("Size", propId, QMetaType::QSize, sizePropertyType );
    mvProperty.push_back( propSize );
    mMapIdToProperty[ propId ] = propSize;

    PointPropertyType pointPropertyType;
    pointPropertyType.miXPosition = mPoint.x();
    pointPropertyType.miYPosition = mPoint.y();
    propId = "point_id";
    auto propPoint = std::make_shared< TypedProperty< PointPropertyType > >("Point", propId, QMetaType::QPoint, pointPropertyType );
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
        return( 2 );
    case PortType::Out:
        return( 3 );
    default:
        return( 0 );
    }
}

NodeDataType
TemplateModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::Out )
    {
        if( portIndex == 0 )
            return CVImageData().type();
        else if( portIndex == 1 )
            return StdVectorIntData().type();
        else if( portIndex == 2 )
            return InformationData().type();
        else
            return NodeDataType();
    }
    else if( portType == PortType::In )
    {
        if( portIndex == 0 )
            return CVImageData().type();
        else if( portIndex == 1 )
            return StdVectorIntData().type();
        else
            return NodeDataType();
    }
    else
        return NodeDataType();
}

std::shared_ptr<NodeData>
TemplateModel::
outData(PortIndex index)
{
    if( isEnable() )
    {
        if( index ==  0 )
        {
            if( mpCVImageData->data().data != nullptr )
                return mpCVImageData;
            else
                return nullptr;
        }
        else if( index == 1 )
        {
            return mpStdVectorIntData;
        }
        else if( index == 2 )
        {
            return mpInformationData;
        }
    }
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
            mpCVImageData->set_image( d->data() );
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
     * If save() was overrided, PBNodeDelegateModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDelegateModel::save();

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
        QJsonValue v = paramsObj[ "combobox_text" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "combobox_id" ];
            /* Restore internal property */
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( v.toString() );
            /* Restore mpEmbeddedWidget */
            mpEmbeddedWidget->set_combobox_value( v.toString() );
        }
        v = paramsObj[ "spinbox_value" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "spinbox_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();

            mpEmbeddedWidget->set_spinbox_value( v.toInt() );
        }
        v = paramsObj[ "checkbox_value" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "checkbox_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mbCheckBox = v.toBool();
        }
        v = paramsObj[ "display_text" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "display_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = v.toString();

            msDisplayText = v.toString();
            mpEmbeddedWidget->set_display_text( msDisplayText );
        }
        QJsonValue qjWidth = paramsObj[ "size_width" ];
        QJsonValue qjHeight = paramsObj[ "size_height" ];
        if( !qjWidth.isNull() && !qjHeight.isNull() )
        {
            auto prop = mMapIdToProperty[ "size_id" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );

            typedProp->getData().miWidth = qjWidth.toInt();
            typedProp->getData().miHeight = qjHeight.toInt();

            mSize = QSize( qjWidth.toInt(), qjHeight.toInt() );
        }

        QJsonValue qjXPos = paramsObj[ "point_x" ];
        QJsonValue qjYPos = paramsObj[ "point_y" ];
        if( !qjXPos.isNull() && !qjYPos.isNull() )
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
    PBNodeDelegateModel::setModelProperty( id, value );

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
    PBNodeDelegateModel::enable_changed( enable );
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
    DEBUG_LOG_INFO() << "[em_button_clicked] button:" << button << "isSelected:" << isSelected();
    
    // If node is not selected, select it first and block the interaction
    // User needs to click again when node is selected to perform the action
    if (!isSelected())
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Node not selected, requesting selection";
        Q_EMIT selection_request_signal();
        return;
    }
    
    if( button == 0 ) //Start
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Start button - enabling node";
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
        DEBUG_LOG_INFO() << "[em_button_clicked] Stop button - disabling node";
        auto prop = mMapIdToProperty[ "enable" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        typedProp->getData() = false;
        Q_EMIT property_changed_signal( prop );

        enable_changed( false );
    }
    else if( button == 2 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Button 2 - update spinbox value";
        auto prop = mMapIdToProperty[ "spinbox_id" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = mpEmbeddedWidget->get_spinbox()->value();
        Q_EMIT property_changed_signal( prop );
    }
    else if( button == 3 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Button 3 - update combobox value";
        auto prop = mMapIdToProperty[ "combobox_id" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( mpEmbeddedWidget->get_combobox_text() );
        Q_EMIT property_changed_signal( prop );
    }
    else if( button == 4 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Button 4 - update all output ports";
        updateAllOutputPorts();
    }
    // Notify node's NodeGraphicsObject to redraw itself.
    Q_EMIT embeddedWidgetSizeUpdated();
}


