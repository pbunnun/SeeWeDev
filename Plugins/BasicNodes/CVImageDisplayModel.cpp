#include "CVImageDisplayModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

CVImageDisplayModel::
CVImageDisplayModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget( new PBImageDisplayWidget(qobject_cast<QWidget *>(this)) )
{
    mpEmbeddedWidget->installEventFilter( this );
    mpEmbeddedWidget->resize(120, 90);
    mpSyncData = std::make_shared<SyncData>();

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = 0;
    sizePropertyType.miHeight = 0;
    auto propId = "image_size";
    auto propImageSize = std::make_shared< TypedProperty< SizePropertyType > >( "Size", propId, QVariant::Size, sizePropertyType, "", true );
    mvProperty.push_back( propImageSize );
    mMapIdToProperty[ propId ] = propImageSize;

    propId = "image_format";
    auto propFormat = std::make_shared< TypedProperty< QString > >( "Format", propId, QVariant::String, "", "", true );
    mvProperty.push_back( propFormat );
    mMapIdToProperty[ propId ] = propFormat;
}

unsigned int
CVImageDisplayModel::
nPorts(PortType portType) const
{
    if( portType == PortType::In )
        return 1;
    else if( portType == PortType::Out)
        return 1;
    else
        return 0;
}

bool
CVImageDisplayModel::
eventFilter(QObject *object, QEvent *event)
{
    if( object == mpEmbeddedWidget )
    {
        if( event->type() == QEvent::Resize )
            display_image();
    }
    return false;
}


NodeDataType
CVImageDisplayModel::
dataType( PortType portType, PortIndex ) const
{
    if(portType == PortType::In)
        return CVImageData().type();
    else
        return SyncData().type();
}

std::shared_ptr<NodeData>
CVImageDisplayModel::
outData(PortIndex)
{
    return mpSyncData;
}

void
CVImageDisplayModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;

    if (nodeData)
    {
        mpSyncData->state() = false;
        Q_EMIT dataUpdated(0);
        mpNodeData = nodeData;
        display_image();
        mpSyncData->state() = true;
        Q_EMIT dataUpdated(0);
    }
}

void
CVImageDisplayModel::
display_image()
{
    auto d = std::dynamic_pointer_cast< CVImageData >( mpNodeData );
    if ( d )
    {
        mpEmbeddedWidget->Display(d->image());
        auto prop = mMapIdToProperty["image_size"];
        auto typedPropSize = std::static_pointer_cast<TypedProperty<SizePropertyType>>( prop );
        typedPropSize->getData().miWidth = d->image().cols;
        typedPropSize->getData().miHeight = d->image().rows;
        Q_EMIT property_changed_signal( prop );

        prop = mMapIdToProperty[ "image_format" ];
        auto typedPropFormat = std::static_pointer_cast<TypedProperty<QString>>( prop );
        if( d->image().channels() == 1 )
            typedPropFormat->getData() = "CV_8UC1";
        else
            typedPropFormat->getData() = "CV_8UC3";
        Q_EMIT property_changed_signal( prop );
    }
}

const QString CVImageDisplayModel::_category = QString( "Output" );

const QString CVImageDisplayModel::_model_name = QString( "CV Image Display" );
