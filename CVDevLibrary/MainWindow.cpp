//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include <QPluginLoader>
#include <QDir>
#include <QFileDialog>
#include <QTabWidget>
#include <QChar>
#include "MainWindow.hpp"
#include <nodes/Node>
#include "PluginInterface.hpp"
#include "PBFlowView.hpp"
#include "ui_MainWindow.h"
#include <QGraphicsScene>
#include <QVector>
#include "qtpropertybrowser.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QDate>

using QtNodes::DataModelRegistry;

MainWindow::MainWindow( QWidget *parent )
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
{
    ui->setupUi( this );

    QDate check_day(2025, 1, 1);
    QDate current = QDate::currentDate();
    int no_days = check_day.daysTo(current);
    if( no_days >= 365 )
        QMessageBox::warning(this, msProgramName, "<p>This version is too old. There might be a newer version with some bugs fixed and improvements. "
                                                 "Please contact <a href=mailto:pished.bunnun@nectec.or.th>pished.bunnun@nectec.or.th</a> to get a new version.</p>");

    mpDataModelRegistry = std::make_shared<DataModelRegistry>();
    add_type_converters( mpDataModelRegistry );
    load_plugins( mpDataModelRegistry, mPluginsList );

    createScene( "", mpDataModelRegistry );

    QStringList headers = { "Caption", "ID" };
    ui->mpNodeListTreeView->setHeaderLabels( headers );

    ui->mpMenuView->addAction( ui->mpAvailableNodeCategoryDockWidget->toggleViewAction() );
    ui->mpMenuView->addAction( ui->mpNodeListDockWidget->toggleViewAction() );
    ui->mpMenuView->addAction( ui->mpPropertyBrowserDockWidget->toggleViewAction() );

    setupPropertyBrowserDockingWidget();
    setupNodeCategoriesDockingWidget();
    setupNodeListDockingWidget();

    connect( ui->mpNodeListTreeView, &QTreeWidget::itemClicked, this, &MainWindow::nodeListClicked );
    connect( ui->mpNodeListTreeView, &QTreeWidget::itemDoubleClicked, this, &MainWindow::nodeListDoubleClicked );
    connect( ui->mpTabWidget, &QTabWidget::currentChanged, this, &MainWindow::tabPageChanged );
    connect( ui->mpTabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeScene );

    setWindowTitle( msProgramName );

    showMaximized();

    loadSettings();
}

MainWindow::
~MainWindow()
{
    while( !mlSceneProperty.empty() )
    {
        struct SceneProperty sceneProperty = mlSceneProperty.back();
        delete sceneProperty.pFlowScene;
        delete sceneProperty.pFlowView;
        mlSceneProperty.pop_back();
    }

    delete mpVariantManager;
    delete mpPropertyEditor;

    QMap< QString, QTreeWidgetItem * >::ConstIterator it = mMapModelCategoryToNodeTreeWidgetItem.constBegin();
    while( it != mMapModelCategoryToNodeTreeWidgetItem.constEnd() )
    {
        delete it.value();
        it++;
    }

    while( !mGroupPropertyManagerList.isEmpty() )
    {
        delete mGroupPropertyManagerList.last();
        mGroupPropertyManagerList.removeLast();
    }

    while( !mPluginsList.isEmpty() )
    {
        mPluginsList.last()->unload();
        delete mPluginsList.last();
        mPluginsList.removeLast();
    }

    delete ui;
}

