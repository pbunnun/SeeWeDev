#include "DisplayTextModel.hpp"
#include <QDebug>
#include <QEvent>
#include <QVariant>
#include <QTextDocument>
#include <QJsonArray>
#include "qtvariantproperty_p.h"

const QString DisplayTextModel::_category = QString( "GUI" );

const QString DisplayTextModel::_model_name = QString( "Display Text" );

DisplayTextModel::
DisplayTextModel()
    : PBNodeDelegateModel( _model_name ),
      mpEmbeddedWidget( new QTextEdit( qobject_cast<QWidget *>(this) ) ),
    _minPixmap(":/DisplayTextModel.png")
{
    mpInformationData = std::make_shared< InformationData >();

    // Configure widget appearance
    mpEmbeddedWidget->setPlainText(msText);
    
    // Set size policy to allow the widget to shrink and expand in both directions
    mpEmbeddedWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    
    // Set a small minimum size to allow the widget to shrink
    // The node framework will handle the actual sizing
    mpEmbeddedWidget->setMinimumSize(QSize(50, 30));

    // Install event filter to handle focus events
    mpEmbeddedWidget->installEventFilter(this);

    // Property browser setup
    QString propId = "font";
    auto propFont = std::make_shared< TypedProperty< QString > >("Font", propId, QMetaType::QString, msFontFamily);
    mvProperty.push_back( propFont );
    mMapIdToProperty[ propId ] = propFont;

    IntPropertyType intPropertyType;
    intPropertyType.miMax = 72;
    intPropertyType.miMin = 8;
    intPropertyType.miValue = miFontSize;
    propId = "size";
    auto propSize = std::make_shared< TypedProperty< IntPropertyType > >("Size", propId, QMetaType::Int, intPropertyType);
    mvProperty.push_back( propSize );
    mMapIdToProperty[ propId ] = propSize;

    // Text color properties (R, G, B)
    std::vector<std::string> colorNames = {"Red", "Green", "Blue"};
    UcharPropertyType ucharPropertyType;
    for(int i = 0; i < 3; i++)
    {
        ucharPropertyType.mucValue = mucTextColor[i];
        propId = QString("text_color_%1").arg(i);
        auto propTextColor = std::make_shared< TypedProperty< UcharPropertyType > >(
            QString("%1").arg(QString::fromStdString(colorNames[i])), 
            propId, 
            QMetaType::Int, 
            ucharPropertyType,
            "Text Color"
        );
        mvProperty.push_back( propTextColor );
        mMapIdToProperty[ propId ] = propTextColor;
    }

    // Background color properties (R, G, B)
    for(int i = 0; i < 3; i++)
    {
        ucharPropertyType.mucValue = mucBackgroundColor[i];
        propId = QString("bg_color_%1").arg(i);
        auto propBgColor = std::make_shared< TypedProperty< UcharPropertyType > >(
            QString("%1").arg(QString::fromStdString(colorNames[i])), 
            propId, 
            QMetaType::Int, 
            ucharPropertyType,
            "Background Color"
        );
        mvProperty.push_back( propBgColor );
        mMapIdToProperty[ propId ] = propBgColor;
    }

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = QStringList({"Left", "Center", "Right", "Justify"});
    enumPropertyType.miCurrentIndex = miAlignment;
    propId = "alignment";
    auto propAlignment = std::make_shared< TypedProperty< EnumPropertyType > >("Alignment", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propAlignment );
    mMapIdToProperty[ propId ] = propAlignment;

    // Number of input ports property
    intPropertyType.miMax = 10;
    intPropertyType.miMin = 0;
    intPropertyType.miValue = miNumInputPorts;
    propId = "num_input_ports";
    auto propNumInputPorts = std::make_shared< TypedProperty< IntPropertyType > >("Number of Input Ports", propId, QMetaType::Int, intPropertyType);
    mvProperty.push_back( propNumInputPorts );
    mMapIdToProperty[ propId ] = propNumInputPorts;

    // Initialize input data vector
    mvInputData.resize(miNumInputPorts);
    for(auto& data : mvInputData) {
        data = std::make_shared<InformationData>();
    }

    // Connect text changes
    connect(mpEmbeddedWidget, &QTextEdit::textChanged, this, &DisplayTextModel::text_changed);
    
    // Apply initial styling
    apply_styling();
}

