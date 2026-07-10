//Copyright © 2021 - 2026, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

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
}

QString
PushButtonModel::
portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::Out)
    {
        if (portIndex == 0)
            return "Button Trigger: Emitted when the button is clicked.";
        else if (portIndex == 1)
            return "Button Click Count: Cumulative number of times the button has been clicked.";
    }
    return PBNodeDelegateModel::portToolTip(portType, portIndex);
}