void
MainWindow::
nodeInSceneSelectionChanged()
{
    auto nodes = mpFlowScene->selectedNodes();
    if( nodes.size() == 1 )
    {
        clearPropertyBrowser();

        mpSelectedNode = nodes[ 0 ];
        mpSelectedNodeDataModel = static_cast< PBNodeDataModel * >( mpSelectedNode->nodeDataModel() );
        mpSelectedNodeDataModel->setSelected( true ); // TODO: This should not be called explicitly. It could have done in NodeGraphicsObject class.
        connect( mpSelectedNodeDataModel, SIGNAL(property_changed_signal(std::shared_ptr<Property>)),
                this, SLOT(nodePropertyChanged(std::shared_ptr<Property>)) );
        connect( mpSelectedNodeDataModel, SIGNAL( property_structure_changed_signal() ),
                this, SLOT( nodeInSceneSelectionChanged() ) );

        auto propertyVector = mpSelectedNodeDataModel->getProperty();

        auto nodeID = mpSelectedNode->id().toString();
        auto nodeTreeWidgetItem = mMapNodeIDToNodeTreeWidgetItem[ nodeID ];
        ui->mpNodeListTreeView->clearSelection();
        nodeTreeWidgetItem->setSelected( true );

        QtVariantProperty *property;
        property = mpVariantManager->addProperty( QMetaType::QString, "Node ID" );
        property->setAttribute( "readOnly", true);
        property->setValue( nodeID );
        addProperty( property, "id", "Common" );

        property = mpVariantManager->addProperty( QMetaType::Bool, "Source" );
        property->setAttribute( "readOnly", true );
        property->setAttribute( QLatin1String( "textVisible" ), false );
        property->setValue( mpSelectedNodeDataModel->isSource() );
        addProperty( property, "source", "Common" );

        auto it = propertyVector.begin();
        while( it != propertyVector.end() )
        {
            auto type = ( *it )->getType();
            if( type == QMetaType::QString )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( typedProp->getData() );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QMetaType::Int )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                IntPropertyType intPropType = typedProp->getData();
                property->setAttribute( QLatin1String( "minimum" ), intPropType.miMin );
                property->setAttribute( QLatin1String( "maximum" ), intPropType.miMax );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( intPropType.miValue );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QMetaType::Double )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                DoublePropertyType doublePropType = typedProp->getData();
                property->setAttribute( QLatin1String( "minimum" ), doublePropType.mdMin );
                property->setAttribute( QLatin1String( "maximum" ), doublePropType.mdMax );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( doublePropType.mdValue );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QtVariantPropertyManager::enumTypeId() )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( QLatin1String( "enumNames" ), typedProp->getData().mslEnumNames );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( typedProp->getData().miCurrentIndex );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QMetaType::Bool )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< bool >>( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( QLatin1String( "textVisible" ), false );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( typedProp->getData() );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QtVariantPropertyManager::filePathTypeId() )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( QLatin1String( "filter" ), typedProp->getData().msFilter );
                property->setAttribute( QLatin1String( "mode" ), typedProp->getData().msMode );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( typedProp->getData().msFilename );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QtVariantPropertyManager::pathTypeId() )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( typedProp->getData().msPath );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QMetaType::QSize )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( QSize( typedProp->getData().miWidth, typedProp->getData().miHeight ) );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QMetaType::QSizeF )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< SizeFPropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( QSizeF( typedProp->getData().mfWidth, typedProp->getData().mfHeight ) );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QMetaType::QRect )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< RectPropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( QLatin1String( "constraint" ), QRect(0, 0, INT_MAX, INT_MAX) );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( QRect( typedProp->getData().miXPosition, typedProp->getData().miYPosition, typedProp->getData().miWidth, typedProp->getData().miHeight ) );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QMetaType::QPoint )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( QPoint( typedProp->getData().miXPosition, typedProp->getData().miYPosition ) );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            else if( type == QMetaType::QPointF )
            {
                auto typedProp = std::static_pointer_cast< TypedProperty< PointFPropertyType > >( *it );
                property = mpVariantManager->addProperty( type, typedProp->getName() );
                property->setAttribute( "readOnly", typedProp->isReadOnly());
                property->setValue( QPointF( typedProp->getData().mfXPosition, typedProp->getData().mfYPosition ) );
                addProperty( property, typedProp->getID(), typedProp->getSubPropertyText() );
            }
            it++;
        }
    }
    else
    {
        if( mpSelectedNodeDataModel != nullptr )
        {
            clearPropertyBrowser();
            mpSelectedNodeDataModel->setSelected( false );// TODO: This should not be called explicitly. It could have done in NodeGraphicsObject class.
            disconnect(mpSelectedNodeDataModel, nullptr, this, nullptr);
            mpSelectedNodeDataModel = nullptr;
            mpSelectedNode = nullptr;

            ui->mpNodeListTreeView->clearSelection();
        }
    }
}

void
MainWindow::
setupPropertyBrowserDockingWidget()
{
    mpVariantManager = new QtVariantPropertyManager(this);
    connect(mpVariantManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
            this, SLOT(editorPropertyChanged(QtProperty *, const QVariant &)));

    QtVariantEditorFactory *variantFactory = new QtVariantEditorFactory(this);
    mpPropertyEditor = new QtTreePropertyBrowser(ui->mpPropertyBrowserDockWidget);
    mpPropertyEditor->setResizeMode(QtTreePropertyBrowser::Interactive);
    mpPropertyEditor->setFactoryForManager(mpVariantManager, variantFactory);
    ui->mpPropertyBrowserDockWidget->setWidget(mpPropertyEditor);

    mMapPropertyIdToExpanded["Common"] = false;
}