unsigned int
DisplayTextModel::
nPorts( PortType portType ) const
{
    if(portType == PortType::In)
        return miNumInputPorts;
    else
        return 1;  // One output
}

NodeDataType
DisplayTextModel::
dataType( PortType, PortIndex ) const
{
    return InformationData().type();
}

QJsonObject
DisplayTextModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "text" ] = msText;
    cParams[ "font" ] = msFontFamily;
    cParams[ "size" ] = miFontSize;
    
    // Save text color as RGB array
    QJsonArray textColorArray;
    for(int i = 0; i < 3; i++)
        textColorArray.append(mucTextColor[i]);
    cParams[ "text_color" ] = textColorArray;
    
    // Save background color as RGB array
    QJsonArray bgColorArray;
    for(int i = 0; i < 3; i++)
        bgColorArray.append(mucBackgroundColor[i]);
    cParams[ "background_color" ] = bgColorArray;
    
    cParams[ "alignment" ] = miAlignment;
    cParams[ "num_input_ports" ] = static_cast<int>(miNumInputPorts);
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void
DisplayTextModel::
load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "text" ];
        if( !v.isNull() )
            msText = v.toString();

        v = paramsObj[ "font" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "font" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = v.toString();
            msFontFamily = v.toString();
        }

        v = paramsObj[ "size" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();
            miFontSize = v.toInt();
        }

        // Load text color from RGB array
        v = paramsObj[ "text_color" ];
        if( !v.isNull() && v.isArray() )
        {
            QJsonArray colorArray = v.toArray();
            for(int i = 0; i < 3 && i < colorArray.size(); i++)
            {
                auto propId = QString("text_color_%1").arg(i);
                auto prop = mMapIdToProperty[ propId ];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = colorArray[i].toInt();
                mucTextColor[i] = colorArray[i].toInt();
            }
        }

        // Load background color from RGB array
        v = paramsObj[ "background_color" ];
        if( !v.isNull() && v.isArray() )
        {
            QJsonArray colorArray = v.toArray();
            for(int i = 0; i < 3 && i < colorArray.size(); i++)
            {
                auto propId = QString("bg_color_%1").arg(i);
                auto prop = mMapIdToProperty[ propId ];
                auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
                typedProp->getData().mucValue = colorArray[i].toInt();
                mucBackgroundColor[i] = colorArray[i].toInt();
            }
        }

        v = paramsObj[ "alignment" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "alignment" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();
            miAlignment = v.toInt();
        }

        v = paramsObj[ "num_input_ports" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "num_input_ports" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
            typedProp->getData().miValue = v.toInt();
            miNumInputPorts = v.toInt();
            
            // Resize the input data vector to match loaded port count
            mvInputData.resize(miNumInputPorts);
            for(auto& data : mvInputData) {
                if(!data) {
                    data = std::make_shared<InformationData>();
                }
            }
        }
    }
    
    // Set the text content after loading all properties
    if(!msText.isEmpty())
    {
        mpEmbeddedWidget->setPlainText(msText);
        
        // Apply styling (including alignment) after setting text
        apply_styling();
    }
}

