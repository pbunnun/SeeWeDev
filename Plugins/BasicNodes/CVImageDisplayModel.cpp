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

#include "CVImageDisplayModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>

#include <QtWidgets/QFileDialog>


#include "CVImageData.hpp"

const QString CVImageDisplayModel::_category = QString( "Output" );

const QString CVImageDisplayModel::_model_name = QString( "CV Image Display" );

CVImageDisplayModel::
CVImageDisplayModel()
    : PBNodeDelegateModel( _model_name ),
      mpEmbeddedWidget( new PBImageDisplayWidget(qobject_cast<QWidget *>(this)) ),
    _minPixmap(":/Image Display.png")
{
    mpEmbeddedWidget->installEventFilter( this );
    mpEmbeddedWidget->resize(640, 480);
    mpSyncData = std::make_shared<SyncData>( true );

    // Make the minimize property read-only for display nodes
    auto minimizePropId = QString("minimize");
    auto minimizeProp = std::make_shared< TypedProperty< bool > >( "Minimize", minimizePropId, QMetaType::Bool, false, "Common", true );
    // Find and replace the minimize property in the vector
    for (size_t i = 0; i < mvProperty.size(); ++i) {
        if (mvProperty[i]->getID() == minimizePropId) {
            mvProperty[i] = minimizeProp;
            break;
        }
    }
    mMapIdToProperty[ minimizePropId ] = minimizeProp;

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = 0;
    sizePropertyType.miHeight = 0;
    auto propId = "image_size";
    auto propImageSize = std::make_shared< TypedProperty< SizePropertyType > >( "Size", propId, QMetaType::QSize, sizePropertyType, "", true );
    mvProperty.push_back( propImageSize );
    mMapIdToProperty[ propId ] = propImageSize;

    propId = "image_format";
    auto propFormat = std::make_shared< TypedProperty< QString > >( "Format", propId, QMetaType::QString, "", "", true );
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
    else if(portType == PortType::Out)
        return SyncData().type();
    return NodeDataType();
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
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
        {
            const cv::Mat& frame = d->data();
            if( frame.data != nullptr )
            {
                mpSyncData->data() = false;
                // Consumer: read-only access to the pooled or owned frame
                // Copy only when displaying to avoid holding the pool slot
                frame.copyTo( mCVImageDisplay );
                display_image();
                mpSyncData->data() = true;
                Q_EMIT dataUpdated(0);
            }
        }
    }
}

void
CVImageDisplayModel::
display_image()
{
    // Don't try to display when node is minimized - the widget may not be visible
    // Also check if the widget itself is valid and visible
    if (isMinimize() || !mpEmbeddedWidget || !mpEmbeddedWidget->isVisible())
        return;
    
    mpEmbeddedWidget->Display( mCVImageDisplay );

    if( mCVImageDisplay.cols != miImageWidth || mCVImageDisplay.rows != miImageHeight )
    {
        miImageWidth = mCVImageDisplay.cols;
        miImageHeight = mCVImageDisplay.rows;

        // Update the widget size to match the new image aspect ratio
        if (mpEmbeddedWidget && miImageWidth > 0 && miImageHeight > 0)
        {
            // Calculate new height based on current width and new aspect ratio
            int currentWidth = mpEmbeddedWidget->width();
            double aspectRatio = static_cast<double>(miImageHeight) / static_cast<double>(miImageWidth);
            int newHeight = static_cast<int>(currentWidth * aspectRatio);
            
            // Resize the widget to match the aspect ratio
            mpEmbeddedWidget->resize(currentWidth, newHeight);
        }

        auto prop = mMapIdToProperty["image_size"];
        auto typedPropSize = std::static_pointer_cast<TypedProperty<SizePropertyType>>( prop );
        typedPropSize->getData().miWidth = miImageWidth;
        typedPropSize->getData().miHeight = miImageHeight;
        Q_EMIT property_changed_signal( prop );
    }

    if( mCVImageDisplay.channels() != miImageFormat )
    {
        miImageFormat = mCVImageDisplay.channels();

        auto prop = mMapIdToProperty[ "image_format" ];
        auto typedPropFormat = std::static_pointer_cast<TypedProperty<QString>>( prop );
        if( mCVImageDisplay.channels() == 1 )
            typedPropFormat->getData() = "CV_8UC1";
        else
            typedPropFormat->getData() = "CV_8UC3";
        Q_EMIT property_changed_signal( prop );
    }
}