void
MainWindow::
setupNodeCategoriesDockingWidget()
{
    // Add filterbox to the menu.
    ui->mpAvailableNodeCategoryFilterLineEdit->setPlaceholderText( QStringLiteral( "Filter" ) );
    ui->mpAvailableNodeCategoryFilterLineEdit->setClearButtonEnabled( true );
    //ui->mpAvailableNodeCategoryTreeView->expandAll(); // TODO: add a button to expandAll and collapseAll.
    //Setup filtering
    connect( ui->mpAvailableNodeCategoryFilterLineEdit, &QLineEdit::textChanged, [&]( const QString &text )
    {
        for( auto& item : mMapModelCategoryToNodeTreeWidgetItem )
        {
            bool shouldHideCategory = true;
            for (int i = 0; i < item->childCount(); ++i)
            {
                auto child = item->child( i );
                auto modelName = child->data( 0, Qt::UserRole ).toString();
                const bool match = ( modelName.contains( text, Qt::CaseInsensitive ) );
                if( match )
                    shouldHideCategory = false;
                child->setHidden( !match );
            }
            item->setHidden( shouldHideCategory );
        }
    });

    updateNodeCategoriesDockingWidget();
}

void
MainWindow::
updateNodeCategoriesDockingWidget()
{
    auto skipText = QStringLiteral( "skip me" );
    // Add models to the view.
    mMapModelCategoryToNodeTreeWidgetItem.clear();
    ui->mpAvailableNodeCategoryTreeView->clear();
    for ( auto const &cat : mpDataModelRegistry->categories() )
    {
        auto item = new QTreeWidgetItem( ui->mpAvailableNodeCategoryTreeView );
        item->setText( 0, cat );
        item->setData( 0, Qt::UserRole, skipText );
        mMapModelCategoryToNodeTreeWidgetItem[ cat ] = item;
    }

    for ( auto const &assoc : mpDataModelRegistry->registeredModelsCategoryAssociation() )
    {
        auto parent = mMapModelCategoryToNodeTreeWidgetItem[assoc.second];
        auto item = new QTreeWidgetItem( parent );
        item->setText( 0, assoc.first );
        item->setData( 0, Qt::UserRole, assoc.first );

        auto type = mpDataModelRegistry->create( assoc.first );
        item->setIcon( 0, QIcon( type->minPixmap() ) );
    }
}

void
MainWindow::
setupNodeListDockingWidget()
{
    auto skipText = QStringLiteral( "skip me" );
    // Add filterbox to the menu.
    ui->mpNodeListFilterLineEdit->setPlaceholderText( QStringLiteral( "Filter" ) );
    ui->mpNodeListFilterLineEdit->setClearButtonEnabled( true );

    ui->mpNodeListTreeView->expandAll();
    connect( ui->mpNodeListFilterLineEdit, &QLineEdit::textChanged, [&]( const QString &text )
    {
        for( auto& item : mMapModelNameToNodeTreeWidgetItem )
        {
            for( int i = 0; i < item->childCount(); ++i )
            {
                auto child = item->child( i );
                auto nodeName = child->data( 0, Qt::UserRole ).toString();
                const bool match = ( nodeName.contains( text, Qt::CaseInsensitive ) );
                child->setHidden( !match );
            }
        }
    });
}

void
MainWindow::
updatePropertyExpandState()
{
    QList<QtBrowserItem *> vec = mpPropertyEditor->topLevelItems();
    QListIterator<QtBrowserItem *> it( vec );
    while( it.hasNext() )
    {
        QtBrowserItem * item = it.next();
        QtProperty * prop = item->property();
        mMapPropertyIdToExpanded[ mMapQtPropertyToPropertyId[ prop ] ] = mpPropertyEditor->isExpanded( item );
    }
}

// Set node's property when its property changed by the property browser.
void
MainWindow::
editorPropertyChanged(QtProperty * property, const QVariant & value)
{
    if( !mMapQtPropertyToPropertyId.contains( property ) )
        return;
    if( !mpSelectedNode )
        return;

    QString propId = mMapQtPropertyToPropertyId[ property ];
    //Update node's property
    mpSelectedNodeDataModel->setModelProperty( propId, value);
    mpSelectedNode->nodeGraphicsObject().setGeometryChanged();
    if( propId == "caption" )
    {
        auto nodeId = mpSelectedNode->id().toString();
        auto child = mMapNodeIDToNodeTreeWidgetItem[ nodeId ];
        child->setText( 0, value.toString() );
    }
    else if( propId == "minimize" )
    {
        mpSelectedNode->nodeGraphicsObject().setGeometryChanged();
        mpSelectedNode->nodeGeometry().recalculateSize();
        mpSelectedNode->nodeGraphicsObject().moveConnections();
    }
    else if( propId == "lock_position" )
    {
        mpSelectedNode->nodeGraphicsObject().lock_position( value.toBool() );
    }
    else if( propId == "draw_entries" )
    {
        mpSelectedNode->nodeGraphicsObject().setGeometryChanged();
        mpSelectedNode->nodeGeometry().recalculateSize();
        mpSelectedNode->nodeGraphicsObject().move_embeddedWidget();
        mpSelectedNode->nodeGraphicsObject().moveConnections();
    }
    //Update node's gui
    mpSelectedNode->nodeGraphicsObject().update();
    //Update History for Undo/Redo
    mpFlowScene->UpdateHistory();
}