void
DisplayTextModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "font" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        typedProp->getData() = value.toString();
        msFontFamily = value.toString();
        apply_styling();
    }
    else if( id == "size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();
        miFontSize = value.toInt();
        apply_styling();
    }
    else if( id.startsWith("text_color_") )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
        typedProp->getData().mucValue = value.toInt();
        
        // Extract index from property ID (text_color_0, text_color_1, text_color_2)
        int colorIndex = id.right(1).toInt();
        mucTextColor[colorIndex] = value.toInt();
        apply_styling();
    }
    else if( id.startsWith("bg_color_") )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< UcharPropertyType > >( prop );
        typedProp->getData().mucValue = value.toInt();
        
        // Extract index from property ID (bg_color_0, bg_color_1, bg_color_2)
        int colorIndex = id.right(1).toInt();
        mucBackgroundColor[colorIndex] = value.toInt();
        apply_styling();
    }
    else if( id == "alignment" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = value.toInt();
        miAlignment = value.toInt();
        apply_styling();
    }
    else if( id == "num_input_ports" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        typedProp->getData().miValue = value.toInt();
        
        unsigned int newNumPorts = value.toInt();
        if(newNumPorts != miNumInputPorts)
        {
            miNumInputPorts = newNumPorts;
            
            // Resize the input data vector
            mvInputData.resize(miNumInputPorts);
            for(auto& data : mvInputData) {
                if(!data) {
                    data = std::make_shared<InformationData>();
                }
            }
            
            // Notify the graph that the node geometry needs to be updated
            Q_EMIT embeddedWidgetSizeUpdated();
        }
    }
}

void
DisplayTextModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex portIndex )
{
    if( !isEnable() )
        return;
    
    if(portIndex < miNumInputPorts)
    {
        auto d = std::dynamic_pointer_cast< InformationData >( nodeData );
        if( d )
        {
            // Store data for this specific port
            if(portIndex < mvInputData.size()) {
                mvInputData[portIndex] = d;
            }
            
            // For backward compatibility, also store in the single data member
            // (use the first port's data)
            if(portIndex == 0) {
                mpInformationData = d;
            }
            
            Q_EMIT dataUpdated(0);
        }
    }
}

std::shared_ptr<NodeData>
DisplayTextModel::
outData(PortIndex port)
{
    if( isEnable() && port == 0 )
        return mpInformationData;
    
    return nullptr;
}

void
DisplayTextModel::
text_changed()
{
    msText = mpEmbeddedWidget->toPlainText();
}

void
DisplayTextModel::
enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed( enable );
    mpEmbeddedWidget->setEnabled(enable);
}

bool
DisplayTextModel::
eventFilter(QObject *obj, QEvent *event)
{
    // Handle focus events for the text edit widget
    if (obj == mpEmbeddedWidget)
    {
        if (event->type() == QEvent::FocusIn)
        {
            // When text edit gets focus, notify that an editable widget is selected
            // This prevents node deletion when Delete/Backspace is pressed
            editable_embedded_widget_selected_changed(true);
        }
        else if (event->type() == QEvent::FocusOut)
        {
            // When text edit loses focus, allow normal node deletion
            editable_embedded_widget_selected_changed(false);
        }
    }
    
    return PBNodeDelegateModel::eventFilter(obj, event);
}

void
DisplayTextModel::
apply_styling()
{
    // Set font
    QFont font(msFontFamily, miFontSize);
    mpEmbeddedWidget->setFont(font);
    
    // Convert RGB arrays to QColor
    QColor textColor(mucTextColor[0], mucTextColor[1], mucTextColor[2]);
    QColor bgColor(mucBackgroundColor[0], mucBackgroundColor[1], mucBackgroundColor[2]);
    
    // Set text color and background color
    QString colorStyle = QString("QTextEdit { color: %1; background-color: %2; }")
                            .arg(textColor.name())
                            .arg(bgColor.name());
    mpEmbeddedWidget->setStyleSheet(colorStyle);
    
    // Set alignment
    Qt::Alignment align;
    switch(miAlignment)
    {
        case 0: align = Qt::AlignLeft; break;
        case 1: align = Qt::AlignCenter; break;
        case 2: align = Qt::AlignRight; break;
        case 3: align = Qt::AlignJustify; break;
        default: align = Qt::AlignLeft; break;
    }
    
    // Apply alignment to all text in the document using a document-local cursor
    // so we don't change the widget's selection/caret.
    QTextCursor docCursor(mpEmbeddedWidget->document());
    docCursor.select(QTextCursor::Document);
    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(align);
    docCursor.mergeBlockFormat(blockFormat);

    // Ensure the widget's visible cursor has no selection after styling
    QTextCursor visibleCursor = mpEmbeddedWidget->textCursor();
    visibleCursor.clearSelection();
    mpEmbeddedWidget->setTextCursor(visibleCursor);
}
