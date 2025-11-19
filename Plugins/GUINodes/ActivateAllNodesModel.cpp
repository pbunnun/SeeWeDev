#include "ActivateAllNodesModel.hpp"
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QVariant>
#include <QApplication>
#include <MainWindow.hpp>

const QString ActivateAllNodesModel::_category = QString( "GUI" );

const QString ActivateAllNodesModel::_model_name = QString( "Activate all Nodes" );

ActivateAllNodesModel::
ActivateAllNodesModel()
    : PBNodeDelegateModel( _model_name ),
      mpEmbeddedWidget( new QPushButton( qobject_cast<QWidget *>(this) ) )
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
    
    connect( mpEmbeddedWidget, &QPushButton::clicked, this, &ActivateAllNodesModel::em_button_clicked );

    mpEmbeddedWidget->setCheckable( true );
    mpEmbeddedWidget->setText("Enable All Nodes");
    QFont font = mpEmbeddedWidget->font();
    font.setPointSize(12);
    mpEmbeddedWidget->setFont(font);
    
    // Calculate minimum size based on caption and ports
    // This ensures the widget fits properly within the node boundaries
    QSize minSize = calculateMinimumWidgetSize(_model_name, 0, 2);
    mpEmbeddedWidget->setMinimumSize(minSize);

    QString propId = "label";
    auto propLabel = std::make_shared< TypedProperty< QString > > ("Label", propId, QMetaType::QString, "Enable All Nodes" );
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
}

QJsonObject
ActivateAllNodesModel::
save() const
{
    /*
     * If save() was overrided, PBNodeDelegateModel::save() must be called explicitely.
     */
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "label" ] = mpEmbeddedWidget->text();
    cParams[ "fontsize" ] = mpEmbeddedWidget->font().pointSize();
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

NodeDataType
ActivateAllNodesModel::
dataType(PortType, PortIndex) const
{
    return NodeDataType();
}

void
ActivateAllNodesModel::
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
    }
    mpEmbeddedWidget->setEnabled( isEnable() );
}

void
ActivateAllNodesModel::
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
}

void
ActivateAllNodesModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed( enable );

    mpEmbeddedWidget->setEnabled(enable);
}

void
ActivateAllNodesModel::
em_button_clicked(bool check)
{
    if( !isSelected() )
    {
        Q_EMIT selection_request_signal();
        mpEmbeddedWidget->setChecked(!check);
        return;
    }

    auto topWidgets = QApplication::topLevelWidgets();
    for( QWidget * widget : topWidgets )
    {
        MainWindow * mainW = qobject_cast<MainWindow*>( widget );
        if( mainW )
        {
            mainW->enable_all_nodes( check );
            break;
        }
    }
    if( !check )
        this->setEnable(true);
}