// Set node's property browser when its propery changed from within node itself.
void
MainWindow::
nodePropertyChanged( std::shared_ptr< Property > prop)
{
    QString id = prop->getID();
    auto property = static_cast< QtVariantProperty * >( mMapPropertyIdToQtProperty[ id ] );
    auto type = prop->getType();

    if( type == QMetaType::QString )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
        property->setValue( typedProp->getData() );
    }
    else if( type == QMetaType::Int )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        property->setValue( typedProp->getData().miValue );
    }
    else if( type == QtVariantPropertyManager::enumTypeId() )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        property->setValue( typedProp->getData().miCurrentIndex );
    }
    else if( type == QMetaType::Bool )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
        property->setValue( typedProp->getData() );
    }
    else if( type == QtVariantPropertyManager::filePathTypeId() )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< FilePathPropertyType > >( prop );
        property->setValue( typedProp->getData().msFilename );
    }
    else if( type == QtVariantPropertyManager::pathTypeId() )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > >( prop );
        property->setValue( typedProp->getData().msPath );
    }
    else if( type == QMetaType::QSize )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
        auto size = QSize( typedProp->getData().miWidth, typedProp->getData().miHeight );
        property->setValue( size );
    }
    else if( type == QMetaType::QSizeF )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< SizeFPropertyType > >( prop );
        auto sizef = QSizeF( typedProp->getData().mfWidth, typedProp->getData().mfHeight );
        property->setValue( sizef );
    }
    else if( type == QMetaType::QPoint )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointPropertyType > >( prop );
        auto point = QPoint( typedProp->getData().miXPosition, typedProp->getData().miYPosition );
        property->setValue( point );
    }
    else if( type == QMetaType::QPointF )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PointFPropertyType > >( prop );
        auto pointf = QPointF( typedProp->getData().mfXPosition, typedProp->getData().mfYPosition );
        property->setValue( pointf );
    }
}

void
MainWindow::
clearPropertyBrowser()
{
    updatePropertyExpandState();

    QMap< QtProperty *, QString >::ConstIterator itProp = mMapQtPropertyToPropertyId.constBegin();
    while( itProp != mMapQtPropertyToPropertyId.constEnd() )
    {
        delete itProp.key();
        itProp++;
    }
    while( !mGroupPropertyManagerList.isEmpty() )
    {
        delete mGroupPropertyManagerList.last();
        mGroupPropertyManagerList.removeLast();
    }

    mMapQtPropertyToPropertyId.clear();
    mMapPropertyIdToQtProperty.clear();
    mGroupPropertyManagerList.clear();
}

void
MainWindow::
nodeCreated( Node & node )
{
    addToNodeTree( node );
    mpFlowScene->clearSelection();
    node.nodeGraphicsObject().setSelected( true );
}

void
MainWindow::
addToNodeTree( Node & node )
{
    auto skipText = QStringLiteral( "skip me" );
    auto modelName = node.nodeDataModel()->name();
    auto caption = node.nodeDataModel()->caption();
    auto nodeID = node.id().toString();
    if( !mMapModelNameToNodeTreeWidgetItem.contains( modelName ) )
    {
        auto item = new QTreeWidgetItem( ui->mpNodeListTreeView );
        item->setText( 0, modelName );
        item->setData( 0, Qt::UserRole, skipText );
        auto type = mpFlowScene->registry().create( modelName );
        item->setIcon( 0, QIcon( type->minPixmap() ) );
        mMapModelNameToNodeTreeWidgetItem[ modelName ] = item;
    }

    auto item = mMapModelNameToNodeTreeWidgetItem[ modelName ];
    auto child = new QTreeWidgetItem( item );
    child->setText( 0, caption);
    child->setData( 0, Qt::UserRole, caption );
    child->setText( 1, nodeID );
    child->setData( 1, Qt::UserRole, nodeID );

    mMapNodeIDToNode[ nodeID ] = & node;
    mMapNodeIDToNodeTreeWidgetItem[ nodeID ] = child;

    ui->mpNodeListTreeView->expandItem( item );
}

void
MainWindow::
nodeDeleted( Node & node )
{
    removeFromNodeTree( node );
    ui->mpNodeListTreeView->clearSelection();
}

void
MainWindow::
removeFromNodeTree( Node & node )
{
    auto nodeID = node.id().toString();
    if( mMapNodeIDToNode.contains( nodeID ) )
        mMapNodeIDToNode.remove( nodeID );
    auto child = mMapNodeIDToNodeTreeWidgetItem[ nodeID ];
    if( child )
    {
        mMapNodeIDToNodeTreeWidgetItem.remove( nodeID );
        auto parent = child->parent();
        delete child;
        if( parent->childCount() == 0 )
        {
            mMapModelNameToNodeTreeWidgetItem.remove( parent->text( 0 ) );
            delete parent;
        }
    }
}

void
MainWindow::
createScene(QString const & _filename, std::shared_ptr<DataModelRegistry> & pDataModelRegistry )
{
    QString filename("Untitle.flow");
    if( !_filename.isEmpty() )
        filename = _filename;

    struct SceneProperty sceneProperty;
    sceneProperty.pFlowScene = new PBFlowScene(this);
    sceneProperty.pFlowScene->setRegistry( pDataModelRegistry );
    sceneProperty.pFlowView = new PBFlowView();
    sceneProperty.pFlowView->setScene( sceneProperty.pFlowScene );
    sceneProperty.sFilename = filename;

    QFileInfo file(filename);
    int tabIndex = ui->mpTabWidget->addTab(static_cast<QWidget*>(sceneProperty.pFlowView), file.completeBaseName() );

    mpFlowScene = sceneProperty.pFlowScene;
    mpFlowView = sceneProperty.pFlowView;

    connect( mpFlowScene, &PBFlowScene::nodeCreated, this, &MainWindow::nodeCreated );
    connect( mpFlowScene, &PBFlowScene::nodeDeleted, this, &MainWindow::nodeDeleted );
    connect( mpFlowScene, &QtNodes::FlowScene::historyUpdated, this, &MainWindow::nodeChanged );
    connect( mpFlowScene, &QGraphicsScene::selectionChanged, this, &MainWindow::nodeInSceneSelectionChanged );

    mlSceneProperty.push_back( sceneProperty );
    mitSceneProperty = std::prev(mlSceneProperty.end());

    ui->mpTabWidget->setCurrentIndex( tabIndex );

    mpFlowScene->SetSnap2Grid(ui->mpActionSnapToGrid->isChecked());
}

bool
MainWindow::
closeScene( int index )
{
    bool isDiscard = false;
    QString tabTitle = ui->mpTabWidget->tabText( index );
    if( tabTitle.at(0) == QChar('*') )
    {
        QMessageBox msg;
        msg.setText("The scene " + tabTitle + " has been modified.");
        msg.setInformativeText( "Do you want to save the changes made to the scene?" );
        msg.setStandardButtons( QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel );
        msg.setDefaultButton( QMessageBox::Save );
        msg.setIcon( QMessageBox::Question );
        int ret = msg.exec();
        switch( ret )
        {
        case QMessageBox::Save:
            on_mpActionSave_triggered();
            break;
        case QMessageBox::Discard:
            isDiscard = true;
            break;
        case QMessageBox::Cancel:
            return false;
        }
    }
    if( !isDiscard )
    {
        // check it again because the page might not be save.
        tabTitle = ui->mpTabWidget->tabText( index );
        if( tabTitle.at(0) == QChar('*') )
            return false;
    }
    // if there is only one page and it's closing, just close it and add empty Untitle scene.
    if( ui->mpTabWidget->count() == 1 )
    {
        createScene( "", mpDataModelRegistry );
        ui->mpTabWidget->removeTab( 0 );
        delete mlSceneProperty.front().pFlowScene;
        delete mlSceneProperty.front().pFlowView;
        mlSceneProperty.pop_front();
    }
    else
    {
        PBFlowView * page2bClosed = static_cast<PBFlowView*>( ui->mpTabWidget->widget( index ) );
        ui->mpTabWidget->removeTab( index );
        for( std::list<struct SceneProperty>::iterator it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it )
        {
            if( it->pFlowView == page2bClosed )
            {
                delete it->pFlowScene;
                delete it->pFlowView;
                mlSceneProperty.erase( it );
                break;
            }
        }
        PBFlowView * currentPage = static_cast<PBFlowView*>( ui->mpTabWidget->currentWidget() );
        for( std::list<struct SceneProperty>::iterator it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it )
        {
            if( it->pFlowView == currentPage )
            {
                mpFlowScene = it->pFlowScene;
                mpFlowView = it->pFlowView;
                mitSceneProperty = it;
                break;
            }
        }
    }
    return true;
}

void
MainWindow::
nodeListClicked( QTreeWidgetItem * item, int )
{
    if( item->columnCount() == 2 )
    {
        mpFlowScene->clearSelection();

        auto node = mMapNodeIDToNode[ item->text( 1 ) ];
        node->nodeGraphicsObject().setSelected( true );
    }
}

void
MainWindow::
nodeListDoubleClicked( QTreeWidgetItem * item, int )
{
    if( item->columnCount() == 2 )
    {
        auto node = mMapNodeIDToNode[ item->text( 1 ) ];
        mpFlowView->center_on( node );
    }
}

void
MainWindow::
addProperty( QtVariantProperty *property, const QString & prop_id, const QString & sub_text )
{
    mMapQtPropertyToPropertyId[ property ] = prop_id;
    mMapPropertyIdToQtProperty[ prop_id ] = property;

    if( sub_text == "" )
    {
        QtBrowserItem * item = mpPropertyEditor->addProperty( property );
        if( mMapPropertyIdToExpanded.contains( prop_id ) )
            mpPropertyEditor->setExpanded( item, mMapPropertyIdToExpanded[ prop_id ] );
    }
    else if( mMapPropertyIdToQtProperty.contains( sub_text ) )
    {
        auto main_prop = mMapPropertyIdToQtProperty[ sub_text ];
        main_prop->addSubProperty( property );
    }
    else
    {
        QtGroupPropertyManager * new_group = new QtGroupPropertyManager( this );
        QtProperty * main_prop = new_group->addProperty( sub_text );
        main_prop->addSubProperty( property );

        mMapQtPropertyToPropertyId[ main_prop ] = sub_text;
        mMapPropertyIdToQtProperty[ sub_text ] = main_prop;
        mGroupPropertyManagerList.push_back( new_group );
        QtBrowserItem * item = mpPropertyEditor->addProperty( main_prop );
        if( mMapPropertyIdToExpanded.contains( sub_text ) )
            mpPropertyEditor->setExpanded( item, mMapPropertyIdToExpanded[ sub_text ] );
    }
}

void
MainWindow::
on_mpActionSave_triggered()
{
    if( !mitSceneProperty->sFilename.isEmpty() && mitSceneProperty->sFilename != QString("Untitle.flow") )
    {
        mpFlowScene->save( mitSceneProperty->sFilename );
        QFileInfo file( mitSceneProperty->sFilename );
        ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), file.completeBaseName() );
    }
    else
        on_mpActionSaveAs_triggered();
}

void
MainWindow::
on_mpActionLoad_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                   tr( "Open Flow Scene" ),
                                   QDir::homePath(),
                                   tr( "Flow Scene Files (*.flow)" ));
    if( filename.isEmpty() )
        return;
    loadScene(filename);
}

void
MainWindow::
on_mpActionQuit_triggered()
{
    close();
}

void
MainWindow::
on_mpActionLoadPlugin_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                   tr( "Load Plugin" ),
                                   QDir::homePath(),
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
                                   tr( "dll (*.dll)" ));
#elif defined( __APPLE__ )
                                   tr( "dylib (*.dylib)" ));
#elif defined( __linux__ )
                                   tr( "so (*.so)" ));
#endif

    if( filename.isEmpty() )
        return;
    load_plugin( mpDataModelRegistry, filename, mPluginsList );
    updateNodeCategoriesDockingWidget();
}

void
MainWindow::
closeEvent(QCloseEvent *ev)
{
    saveSettings();

    auto no_tabs = ui->mpTabWidget->count();
    for( int no_tab = no_tabs - 1; no_tab >= 0; no_tab-- )
    {
        if( !closeScene(no_tab) )
        {
            ev->ignore();
            return;
        }
    }
    ev->accept();
}

void
MainWindow::
on_mpActionSaveAs_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                   tr( "Save the Flow Scene to" ),
                                   QDir::homePath() + "/Untitle.flow",
                                   tr( "Flow Scene Files (*.flow)" ));

    if( !filename.isEmpty() )
    {
        if( !filename.endsWith( "flow", Qt::CaseInsensitive ) )
            filename += ".flow";
        if( mpFlowScene->save(filename) )
        {
            mitSceneProperty->sFilename = filename;
            QFileInfo file(filename);
            ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), file.completeBaseName() );
        }
    }
}

void
MainWindow::
on_mpActionSceneOnly_triggered()
{
    ui->mpAvailableNodeCategoryDockWidget->hide();
    ui->mpNodeListDockWidget->hide();
    ui->mpPropertyBrowserDockWidget->hide();
}

void
MainWindow::
on_mpActionAllPanels_triggered()
{
    ui->mpAvailableNodeCategoryDockWidget->show();
    ui->mpNodeListDockWidget->show();
    ui->mpPropertyBrowserDockWidget->show();
}

void
MainWindow::
on_mpActionZoomReset_triggered()
{
    mpFlowView->resetTransform();
}

void
MainWindow::
on_mpActionNew_triggered()
{
    if( mpFlowView )
        mpFlowView->addAnchor( 10 ); // Keep the current sceneRect.
    createScene( "", mpDataModelRegistry );
}

void
MainWindow::
on_mpActionCut_triggered()
{
    mpFlowView->cutSelectedNodes();
}

void
MainWindow::
on_mpActionCopy_triggered()
{
    mpFlowView->copySelectedNodes();
}

void
MainWindow::
on_mpActionPaste_triggered()
{
    mpFlowView->pasteNodes();
}

void
MainWindow::
on_mpActionDelete_triggered()
{
    mpFlowView->deleteSelectedNodes();
}

void
MainWindow::
on_mpActionUndo_triggered()
{
    mpFlowScene->Undo();
}

void
MainWindow::
on_mpActionRedo_triggered()
{
    mpFlowScene->Redo();
}

void
MainWindow::
on_mpActionEnableAll_triggered()
{
    auto nodes = mpFlowScene->allNodes();
    for( auto node : nodes )
    {
        if( !static_cast< PBNodeDataModel * >(node->nodeDataModel())->isSource() )
        {
            node->nodeDataModel()->setEnable(true);
            node->nodeGraphicsObject().update();
        }
    }
    for( auto node : nodes )
    {
        if( static_cast< PBNodeDataModel * >(node->nodeDataModel())->isSource() )
        {
            node->nodeDataModel()->setEnable(true);
            node->nodeGraphicsObject().update();
        }
    }
}

void
MainWindow::
on_mpActionDisableAll_triggered()
{
    auto nodes = mpFlowScene->allNodes();
    for( auto node : nodes )
    {
        node->nodeDataModel()->setEnable(false);
        node->nodeGraphicsObject().update();
    }
}

void
MainWindow::
on_mpActionSnapToGrid_toggled(bool checked)
{
    for( auto it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it )
        it->pFlowScene->SetSnap2Grid(checked);
}

void
MainWindow::
on_mpActionFocusView_toggled(bool checked)
{
    if( checked )
    {
        auto & nodes = mpFlowScene->nodes();
        for( auto it = nodes.begin(); it != nodes.end(); ++it )
        {
            if( !it->second.get()->nodeDataModel()->embeddedWidget() )
                it->second.get()->nodeGraphicsObject().hide();
            else
            {
                it->second.get()->nodeDataModel()->setDrawConnectionPoints(false);
                it->second.get()->nodeGraphicsObject().update();
            }
        }

        auto & connections = mpFlowScene->connections();
        for( auto it = connections.begin(); it != connections.end(); ++it )
            it->second.get()->getConnectionGraphicsObject().hide();

        ui->mpAvailableNodeCategoryDockWidget->hide();
        ui->mpNodeListDockWidget->hide();
        ui->mpPropertyBrowserDockWidget->hide();
        ui->mpToolBar->hide();
        ui->mpStatusBar->hide();

        ui->mpTabWidget->setTabsClosable(false);
    }
    else
    {
        auto & nodes = mpFlowScene->nodes();
        for( auto it = nodes.begin(); it != nodes.end(); ++it )
        {
            if( !it->second.get()->nodeDataModel()->embeddedWidget() )
                it->second.get()->nodeGraphicsObject().show();
            else
            {
                it->second.get()->nodeDataModel()->setDrawConnectionPoints(true);
                it->second.get()->nodeGraphicsObject().update();
            }
        }
        auto & connections = mpFlowScene->connections();
        for( auto it = connections.begin(); it != connections.end(); ++it )
            it->second.get()->getConnectionGraphicsObject().show();

        ui->mpAvailableNodeCategoryDockWidget->show();
        ui->mpNodeListDockWidget->show();
        ui->mpPropertyBrowserDockWidget->show();
        ui->mpToolBar->show();
        ui->mpStatusBar->show();

        ui->mpTabWidget->setTabsClosable(true);
    }
}

void
MainWindow::
on_mpActionFullScreen_toggled(bool checked)
{
    if( checked )
        showFullScreen();
    else
        showMaximized();
}

void
MainWindow::
tabPageChanged( int index )
{
    if( index < 0 )
        return;
    while( !mMapNodeIDToNode.empty() )
    {
        auto node = mMapNodeIDToNode.begin().value();
        removeFromNodeTree( *node );
    }
    for( auto it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it )
    {
        if( it->pFlowView == static_cast<PBFlowView*>(ui->mpTabWidget->currentWidget()) )
        {
            mitSceneProperty = it;
            if( mpFlowView )
                mpFlowView->addAnchor(10); // Keep the current sceneRect.
            mpFlowScene = it->pFlowScene;
            mpFlowView = it->pFlowView;
            mpFlowView->goToAnchor(10);    // Make sure to display the old view.
            break;
        }
    }
    auto nodes = mpFlowScene->allNodes();
    for( std::vector<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
    {
        auto & node = **it;
        addToNodeTree( node );
    }

    nodeInSceneSelectionChanged();
}

void
MainWindow::
nodeChanged()
{
    QString tabTitle = ui->mpTabWidget->tabText( ui->mpTabWidget->currentIndex() );
    if( tabTitle.length() != 0 && tabTitle.at(0) != QChar('*') )
    {
        tabTitle = "*" + tabTitle;
        ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), tabTitle );
    }
}

void
MainWindow::
loadSettings()
{
    msSettingFilename = QDir(QApplication::applicationDirPath()).filePath("cvdev.ini");
    if( QFileInfo::exists(msSettingFilename) )
    {
        QSettings settings(msSettingFilename, QSettings::IniFormat);
        auto filename = settings.value("Open Scene", "").toString();
        if( QFileInfo::exists(filename) )
            loadScene(filename);
        if( settings.value("Hide Node Category", false).toBool() )
            ui->mpAvailableNodeCategoryDockWidget->setHidden(true);
        if( settings.value("Hide Workspace", false).toBool() )
            ui->mpNodeListDockWidget->setHidden(true);
        if( settings.value("Hide Properties", false).toBool() )
            ui->mpPropertyBrowserDockWidget->setHidden(true);
        if( settings.value("In Focus View", false).toBool() )
            ui->mpActionFocusView->setChecked(true);
        if( settings.value("In Full Screen", false).toBool() )
            ui->mpActionFullScreen->setChecked(true);
    }
}

void
MainWindow::
saveSettings()
{
    QSettings settings(msSettingFilename, QSettings::IniFormat);
    if( !mitSceneProperty->sFilename.isEmpty() )
    {
        if( QFileInfo::exists(mitSceneProperty->sFilename) )
            settings.setValue("Open Scene", mitSceneProperty->sFilename);
        else
            settings.setValue("Open Scene", "");
    }
    settings.setValue("Hide Node Category", ui->mpAvailableNodeCategoryDockWidget->isHidden());
    settings.setValue("Hide Workspace", ui->mpNodeListDockWidget->isHidden());
    settings.setValue("Hide Properties", ui->mpPropertyBrowserDockWidget->isHidden());
    settings.setValue("In Focus View", ui->mpActionFocusView->isChecked());
    settings.setValue("In Full Screen", ui->mpActionFullScreen->isChecked());
}

void
MainWindow::
loadScene(QString & filename)
{
    if( mpFlowScene->allNodes().size() != 0 )
        createScene( filename, mpDataModelRegistry );
    else
        mitSceneProperty->sFilename = filename;
    if( mpFlowScene->load( filename ) )
    {
        QFileInfo file(filename);
        ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), file.completeBaseName() );

        auto nodes = mpFlowScene->allNodes();
        if( nodes.size() > 0 )
        {
            auto left_pos = std::numeric_limits<double>::max();
            auto right_pos = std::numeric_limits<double>::min();
            auto top_pos = std::numeric_limits<double>::max();
            auto bottom_pos = std::numeric_limits<double>::min();

            for( auto node : nodes )
            {
                auto nodeRect = node->nodeGraphicsObject().sceneBoundingRect();
                if( nodeRect.x() < left_pos )
                    left_pos = nodeRect.x();
                if( nodeRect.y() < top_pos )
                    top_pos = nodeRect.y();
                if( nodeRect.x() + nodeRect.width() > right_pos )
                    right_pos = nodeRect.x() + nodeRect.width();
                if( nodeRect.y() + nodeRect.height() > bottom_pos )
                    bottom_pos = nodeRect.y() + nodeRect.height();
            }
            auto center_pos = QPointF((left_pos+right_pos)*0.5, (top_pos+bottom_pos)*0.5);
            mpFlowView->center_on( center_pos );
        }
    }
}

void MainWindow::
on_mpActionAbout_triggered()
{
    QMessageBox::about(this, msProgramName, "<p>" + msProgramName + "(Beta 0) has been designed and developped as a software tool so that "
                                           "developers can reuse their codes and share their work with others. If you have any comment please "
                                           "feel free to contact <a href=mailto:pished.bunnun@nectec.or.th>pished.bunnun@nectec.or.th</a>.</p>"
                                           "<p>Copyright (C) 2022 <a href=www.nectec.or.th>NECTEC</a> All rights reserved.</p>"
                                           "<p>" + msProgramName + " is made possible by open source softwares.</p>");
}
