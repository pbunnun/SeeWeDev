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

/**
 * @file MainWindow.cpp
 * @brief Implementation of the MainWindow class
 * 
 * This file contains the implementation of CVDev's main application window,
 * including UI setup, scene management, property browser integration, and
 * all menu action handlers.
 */

#include <QPluginLoader>
#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QTabWidget>
#include <QChar>
#include <QClipboard>
#include <QApplication>
#include <QUuid>
#include <QInputDialog>
#include <QColorDialog>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/internal/UndoCommands.hpp>
#include <QtNodes/internal/StyleCollection.hpp>
#include <QGraphicsScene>
#include <QVector>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QDate>
#include <QStandardPaths>
#include <QUndoStack>
#include "MainWindow.hpp"

#include "PluginInterface.hpp"
#include "PBDataFlowGraphicsScene.hpp"
#include "PBFlowGraphicsView.hpp"
#include "PropertyChangeCommand.hpp"
#include "GroupCommands.hpp"
#include "PBNodeGroup.hpp"
#include "PBNodeGroupGraphicsItem.hpp"
#include "PBDataFlowGraphicsScene.hpp"
#include "ui_MainWindow.h"
#include "qtpropertybrowser_p.h"
#include "qtvariantproperty_p.h"
#include "qttreepropertybrowser_p.h"

using QtNodes::NodeDelegateModelRegistry;

/**
 * @brief MainWindow constructor - initializes the entire application UI
 * 
 * Setup sequence:
 * 1. Initialize Qt UI components
 * 2. Check version/expiration warning
 * 3. Create plugin registry and load plugins
 * 4. Create initial empty scene
 * 5. Setup dock widgets (property browser, node categories, node list)
 * 6. Connect all signal-slot pairs
 * 7. Load saved settings
 */
MainWindow::MainWindow( QWidget *parent )
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
{
    ui->setupUi( this );

    // Show a visual shortcut hint in the Edit menu for Copy/Cut/Paste/Delete
    // without assigning the actual shortcut to the menu action (to avoid duplicate triggers).
#if defined(Q_OS_MAC)
    // Use the Unicode "Command" symbol (U+2318) so macOS shows the familiar ⌘ glyph
    ui->mpActionCopyMenuProxy->setText(QStringLiteral("Copy\t\u2318C"));
    ui->mpActionCutMenuProxy->setText(QStringLiteral("Cut\t\u2318X"));
    ui->mpActionPasteMenuProxy->setText(QStringLiteral("Paste\t\u2318V"));
    ui->mpActionDeleteMenuProxy->setText(QStringLiteral("Delete\tDel"));
#else
    ui->mpActionCopyMenuProxy->setText(QStringLiteral("Copy\tCtrl+C"));
    ui->mpActionCutMenuProxy->setText(QStringLiteral("Cut\tCtrl+X"));
    ui->mpActionPasteMenuProxy->setText(QStringLiteral("Paste\tCtrl+V"));
    ui->mpActionDeleteMenuProxy->setText(QStringLiteral("Delete\tDel"));
#endif
    // Disable node shadows globally at startup to avoid QGraphicsDropShadowEffect
    // rasterization issues during geometry updates. Override the NodeStyle via
    // StyleCollection without modifying NodeEditor source files.
    {
        QtNodes::NodeStyle nodeStyle = QtNodes::StyleCollection::nodeStyle();
        nodeStyle.ShadowEnabled = false;
        QtNodes::StyleCollection::setNodeStyle(nodeStyle);
    }

    // Clear the clipboard on startup to ensure paste is disabled initially
    // This prevents confusion from OS clipboard data or previous session data
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();

    // Version warning: Alert users if software is older than 1 year
    QDate check_day(2025, 1, 1);
    QDate current = QDate::currentDate();
    int no_days = check_day.daysTo(current);
    if( no_days >= 365 )
        QMessageBox::warning(this, msProgramName, "<p>This version is too old. There might be a newer version with some bugs fixed and improvements. "
                                                 "Please contact <a href=mailto:pished.bunnun@nectec.or.th>pished.bunnun@nectec.or.th</a> to get a new version.</p>");

    // Create shared registry for all node types (plugins + built-in nodes)
    mpDelegateModelRegistry = std::make_shared<NodeDelegateModelRegistry>();
    // NOTE: Plugin loading deferred below to speed up initial GUI appearance.

    // Create the first empty scene (Untitle.flow)
    createScene( "", mpDelegateModelRegistry );

    // Setup node list tree view columns
    QStringList headers = { "Caption", "ID" };
    ui->mpNodeListTreeView->setHeaderLabels( headers );

    ui->mpMenuView->addAction( ui->mpAvailableNodeCategoryDockWidget->toggleViewAction() );
    ui->mpMenuView->addAction( ui->mpNodeListDockWidget->toggleViewAction() );
    ui->mpMenuView->addAction( ui->mpPropertyBrowserDockWidget->toggleViewAction() );

    setupPropertyBrowserDockingWidget();
    setupNodeCategoriesDockingWidget();
    setupNodeListDockingWidget();

    // Manual signal-slot connections for node tree and tabs
    connect( ui->mpNodeListTreeView, &QTreeWidget::itemClicked, this, &MainWindow::nodeListClicked );
    connect( ui->mpNodeListTreeView, &QTreeWidget::itemDoubleClicked, this, &MainWindow::nodeListDoubleClicked );
    connect( ui->mpTabWidget, &QTabWidget::currentChanged, this, &MainWindow::tabPageChanged );
    connect( ui->mpTabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeScene );

    // Manual signal-slot connections for actions (instead of auto-connect)
    connect( ui->mpActionNew, &QAction::triggered, this, &MainWindow::actionNew_slot );
    connect( ui->mpActionSave, &QAction::triggered, this, &MainWindow::actionSave_slot );
    connect( ui->mpActionLoad, &QAction::triggered, this, &MainWindow::actionLoad_slot );
    connect( ui->mpActionQuit, &QAction::triggered, this, &MainWindow::actionQuit_slot );
    connect( ui->mpActionSaveAs, &QAction::triggered, this, &MainWindow::actionSaveAs_slot );

    connect( ui->mpActionSceneOnly, &QAction::triggered, this, &MainWindow::actionSceneOnly_slot );
    connect( ui->mpActionAllPanels, &QAction::triggered, this, &MainWindow::actionAllPanels_slot );
    connect( ui->mpActionZoomReset, &QAction::triggered, this, &MainWindow::actionZoomReset_slot );

    // Copy action removed from main menu. Copy is provided by view-local action in GraphicsView.
    // Delegate Cut to the active view
    connect( ui->mpActionCutMenuProxy, &QAction::triggered, this, [this]() {
        PBFlowGraphicsView* view = getCurrentView();
        if (view)
            view->triggerCut();
    });
    // Delegate Paste to the active view
    connect( ui->mpActionPasteMenuProxy, &QAction::triggered, this, [this]() {
        PBFlowGraphicsView* view = getCurrentView();
        if (view)
            view->triggerPaste();
    });
    // Delegate menu Copy to the active view's copy implementation (no shortcut here)
    connect( ui->mpActionCopyMenuProxy, &QAction::triggered, this, [this]() {
        PBFlowGraphicsView* view = getCurrentView();
        if (view)
            view->triggerCopy();
    });
    // Delegate Delete to the active view
    connect( ui->mpActionDeleteMenuProxy, &QAction::triggered, this, [this]() {
        PBFlowGraphicsView* view = getCurrentView();
        if (view)
            view->triggerDelete();
    });

    connect( ui->mpActionUndo, &QAction::triggered, this, &MainWindow::actionUndo_slot );
    connect( ui->mpActionRedo, &QAction::triggered, this, &MainWindow::actionRedo_slot );

    connect( ui->mpActionDisableAll, &QAction::triggered, this, &MainWindow::actionDisableAll_slot );
    connect( ui->mpActionEnableAll, &QAction::triggered, this, &MainWindow::actionEnableAll_slot );

    connect( ui->mpActionSnapToGrid, &QAction::toggled, this, &MainWindow::actionSnapToGrid_slot );

    connect( ui->mpActionLoadPlugin, &QAction::triggered, this, &MainWindow::actionLoadPlugin_slot );

    connect( ui->mpActionFocusView, &QAction::toggled, this, &MainWindow::actionFocusView_slot );

    connect( ui->mpActionFullScreen, &QAction::toggled, this, &MainWindow::actionFullScreen_slot );

    connect( ui->mpActionAbout, &QAction::triggered, this, &MainWindow::actionAbout_slot );
    

    // Group actions
    connect( ui->mpActionGroupNodes, &QAction::triggered, this, &MainWindow::actionGroupSelectedNodes_slot );
    connect( ui->mpActionUngroupNodes, &QAction::triggered, this, &MainWindow::actionUngroupSelectedNodes_slot );
    connect( ui->mpActionRenameGroup, &QAction::triggered, this, &MainWindow::actionRenameGroup_slot );
    connect( ui->mpActionChangeGroupColor, &QAction::triggered, this, &MainWindow::actionChangeGroupColor_slot );

    setWindowTitle( msProgramName );

    showMaximized();

    // Defer plugin loading + settings restore slightly so the window shows fast.
    // We must load plugins BEFORE restoring previous scenes (loadSettings),
    // otherwise scene nodes from plugins won't be recognized. Use a short delay
    // (1 ms) to allow the GUI to render before heavy I/O begins.
    statusBar()->showMessage("Loading plugins...");
    QApplication::processEvents();
    QTimer::singleShot(1, this, [this]() {
        DEBUG_LOG_INFO() << "[MainWindow] Deferred plugin loading starting...";
        load_plugins(mpDelegateModelRegistry, mPluginsList);
        updateNodeCategoriesDockingWidget();
        DEBUG_LOG_INFO() << "[MainWindow] Deferred plugin loading completed. Restoring settings...";
        statusBar()->showMessage("Loading scene...");
        QApplication::processEvents();
        loadSettings();
        statusBar()->clearMessage();
        DEBUG_LOG_INFO() << "[MainWindow] Settings restore completed.";
    });
}

MainWindow::
~MainWindow()
{
    while( !mlSceneProperty.empty() )
    {
        struct SceneProperty sceneProperty = mlSceneProperty.back();

        // Delete view first (it references the scene)
        delete sceneProperty.pFlowGraphicsView;
        // Delete scene second (it references the model)
        delete sceneProperty.pDataFlowGraphicsScene;
        // Delete model last
        delete sceneProperty.pDataFlowGraphModel;
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
    // Safety check - don't access scene during destruction
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphModel* model = getCurrentModel();
    if (!view || !model)
        return;

    // First, check if a group graphics item is selected in the scene.
    // If so, select the corresponding entry in the Workspace tree and
    // show no node properties for the group selection.
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    if (scene) {
        auto selectedItems = scene->selectedItems();
        for (auto *it : selectedItems) {
            if (auto *groupItem = qgraphicsitem_cast<PBNodeGroupGraphicsItem*>(it)) {
                GroupId gid = groupItem->groupId();

                // Clear property browser (group selection does not show node props)
                clearPropertyBrowser();

                // Clear any existing selections in the tree and select group item
                ui->mpNodeListTreeView->clearSelection();
                if (mMapGroupIdToNodeTreeWidgetItem.contains(gid)) {
                    QTreeWidgetItem *groupTreeItem = mMapGroupIdToNodeTreeWidgetItem[gid];
                    if (groupTreeItem) {
                        groupTreeItem->setSelected(true);
                    }
                }

                // We prioritize group selection over node selection so return
                // after handling the first found selected group.
                return;
            }
        }
    }

    auto selectedNodeIds = view->selectedNodes();

    if( selectedNodeIds.size() == 1 )
    {
        // First, deselect all nodes to ensure only one node has mbSelected=true
        // Only disconnect property-related signals; keep selection_request_signal connected
        auto allNodeIds = model->allNodeIds();
        for (const auto& nodeId : allNodeIds)
        {
            auto* delegateModel = model->delegateModel<PBNodeDelegateModel>(nodeId);
            if (delegateModel && delegateModel->isSelected())
            {
                delegateModel->setSelected(false);
                // Disconnect all signals from this delegate model
                disconnect(delegateModel, nullptr, this, nullptr);
                
                // Reconnect the selection_request_signal so unselected nodes can still request selection
                connect(delegateModel, SIGNAL(selection_request_signal()),
                        this, SLOT(handleSelectionRequest()));
            }
        }
        
        clearPropertyBrowser();

        NodeId selectedNodeId = selectedNodeIds[0];
        auto* selectedNodeDelegateModel = model->delegateModel<PBNodeDelegateModel>(selectedNodeId);
        if (!selectedNodeDelegateModel) {
            qDebug() << "Failed to get delegate model for selected node" << selectedNodeId;
            return;
        }

        selectedNodeDelegateModel->setSelected( true ); // TODO: This should not be called explicitly. It could have done in NodeGraphicsObject class.
        connect( selectedNodeDelegateModel, SIGNAL(property_changed_signal(std::shared_ptr<Property>)),
                this, SLOT(nodePropertyChanged(std::shared_ptr<Property>)) );
        connect( selectedNodeDelegateModel, SIGNAL(property_change_request_signal(const QString&, const QVariant&, const QVariant&)),
                this, SLOT(handlePropertyChangeRequest(const QString&, const QVariant&, const QVariant&)) );
        connect( selectedNodeDelegateModel, SIGNAL(selection_request_signal()),
                this, SLOT(handleSelectionRequest()) );
        connect( selectedNodeDelegateModel, SIGNAL( property_structure_changed_signal() ),
                this, SLOT( nodeInSceneSelectionChanged() ) );

        auto propertyVector = selectedNodeDelegateModel->getProperty();

        // Block signals while populating property browser to prevent spurious editorPropertyChanged() calls
        mpVariantManager->blockSignals(true);

        auto nodeTreeWidgetItem = mMapNodeIdToNodeTreeWidgetItem[ selectedNodeId ];
        ui->mpNodeListTreeView->clearSelection();
        if (nodeTreeWidgetItem)
            nodeTreeWidgetItem->setSelected( true );

        QtVariantProperty *property;
        property = mpVariantManager->addProperty( QMetaType::QString, "Node ID" );
        property->setAttribute( "readOnly", true);
        property->setValue( selectedNodeId );
        addProperty( property, "id", "Common" );

        property = mpVariantManager->addProperty( QMetaType::Bool, "Source" );
        property->setAttribute( "readOnly", true );
        property->setAttribute( QLatin1String( "textVisible" ), false );
        property->setValue( selectedNodeDelegateModel->isSource() );
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
        
        // Unblock signals after all properties are added
        mpVariantManager->blockSignals(false);
    }
    else
    {
        // Multiple or no selection - clear the property browser and deselect all nodes
        PBDataFlowGraphModel* model = getCurrentModel();
        
        if (model)
        {
            // Deselect ALL nodes to ensure mbSelected is properly synced
            auto allNodeIds = model->allNodeIds();
            for (const auto& nodeId : allNodeIds)
            {
                auto* delegateModel = model->delegateModel<PBNodeDelegateModel>(nodeId);
                if (delegateModel && delegateModel->isSelected())
                {
                    delegateModel->setSelected(false);
                    // Disconnect all signals from this delegate model
                    disconnect(delegateModel, nullptr, this, nullptr);
                    
                    // Reconnect the selection_request_signal so unselected nodes can still request selection
                    connect(delegateModel, SIGNAL(selection_request_signal()),
                            this, SLOT(handleSelectionRequest()));
                }
            }
        }
        
        clearPropertyBrowser();
        ui->mpNodeListTreeView->clearSelection();
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
    
    // Connect expand/collapse all buttons
    connect(ui->mpExpandAllButton, &QPushButton::clicked, this, [this]() {
        ui->mpAvailableNodeCategoryTreeView->expandAll();
    });
    
    connect(ui->mpCollapseAllButton, &QPushButton::clicked, this, [this]() {
        ui->mpAvailableNodeCategoryTreeView->collapseAll();
    });
    
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
    for ( auto const &cat : mpDelegateModelRegistry->categories() )
    {
        auto item = new QTreeWidgetItem( ui->mpAvailableNodeCategoryTreeView );
        item->setText( 0, cat );
        item->setData( 0, Qt::UserRole, skipText );
        mMapModelCategoryToNodeTreeWidgetItem[ cat ] = item;
    }

    for ( auto const &assoc : mpDelegateModelRegistry->registeredModelsCategoryAssociation() )
    {
        auto parent = mMapModelCategoryToNodeTreeWidgetItem[assoc.second];
        auto item = new QTreeWidgetItem( parent );
        item->setText( 0, assoc.first );
        item->setData( 0, Qt::UserRole, assoc.first );

        auto type = mpDelegateModelRegistry->create( assoc.first );
        auto pbType = dynamic_cast<PBNodeDelegateModel*>(type.get());
        if (pbType) {
            item->setIcon( 0, QIcon( pbType->minPixmap() ) );
        }
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

    // Enable custom context menu for the node/group workspace tree
    ui->mpNodeListTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mpNodeListTreeView, &QWidget::customContextMenuRequested,
            this, &MainWindow::nodeListContextMenuRequested);
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

// Handle property change requests from embedded widgets or programmatic changes
void
MainWindow::
handlePropertyChangeRequest(const QString& propertyId, const QVariant& oldValue, const QVariant& newValue)
{
    DEBUG_LOG_INFO() << "[handlePropertyChangeRequest] propertyId:" << propertyId 
            << "oldValue:" << oldValue 
            << "newValue:" << newValue;
    
    // Get the node that sent this signal
    PBNodeDelegateModel* senderModel = qobject_cast<PBNodeDelegateModel*>(sender());
    if (!senderModel)
    {
        DEBUG_LOG_INFO() << "[handlePropertyChangeRequest] No sender model, returning";
        return;
    }
    
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    PBDataFlowGraphModel* model = getCurrentModel();
    if (!view || !scene || !model)
    {
        DEBUG_LOG_INFO() << "[handlePropertyChangeRequest] No view/scene/model, returning";
        return;
    }
    
    // Find the NodeId for this delegate model
    QtNodes::NodeId nodeId = QtNodes::InvalidNodeId;
    auto allNodeIds = model->allNodeIds();
    for (const auto& id : allNodeIds)
    {
        if (model->delegateModel<PBNodeDelegateModel>(id) == senderModel)
        {
            nodeId = id;
            break;
        }
    }
    
    if (nodeId == QtNodes::InvalidNodeId)
    {
        DEBUG_LOG_INFO() << "[handlePropertyChangeRequest] Invalid NodeId, returning";
        return;
    }
    
    DEBUG_LOG_INFO() << "[handlePropertyChangeRequest] Found NodeId:" << nodeId 
            << "Creating PropertyChangeCommand";
    
    // Create and push undo command
    auto *cmd = new PropertyChangeCommand(scene,
                                          nodeId,
                                          senderModel,
                                          propertyId,
                                          oldValue,
                                          newValue);
    
    DEBUG_LOG_INFO() << "[handlePropertyChangeRequest] Pushing command to undo stack";
    scene->undoStack().push(cmd);
    
    // Update tree widget caption if needed
    if( propertyId == "caption" )
    {
        auto child = mMapNodeIdToNodeTreeWidgetItem[ nodeId ];
        if (child) {
            child->setText( 0, newValue.toString() );
        }
    }
}

// Handle selection requests from unselected nodes
void
MainWindow::
handleSelectionRequest()
{
    DEBUG_LOG_INFO() << "[handleSelectionRequest] Selection requested";
    
    // Get the node that sent this signal
    PBNodeDelegateModel* senderModel = qobject_cast<PBNodeDelegateModel*>(sender());
    if (!senderModel)
    {
        DEBUG_LOG_INFO() << "[handleSelectionRequest] No sender model, returning";
        return;
    }
    
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    PBDataFlowGraphModel* model = getCurrentModel();
    if (!view || !scene || !model)
    {
        DEBUG_LOG_INFO() << "[handleSelectionRequest] No view/scene/model, returning";
        return;
    }
    
    // Find the NodeId for this delegate model
    QtNodes::NodeId nodeId = QtNodes::InvalidNodeId;
    auto allNodeIds = model->allNodeIds();
    for (const auto& id : allNodeIds)
    {
        if (model->delegateModel<PBNodeDelegateModel>(id) == senderModel)
        {
            nodeId = id;
            break;
        }
    }
    
    if (nodeId == QtNodes::InvalidNodeId)
    {
        DEBUG_LOG_INFO() << "[handleSelectionRequest] Invalid NodeId, returning";
        return;
    }
    
    // Get the graphics object and select it
    auto nodeGraphicsObject = mMapNodeIdToNodeGraphicsObject[nodeId];
    if (nodeGraphicsObject)
    {
        DEBUG_LOG_INFO() << "[handleSelectionRequest] Selecting NodeId:" << nodeId;
        view->clearSelection();
        nodeGraphicsObject->setSelected(true);
    }
    else
    {
        DEBUG_LOG_INFO() << "[handleSelectionRequest] No graphics object found for NodeId:" << nodeId;
    }
}

// Set node's property when its property changed by the property browser.
void
MainWindow::
editorPropertyChanged(QtProperty * property, const QVariant & value)
{
    DEBUG_LOG_INFO() << "[editorPropertyChanged] property:" << property->propertyName() 
            << "value:" << value
            << "mbApplyingUndoRedo:" << mbApplyingUndoRedo;
    
    if( !mMapQtPropertyToPropertyId.contains( property ) )
    {
        DEBUG_LOG_INFO() << "[editorPropertyChanged] Property not in map, returning";
        return;
    }

    // Don't create undo commands if we're currently applying an undo/redo operation
    // This prevents infinite loops when undo/redo updates the Property Browser
    if (mbApplyingUndoRedo)
    {
        DEBUG_LOG_INFO() << "[editorPropertyChanged] Applying undo/redo, skipping";
        return;
    }

    // Get the currently selected node at runtime instead of relying on stored selection
    // This prevents issues when undo/redo affects different nodes
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    PBDataFlowGraphModel* model = getCurrentModel();
    if (!view || !scene || !model)
    {
        DEBUG_LOG_INFO() << "[editorPropertyChanged] No view/scene/model, returning";
        return;
    }
        
    auto selectedNodeIds = view->selectedNodes();
    if (selectedNodeIds.size() != 1)
    {
        DEBUG_LOG_INFO() << "[editorPropertyChanged] Not exactly one selected node, returning";
        return;  // Only handle single selection
    }
    
    QtNodes::NodeId nodeId = selectedNodeIds[0];
    auto *delegateModel = model->delegateModel<PBNodeDelegateModel>(nodeId);
    if (!delegateModel)
    {
        DEBUG_LOG_INFO() << "[editorPropertyChanged] No delegate model, returning";
        return;
    }

    QString propId = mMapQtPropertyToPropertyId[ property ];
    
    // Get the old value before making changes
    QVariant oldValue = delegateModel->getModelPropertyValue(propId);
    
    DEBUG_LOG_INFO() << "[editorPropertyChanged] NodeId:" << nodeId 
            << "propId:" << propId 
            << "oldValue:" << oldValue
            << "Creating PropertyChangeCommand";
    
    // Create and push undo command
    auto *cmd = new PropertyChangeCommand(scene,
                                          nodeId,
                                          delegateModel,
                                          propId,
                                          oldValue,
                                          value);
    
    DEBUG_LOG_INFO() << "[editorPropertyChanged] Pushing command to undo stack";
    scene->undoStack().push(cmd);

    // Update tree widget caption if needed
    if( propId == "caption" )
    {
        auto child = mMapNodeIdToNodeTreeWidgetItem[ nodeId ];
        if (child) {
            child->setText( 0, value.toString() );
        }
    }
}

// Set node's property browser when its propery changed from within node itself.
void
MainWindow::
nodePropertyChanged( std::shared_ptr< Property > prop)
{
    DEBUG_LOG_INFO() << "[nodePropertyChanged] propertyId:" << prop->getID() 
            << "propertyName:" << prop->getName();
    
    // Block undo command creation while updating Property Browser from model changes
    mbApplyingUndoRedo = true;
    
    QString id = prop->getID();
    
    // Check if this property exists in the property browser
    // Some properties (like read-only display properties or normal properties but 
    // its node was not selected) may not be added to the browser
    if (!mMapPropertyIdToQtProperty.contains(id)) {
        DEBUG_LOG_INFO() << "[nodePropertyChanged] Property not in Property Browser map, skipping UI update";
        mbApplyingUndoRedo = false;
        return;
    }
    
    DEBUG_LOG_INFO() << "[nodePropertyChanged] Updating Property Browser UI";
    
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
    
    DEBUG_LOG_INFO() << "[nodePropertyChanged] Property Browser UI updated, re-enabling undo command creation";
    
    // Re-enable undo command creation
    mbApplyingUndoRedo = false;
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
nodeCreated( NodeId nodeId )
{
    PBDataFlowGraphModel* model = getCurrentModel();

    auto* delegateModel = model ? model->delegateModel<PBNodeDelegateModel>(nodeId) : nullptr;
    if (delegateModel) {
        // Connect selection request signal so unselected nodes can request to be selected
        connect( delegateModel, SIGNAL(selection_request_signal()),
                this, SLOT(handleSelectionRequest()) );
    }

    addToNodeTree( nodeId );

    // Selection is handled through the graphics scene
    // Clear existing selection and select the newly created node
    PBFlowGraphicsView* view = getCurrentView();
    auto nodeGraphicsObject = mMapNodeIdToNodeGraphicsObject[nodeId];
    
    if (view && nodeGraphicsObject)
    {
        view->clearSelection();
        nodeGraphicsObject->setSelected(true);
    }

    // Mark the undo stack as not clean to indicate unsaved changes
    if (mitSceneProperty != mlSceneProperty.end() && mitSceneProperty->pDataFlowGraphicsScene)
    {
        mitSceneProperty->pDataFlowGraphicsScene->undoStack().resetClean();
    }
}

void
MainWindow::
addToNodeTree( NodeId nodeId )
{
    auto skipText = QStringLiteral( "skip me" );

    PBDataFlowGraphModel* model = getCurrentModel();
    PBFlowGraphicsView* view = getCurrentView();
    
    // We need to get the delegate model through the graph model
    auto delegateModel = model ? model->delegateModel<PBNodeDelegateModel>(nodeId) : nullptr;
    if (!delegateModel) {
        qDebug() << "Failed to get delegate model for node" << nodeId;
        return;
    }

    auto modelName = delegateModel->name();
    auto caption = delegateModel->caption();
    auto stringNodeID = QString::number(nodeId);  // NodeId is unsigned int in v3

    if( !mMapModelNameToNodeTreeWidgetItem.contains( modelName ) )
    {
        auto item = new QTreeWidgetItem( ui->mpNodeListTreeView );
        item->setText( 0, modelName );
        item->setData( 0, Qt::UserRole, skipText );

        auto registry = model ? model->dataModelRegistry() : nullptr;
        if (registry) {
            auto type = registry->create( modelName );
            if (type) {
                auto pbType = dynamic_cast<PBNodeDelegateModel*>(type.get());
                if (pbType) {
                    item->setIcon( 0, QIcon( pbType->minPixmap() ) );
                }
            }
        }
        mMapModelNameToNodeTreeWidgetItem[ modelName ] = item;
    }

    auto item = mMapModelNameToNodeTreeWidgetItem[ modelName ];
    auto child = new QTreeWidgetItem( item );
    child->setText( 0, caption);
    child->setData( 0, Qt::UserRole, caption );
    child->setText( 1, stringNodeID );
    child->setData( 1, Qt::UserRole, stringNodeID );

    mMapNodeIdToNodeTreeWidgetItem[ nodeId ] = child;
    mMapNodeIdToNodeDelegateModel[ nodeId ] = delegateModel;
    mMapNodeIdToNodeGraphicsObject[ nodeId ] = view ? view->getGraphicsObject(nodeId) : nullptr;

    ui->mpNodeListTreeView->expandItem( item );
}

void
MainWindow::
nodeDeleted( NodeId nodeId )
{
    // If the deleted node was the currently selected one, disconnect signals and clear selection
    auto result = getSelectedNodeId();
    auto* selectedNodeDelegateModel = getSelectedNodeDelegateModel();
    
    // Only proceed if a node is selected AND it matches the deleted node
    // This now correctly handles the case where node 0 is selected
    if (result.hasSelection && nodeId == result.nodeId && selectedNodeDelegateModel != nullptr)
    {
        selectedNodeDelegateModel->setSelected(false);
        disconnect(selectedNodeDelegateModel, nullptr, this, nullptr);
        
        // Clear the property browser
        clearPropertyBrowser();
    }
    
    removeFromNodeTree( nodeId );
    ui->mpNodeListTreeView->clearSelection();

    // Mark the undo stack as not clean to indicate unsaved changes
    if (mitSceneProperty != mlSceneProperty.end() && mitSceneProperty->pDataFlowGraphicsScene)
    {
        mitSceneProperty->pDataFlowGraphicsScene->undoStack().resetClean();
    }
}

void
MainWindow::
removeFromNodeTree( NodeId nodeId )
{
    if( mMapNodeIdToNodeDelegateModel.contains( nodeId ) )
        mMapNodeIdToNodeDelegateModel.remove( nodeId );
    if( mMapNodeIdToNodeGraphicsObject.contains( nodeId ) )
        mMapNodeIdToNodeGraphicsObject.remove( nodeId );
    auto child = mMapNodeIdToNodeTreeWidgetItem[ nodeId ];
    if( child )
    {
        mMapNodeIdToNodeTreeWidgetItem.remove( nodeId );
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
addToGroupTree( GroupId groupId )
{
    auto skipText = QStringLiteral( "skip me" );

    PBDataFlowGraphModel* model = getCurrentModel();
    if (!model)
        return;

    const PBNodeGroup* group = model->getGroup(groupId);
    if (!group)
        return;

    // Create the Groups root if it doesn't exist
    if (!mGroupRootItem) {
        mGroupRootItem = new QTreeWidgetItem( ui->mpNodeListTreeView );
        mGroupRootItem->setText(0, "Groups");
        mGroupRootItem->setData(0, Qt::UserRole, skipText);
    }
    // If we already have a tree item for this group, update it instead of creating a duplicate
    if (mMapGroupIdToNodeTreeWidgetItem.contains(groupId)) {
        QTreeWidgetItem* existing = mMapGroupIdToNodeTreeWidgetItem[groupId];
        if (existing) {
            existing->setText(0, group->name());
            existing->setText(1, QString::number(groupId));
        }
        ui->mpNodeListTreeView->expandItem( mGroupRootItem );
        return;
    }

    auto child = new QTreeWidgetItem( mGroupRootItem );
    child->setText(0, group->name());
    child->setData(0, Qt::UserRole, group->name());
    // Store the GroupId in a reserved user role so we can detect group items
    child->setData(0, Qt::UserRole + 1, QVariant::fromValue( static_cast<uint>(groupId) ));
    child->setText(1, QString::number(groupId));

    mMapGroupIdToNodeTreeWidgetItem[ groupId ] = child;

    ui->mpNodeListTreeView->expandItem( mGroupRootItem );
}

void
MainWindow::
removeFromGroupTree( GroupId groupId )
{
    if (!mMapGroupIdToNodeTreeWidgetItem.contains(groupId))
        return;

    auto child = mMapGroupIdToNodeTreeWidgetItem[ groupId ];
    if (child) {
        mMapGroupIdToNodeTreeWidgetItem.remove(groupId);
        delete child;
    }

    // If no more groups, remove the root
    if (mGroupRootItem && mGroupRootItem->childCount() == 0) {
        delete mGroupRootItem;
        mGroupRootItem = nullptr;
    }
}

void
MainWindow::
groupCreated( GroupId groupId )
{
    // Only update tree for the current scene
    addToGroupTree(groupId);
}

void
MainWindow::
groupDissolved( GroupId groupId )
{
    removeFromGroupTree(groupId);
}

/**
 * @brief Creates a new flow graph scene with complete MVC hierarchy
 * 
 * Object creation order is critical for proper initialization:
 * 1. Model: Contains the data (nodes, connections)
 * 2. Scene: Provides graphics visualization and undo stack
 * 3. View: Qt widget for user interaction
 * 
 * Object lifetime management:
 * - Model is stored in SceneProperty and deleted last (others reference it)
 * - Scene is QObject-parented to MainWindow for automatic cleanup
 * - View is added to tab widget (ownership transferred to QTabWidget)
 * - Delete order in closeScene: view -> scene -> model (reverse of creation)
 * 
 * Signal connections:
 * - Model signals (nodeCreated, nodeDeleted) -> MainWindow slots for UI updates
 * - Scene undo stack -> Menu actions for enabling/disabling undo/redo
 * - Scene selection -> Property browser updates
 * 
 * Why use local variables for connections instead of member pointers?
 * The old design cached pointers (mpFlowGraphModel, etc.) and used them for
 * signal connections. This created synchronization problems when tabs switched.
 * Now we use local variables here (safe - objects won't be deleted during this
 * function) and query dynamically elsewhere via getCurrentModel(), etc.
 */
void
MainWindow::
createScene(QString const & _filename, std::shared_ptr<NodeDelegateModelRegistry> & pDataModelRegistry )
{
    QString filename("Untitle.flow");
    if( !_filename.isEmpty() )
        filename = _filename;

    struct SceneProperty sceneProperty;

    // Step 1: Create data model
    // The model holds all nodes and connections. It uses the shared registry
    // to instantiate node delegate models when loading/creating nodes.
    sceneProperty.pDataFlowGraphModel = new PBDataFlowGraphModel(pDataModelRegistry);
    sceneProperty.sFilename = filename;

    // Step 2: Create graphics scene
    // The scene takes a REFERENCE to the model (model must outlive scene)
    // MainWindow is parent for Qt ownership (will delete scene in destructor)
    sceneProperty.pDataFlowGraphicsScene = new PBDataFlowGraphicsScene(*sceneProperty.pDataFlowGraphModel, this);
    
    // Install custom node geometry to enable minimize functionality
    // This uses placement new to replace the default NodeGeometry with PBNodeGeometry
    sceneProperty.pDataFlowGraphicsScene->installCustomGeometry();

    // Step 3: Create view widget
    // The view provides user interaction (panning, zooming, drag-drop)
    sceneProperty.pFlowGraphicsView = new PBFlowGraphicsView(sceneProperty.pDataFlowGraphicsScene);

    // Add view to tab widget (transfers ownership to QTabWidget)
    QFileInfo file(filename);
    int tabIndex = ui->mpTabWidget->addTab(static_cast<QWidget*>(sceneProperty.pFlowGraphicsView), file.completeBaseName() );

    // Use local variables for signal connections (NOT member pointers)
    // These are safe to use here because the objects won't be deleted during this function
    PBDataFlowGraphModel* model = sceneProperty.pDataFlowGraphModel;
    PBDataFlowGraphicsScene* scene = sceneProperty.pDataFlowGraphicsScene;

    // Connect model lifecycle signals to update UI
    connect( model, &PBDataFlowGraphModel::nodeCreated, this, &MainWindow::nodeCreated );
    connect( model, &PBDataFlowGraphModel::nodeDeleted, this, &MainWindow::nodeDeleted );
    // Connect group lifecycle signals so MainWindow can update Workspace tree
    connect( model, &PBDataFlowGraphModel::groupCreated, this, &MainWindow::groupCreated );
    connect( model, &PBDataFlowGraphModel::groupDissolved, this, &MainWindow::groupDissolved );

    // Setup undo/redo integration
    QUndoStack* undoStack = &scene->undoStack();

    // Connect undo stack signals to update menu actions
    connect( undoStack, &QUndoStack::canUndoChanged, ui->mpActionUndo, &QAction::setEnabled );
    connect( undoStack, &QUndoStack::canRedoChanged, ui->mpActionRedo, &QAction::setEnabled );

    // Mark scene as modified when undo stack index changes (user makes edits)
    connect( undoStack, &QUndoStack::indexChanged, this, &MainWindow::nodeChanged );

    // Ensure the view and scene repaint after any undo/redo operation. Some
    // undo commands (paste -> undo) may remove model items which can leave
    // visual artefacts due to device/background caching. Force a viewport
    // update when the undo stack index changes to clear stale pixmaps.
    connect( undoStack, &QUndoStack::indexChanged, sceneProperty.pFlowGraphicsView, [sceneProperty]() {
        PBFlowGraphicsView* v = sceneProperty.pFlowGraphicsView;
        PBDataFlowGraphicsScene* s = sceneProperty.pDataFlowGraphicsScene;
        if (v && v->viewport())
            v->viewport()->update();
        if (s)
            s->update();
    });

    // Initialize undo/redo action states
    ui->mpActionUndo->setEnabled(undoStack->canUndo());
    ui->mpActionRedo->setEnabled(undoStack->canRedo());

    // Selection changed signal comes from the graphics scene
    connect( scene, &QGraphicsScene::selectionChanged, this, &MainWindow::nodeInSceneSelectionChanged );

    mlSceneProperty.push_back( sceneProperty );
    mitSceneProperty = std::prev(mlSceneProperty.end());

    ui->mpTabWidget->setCurrentIndex( tabIndex );

    // Apply snap-to-grid setting to the newly created scene
    // This ensures new scenes inherit the current snap-to-grid state
    scene->setSnapToGrid(ui->mpActionSnapToGrid->isChecked());
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
            actionSave_slot();
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

    // Disconnect from selected node if any
    auto* selectedNodeDelegateModel = getSelectedNodeDelegateModel();
    if( selectedNodeDelegateModel )
    {
        selectedNodeDelegateModel->setSelected( false );// TODO: This should not be called explicitly. It could have done in NodeGraphicsObject class.
        disconnect(selectedNodeDelegateModel, nullptr, this, nullptr);
    }
    
    // if there is only one page and it's closing, just close it and add empty Untitle scene.
    if( ui->mpTabWidget->count() == 1 )
    {
        if( !mbClossingApp )
        {
            createScene( "", mpDelegateModelRegistry );
            ui->mpTabWidget->removeTab( 0 );
        }
        // Delete in proper order: view -> scene -> model
        delete mlSceneProperty.front().pFlowGraphicsView;
        delete mlSceneProperty.front().pDataFlowGraphicsScene;
        delete mlSceneProperty.front().pDataFlowGraphModel;
        mlSceneProperty.pop_front();
    }
    else
    {
        PBFlowGraphicsView * pPage2bClosed = static_cast<PBFlowGraphicsView*>( ui->mpTabWidget->widget( index ) );
        ui->mpTabWidget->removeTab( index );
        for( std::list<struct SceneProperty>::iterator it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it )
        {
            if( it->pFlowGraphicsView == pPage2bClosed )
            {
                // Delete in proper order: view -> scene -> model
                delete it->pFlowGraphicsView;
                delete it->pDataFlowGraphicsScene;
                delete it->pDataFlowGraphModel;
                mlSceneProperty.erase( it );
                break;
            }
        }
        // Update iterator to current page
        PBFlowGraphicsView * pCurrentPage = static_cast<PBFlowGraphicsView*>( ui->mpTabWidget->currentWidget() );
        for( std::list<struct SceneProperty>::iterator it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it )
        {
            if( it->pFlowGraphicsView == pCurrentPage )
            {
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
    // If this item represents a group (we store GroupId in UserRole+1), handle it
    if (item->data(0, Qt::UserRole + 1).isValid()) {
        GroupId gid = static_cast<GroupId>( item->data(0, Qt::UserRole + 1).toUInt() );
        PBDataFlowGraphicsScene* scene = getCurrentScene();
        PBFlowGraphicsView* view = getCurrentView();
        if (!scene || !view)
            return;
        if (auto* groupItem = scene->getGroupGraphicsItem(gid)) {
            view->clearSelection();
            groupItem->setSelected(true);
        }
        return;
    }

    if( item->columnCount() == 2 )
    {
        PBFlowGraphicsView* view = getCurrentView();
        if (!view)
            return;

        view->clearSelection();
        auto nodeId = item->text(1).toInt();
        auto nodeGraphicsObject = mMapNodeIdToNodeGraphicsObject[ nodeId ];

        if (nodeGraphicsObject)
            nodeGraphicsObject->setSelected( true );
    }
}

void
MainWindow::
nodeListDoubleClicked( QTreeWidgetItem * item, int )
{
    // If this item is a group, center the view on the group's bounding box
    if (item->data(0, Qt::UserRole + 1).isValid()) {
        GroupId gid = static_cast<GroupId>( item->data(0, Qt::UserRole + 1).toUInt() );
        PBDataFlowGraphicsScene* scene = getCurrentScene();
        PBFlowGraphicsView* view = getCurrentView();
        if (!scene || !view)
            return;
        if (auto* groupItem = scene->getGroupGraphicsItem(gid)) {
            view->center_on( groupItem->sceneBoundingRect().center() );
        }
        return;
    }

    if( item->columnCount() == 2 )
    {
        PBFlowGraphicsView* view = getCurrentView();
        if (!view)
            return;
        
        auto nodeId = item->text( 1 ).toInt();
        view->center_on( nodeId );
    }
}

void
MainWindow::
nodeListContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem* item = ui->mpNodeListTreeView->itemAt(pos);
    if (!item)
        return;

    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    PBDataFlowGraphModel* model = getCurrentModel();
    if (!view || !scene || !model)
        return;

    QMenu menu;
    QAction* copyAction = menu.addAction("Copy");
    copyAction->setIcon(QIcon(":/icons/tango/16x16/edit-copy.png"));
    copyAction->setIconVisibleInMenu(true);

    QAction* cutAction = menu.addAction("Cut");
    cutAction->setIcon(QIcon(":/icons/tango/16x16/edit-cut.png"));
    cutAction->setIconVisibleInMenu(true);

    menu.addSeparator();

    QAction* deleteAction = menu.addAction("Delete");
    deleteAction->setIcon(QIcon(":/icons/tango/16x16/edit-delete.png"));
    deleteAction->setIconVisibleInMenu(true);

    QAction* selected = menu.exec(ui->mpNodeListTreeView->viewport()->mapToGlobal(pos));
    if (!selected)
        return;

    // If the item represents a group (we stored GroupId in UserRole+1)
    if (item->data(0, Qt::UserRole + 1).isValid()) {
        GroupId gid = static_cast<GroupId>( item->data(0, Qt::UserRole + 1).toUInt() );
        if (auto* groupItem = scene->getGroupGraphicsItem(gid)) {
            view->clearSelection();
            groupItem->setSelected(true);
        }
    }
    else if (item->columnCount() >= 2) {
        // Treat as node entry - second column is NodeId
        bool ok = false;
        int nodeId = item->text(1).toInt(&ok);
        if (ok && mMapNodeIdToNodeGraphicsObject.contains(nodeId)) {
            view->clearSelection();
            auto nodeGraphicsObject = mMapNodeIdToNodeGraphicsObject[nodeId];
            if (nodeGraphicsObject)
                nodeGraphicsObject->setSelected(true);
        }
    }

    // Delegate to view-level handlers which perform clipboard/undo-aware operations
    if (selected == copyAction) {
        view->triggerCopy();
    } else if (selected == cutAction) {
        view->triggerCut();
    } else if (selected == deleteAction) {
        view->triggerDelete();
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
actionSave_slot()
{
    if( !mitSceneProperty->sFilename.isEmpty() && mitSceneProperty->sFilename != QString("Untitle.flow") )
    {
        PBDataFlowGraphModel* model = getCurrentModel();
        if (model)
            model->save_to_file( mitSceneProperty->sFilename );

        // Mark undo stack as clean (this will remove the * from tab title)
        if (mitSceneProperty->pDataFlowGraphicsScene)
        {
            mitSceneProperty->pDataFlowGraphicsScene->undoStack().setClean();
        }

        QFileInfo file( mitSceneProperty->sFilename );
        ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), file.completeBaseName() );
    }
    else
        actionSaveAs_slot();
}

void
MainWindow::
actionLoad_slot()
{
    QString filename;

    if( QFileInfo::exists(msSettingFilename) )
    {
        QSettings settings(msSettingFilename, QSettings::IniFormat);
        QString flowPath = settings.value("Flow Directory", QDir::homePath()).toString();
        filename = QFileDialog::getOpenFileName(this,
                                   tr( "Open Flow Scene" ),
                                   flowPath,
                                   tr( "Flow Scene Files (*.flow)" ));
    }
    else
    {
        filename = QFileDialog::getOpenFileName(this,
                                   tr( "Open Flow Scene" ),
                                   QDir::homePath(),
                                   tr( "Flow Scene Files (*.flow)" ));
    }
    if( filename.isEmpty() )
        return;
    if( QFileInfo::exists(msSettingFilename) )
    {
        QSettings settings(msSettingFilename, QSettings::IniFormat);
        QString flowPath = settings.value("Flow Directory", QDir::homePath()).toString();
        if( flowPath != QFileInfo(filename).absolutePath() )
        {
            settings.setValue("Flow Directory", QFileInfo(filename).absolutePath());
        }
    }
    loadScene(filename);
}

void
MainWindow::
actionQuit_slot()
{
    close();
}

void
MainWindow::
actionLoadPlugin_slot()
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
    load_plugin( mpDelegateModelRegistry, mPluginsList, filename );
    updateNodeCategoriesDockingWidget();
}

void
MainWindow::
closeEvent(QCloseEvent *ev)
{
    mbClossingApp = true;
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
actionSaveAs_slot()
{
    QString filename;
    if( QFileInfo::exists(msSettingFilename) )
    {
        QSettings settings(msSettingFilename, QSettings::IniFormat);
        QString flowPath = settings.value("Flow Directory", QDir::homePath()).toString();
        filename = QFileDialog::getSaveFileName(this,
                                   tr( "Save the Flow Scene to" ),
                                   flowPath + "/Untitle.flow",
                                   tr( "Flow Scene Files (*.flow)" ));
    }
    else
    {
        filename = QFileDialog::getSaveFileName(this,
                                   tr( "Save the Flow Scene to" ),
                                   QDir::homePath() + "/Untitle.flow",
                                   tr( "Flow Scene Files (*.flow)" ));
    }

    if( !filename.isEmpty() )
    {
        if( !filename.endsWith( "flow", Qt::CaseInsensitive ) )
            filename += ".flow";
            
        PBDataFlowGraphModel* model = getCurrentModel();
        if( model && model->save_to_file(filename) )
        {
            mitSceneProperty->sFilename = filename;

            // Mark undo stack as clean (this will remove the * from tab title)
            if (mitSceneProperty->pDataFlowGraphicsScene)
            {
                mitSceneProperty->pDataFlowGraphicsScene->undoStack().setClean();
            }

            QFileInfo file(filename);
            ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), file.completeBaseName() );
        }
        if( QFileInfo::exists(msSettingFilename) )
        {
            QSettings settings(msSettingFilename, QSettings::IniFormat);
            QString flowPath = settings.value("Flow Directory", QDir::homePath()).toString();
            if( flowPath != QFileInfo(filename).absolutePath() )
            {
                settings.setValue("Flow Directory", QFileInfo(filename).absolutePath());
            }
        }
    }
}

void
MainWindow::
actionSceneOnly_slot()
{
    ui->mpAvailableNodeCategoryDockWidget->hide();
    ui->mpNodeListDockWidget->hide();
    ui->mpPropertyBrowserDockWidget->hide();
}

void
MainWindow::
actionAllPanels_slot()
{
    ui->mpAvailableNodeCategoryDockWidget->show();
    ui->mpNodeListDockWidget->show();
    ui->mpPropertyBrowserDockWidget->show();
}

void
MainWindow::
actionZoomReset_slot()
{
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphModel* model = getCurrentModel();
    
    if (!view || !model)
        return;
    
    // Reset the zoom transformation
    view->resetTransform();
    
    // Get all nodes in the current scene
    auto nodes = model->allNodeIds();
    
    // If there are nodes, center the view on them
    if (nodes.size() > 0)
    {
        auto left_pos = std::numeric_limits<double>::max();
        auto right_pos = std::numeric_limits<double>::min();
        auto top_pos = std::numeric_limits<double>::max();
        auto bottom_pos = std::numeric_limits<double>::min();

        for (auto nodeId : nodes)
        {
            auto nodeGraphicsObject = mMapNodeIdToNodeGraphicsObject[nodeId];
            if (nodeGraphicsObject)
            {
                auto nodeRect = nodeGraphicsObject->sceneBoundingRect();
                if (nodeRect.x() < left_pos)
                    left_pos = nodeRect.x();
                if (nodeRect.y() < top_pos)
                    top_pos = nodeRect.y();
                if (nodeRect.x() + nodeRect.width() > right_pos)
                    right_pos = nodeRect.x() + nodeRect.width();
                if (nodeRect.y() + nodeRect.height() > bottom_pos)
                    bottom_pos = nodeRect.y() + nodeRect.height();
            }
        }
        
        // Calculate the center position of all nodes
        auto center_pos = QPointF((left_pos + right_pos) * 0.5, (top_pos + bottom_pos) * 0.5);
        
        // Center the view on this position
        view->center_on(center_pos);
    }
}

void
MainWindow::
actionNew_slot()
{
    // TODO NodeEditor v3 Migration: addAnchor was a custom method for saving view position.
    // This feature needs to be reimplemented in v3 using view transformation storage.
    // COMMENTED OUT: addAnchor() custom method not available in NodeEditor v3
    // if( mpFlowView )
    //     mpFlowView->addAnchor( 10 ); // Keep the current sceneRect.
    createScene( "", mpDelegateModelRegistry );
}

// actionCopy_slot removed - Copy is handled by view-local action and MainWindow menu proxy

void
MainWindow::
actionUndo_slot()
{
    // Use QUndoStack in the graphics scene
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    if (scene)
        scene->undoStack().undo();
}

void
MainWindow::
actionRedo_slot()
{
    // Use QUndoStack in the graphics scene
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    if (scene)
    {
        scene->undoStack().redo();
    }
}

void
MainWindow::
enable_all_nodes(bool enable)
{
    PBDataFlowGraphModel* model = getCurrentModel();
    if (!model)
        return;

    PBDataFlowGraphicsScene* scene = getCurrentScene();
    if (!scene)
        return;

    auto nodeIds = model->allNodeIds();
    
    // Use macro to group all enable/disable operations into single undo step
    scene->undoStack().beginMacro(enable ? tr("Enable All Nodes") : tr("Disable All Nodes"));
    
    for( auto nodeId : nodeIds )
    {
        auto* delegateModel = mMapNodeIdToNodeDelegateModel[nodeId];
        if (!delegateModel)
            continue;
            
        // Get current enable state
        QVariant oldValue = delegateModel->isEnable();
        QVariant newValue = enable;
        
        // Only create command if state is actually changing
        if (oldValue.toBool() != newValue.toBool())
        {
            auto* cmd = new PropertyChangeCommand(scene,
                                                  nodeId,
                                                  delegateModel,
                                                  "enable",
                                                  oldValue,
                                                  newValue);
            scene->undoStack().push(cmd);
        }
    }
    
    scene->undoStack().endMacro();
}

void
MainWindow::
actionEnableAll_slot()
{
    enable_all_nodes( true );
}

void
MainWindow::
actionDisableAll_slot()
{
    enable_all_nodes( false );
}

void
MainWindow::
actionSnapToGrid_slot(bool checked)
{
    // Set snap to grid for the current scene
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    if (scene) {
        scene->setSnapToGrid(checked);
    }
    
    // Also update all scenes in the list
    for (auto& sceneProperty : mlSceneProperty) {
        if (sceneProperty.pDataFlowGraphicsScene) {
            sceneProperty.pDataFlowGraphicsScene->setSnapToGrid(checked);
        }
    }
}

void
MainWindow::
actionFocusView_slot(bool checked)
{
    PBDataFlowGraphModel* model = getCurrentModel();
    PBFlowGraphicsView* view = getCurrentView();
    if (!model || !view)
        return;
        
    if( checked )
    {
        auto nodeIds = model->allNodeIds();
        for( auto nodeId : nodeIds )
        {
            if( ! mMapNodeIdToNodeDelegateModel[ nodeId ]->embeddedWidget() )
                mMapNodeIdToNodeGraphicsObject[ nodeId ]->hide();
            else
            {
                mMapNodeIdToNodeDelegateModel[ nodeId ]->setDrawConnectionPoints( false );
                mMapNodeIdToNodeGraphicsObject[ nodeId ]->update();
            }

            auto connectionIds = model->allConnectionIds( nodeId );
            view->showConnections(connectionIds, false);
        }

        ui->mpAvailableNodeCategoryDockWidget->hide();
        ui->mpNodeListDockWidget->hide();
        ui->mpPropertyBrowserDockWidget->hide();
        ui->mpToolBar->hide();
        ui->mpStatusBar->hide();

        ui->mpTabWidget->setTabsClosable(false);
    }
    else
    {
        auto nodeIds = model->allNodeIds();
        for( auto nodeId : nodeIds )
        {
            if( ! mMapNodeIdToNodeDelegateModel[ nodeId ]->embeddedWidget() )
                mMapNodeIdToNodeGraphicsObject[ nodeId ]->show();
            else
            {
                mMapNodeIdToNodeDelegateModel[ nodeId ]->setDrawConnectionPoints( true );
                mMapNodeIdToNodeGraphicsObject[ nodeId ]->update();
            }

            auto connectionIds = model->allConnectionIds( nodeId );
            view->showConnections(connectionIds, true);
        }

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
actionFullScreen_slot(bool checked)
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

    // Clear node tree and group tree for the current view
    while( !mMapNodeIdToNodeGraphicsObject.empty() )
    {
        NodeId nodeId = mMapNodeIdToNodeGraphicsObject.begin().key();
        removeFromNodeTree( nodeId );
    }

    // Clear any existing group entries
    if (mGroupRootItem) {
        delete mGroupRootItem;
        mGroupRootItem = nullptr;
    }
    mMapGroupIdToNodeTreeWidgetItem.clear();

    // Update iterator to current page
    for( auto it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it )
    {
        if( it->pFlowGraphicsView == static_cast<PBFlowGraphicsView*>(ui->mpTabWidget->currentWidget()) )
        {
            mitSceneProperty = it;
            break;
        }
    }

    PBDataFlowGraphModel* model = getCurrentModel();
    if (model)
    {
        auto nodeIds = model->allNodeIds();
        for( NodeId nodeId : nodeIds )
            addToNodeTree( nodeId );

        // Add existing groups into the Workspace tree
        for (const auto &pair : model->groups()) {
            addToGroupTree(pair.first);
        }
    }

    nodeInSceneSelectionChanged();

    // Update undo/redo action states for the current tab
    if (mitSceneProperty != mlSceneProperty.end() && mitSceneProperty->pDataFlowGraphicsScene)
    {
        QUndoStack* undoStack = &mitSceneProperty->pDataFlowGraphicsScene->undoStack();
        ui->mpActionUndo->setEnabled(undoStack->canUndo());
        ui->mpActionRedo->setEnabled(undoStack->canRedo());
    }
    else
    {
        ui->mpActionUndo->setEnabled(false);
        ui->mpActionRedo->setEnabled(false);
    }
}

void
MainWindow::
nodeChanged()
{
    // Check if the undo stack is clean (no unsaved changes)
    bool isClean = false;
    if (mitSceneProperty != mlSceneProperty.end() && mitSceneProperty->pDataFlowGraphicsScene)
    {
        isClean = mitSceneProperty->pDataFlowGraphicsScene->undoStack().isClean();
    }

    QString tabTitle = ui->mpTabWidget->tabText( ui->mpTabWidget->currentIndex() );

    if (isClean)
    {
        // Remove * if undo stack is clean (back to saved state)
        if (tabTitle.length() > 0 && tabTitle.at(0) == QChar('*'))
        {
            tabTitle = tabTitle.mid(1); // Remove the first character (*)
            ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), tabTitle );
        }
    }
    else
    {
        // Add * if not already present
        if( tabTitle.length() != 0 && tabTitle.at(0) != QChar('*') )
        {
            tabTitle = "*" + tabTitle;
            ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), tabTitle );
        }
    }
}

void
MainWindow::
loadSettings()
{
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir configDir(homePath + "/.CVDev");
    if( !configDir.exists() )
        configDir.mkpath(".");

    msSettingFilename = configDir.filePath("cvdev.ini");
    if( QFileInfo::exists(msSettingFilename) )
    {
        QSettings settings(msSettingFilename, QSettings::IniFormat);
        
        // Try to load all previously open scenes
        QStringList openScenes = settings.value("Open Scenes", QStringList()).toStringList();
        if (!openScenes.isEmpty())
        {
            // Load each scene that still exists
            for (const QString& filename : openScenes)
            {
                if (QFileInfo::exists(filename))
                {
                    loadScene(const_cast<QString&>(filename));
                }
            }
            
            // Restore the active tab
            int activeTab = settings.value("Active Tab", 0).toInt();
            if (activeTab >= 0 && activeTab < ui->mpTabWidget->count())
            {
                ui->mpTabWidget->setCurrentIndex(activeTab);
            }
        }
        else
        {
            // Fallback to old single scene format for backward compatibility
            auto filename = settings.value("Open Scene", "").toString();
            if( QFileInfo::exists(filename) )
                loadScene(filename);
        }
        
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
    
    // Save all open scene filenames
    QStringList openScenes;
    for (auto it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it)
    {
        if (!it->sFilename.isEmpty() && QFileInfo::exists(it->sFilename))
        {
            openScenes.append(it->sFilename);
        }
    }
    settings.setValue("Open Scenes", openScenes);
    
    // Save current active tab index
    settings.setValue("Active Tab", ui->mpTabWidget->currentIndex());
    
    // Keep backward compatibility - save current scene as well
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
    // Check if this file is already open in another tab
    int tabIndex = 0;
    for (auto it = mlSceneProperty.begin(); it != mlSceneProperty.end(); ++it, ++tabIndex)
    {
        if (it->sFilename == filename)
        {
            // File is already open, just switch to that tab
            ui->mpTabWidget->setCurrentIndex(tabIndex);
            return;
        }
    }
    
    PBDataFlowGraphModel* model = getCurrentModel();
    if (!model)
        return;
        
    if( model->allNodeIds().size() != 0 )
    {
        createScene( filename, mpDelegateModelRegistry );
        // Get the new model after creating the scene
        model = getCurrentModel();
    }
    else
        mitSceneProperty->sFilename = filename;
    if( model->load_from_file( filename ) )
    {
        QFileInfo file(filename);
        ui->mpTabWidget->setTabText( ui->mpTabWidget->currentIndex(), file.completeBaseName() );
        // Mark the undo stack as clean after loading the file
        if (mitSceneProperty != mlSceneProperty.end() && mitSceneProperty->pDataFlowGraphicsScene)
            mitSceneProperty->pDataFlowGraphicsScene->undoStack().setClean();
        
        // Center the view on all loaded nodes
        PBFlowGraphicsView* view = getCurrentView();
        auto nodes = model->allNodeIds();
        if( view && nodes.size() > 0 )
        {
            auto left_pos = std::numeric_limits<double>::max();
            auto right_pos = std::numeric_limits<double>::min();
            auto top_pos = std::numeric_limits<double>::max();
            auto bottom_pos = std::numeric_limits<double>::min();

            for( auto nodeId : nodes )
            {
                auto nodeGraphicsObject = mMapNodeIdToNodeGraphicsObject[nodeId];
                if (nodeGraphicsObject)
                {
                    auto nodeRect = nodeGraphicsObject->sceneBoundingRect();
                    if( nodeRect.x() < left_pos )
                        left_pos = nodeRect.x();
                    if( nodeRect.y() < top_pos )
                        top_pos = nodeRect.y();
                    if( nodeRect.x() + nodeRect.width() > right_pos )
                        right_pos = nodeRect.x() + nodeRect.width();
                    if( nodeRect.y() + nodeRect.height() > bottom_pos )
                        bottom_pos = nodeRect.y() + nodeRect.height();
                }
            }
            auto center_pos = QPointF((left_pos+right_pos)*0.5, (top_pos+bottom_pos)*0.5);
            view->center_on( center_pos );
        }

            if (mitSceneProperty != mlSceneProperty.end() && mitSceneProperty->pDataFlowGraphicsScene)
            {
                mitSceneProperty->pDataFlowGraphicsScene->updateAllGroupVisuals();
            }
    }
    else
    {
        closeScene( ui->mpTabWidget->currentIndex() );
    }
}

void MainWindow::
actionAbout_slot()
{
    QMessageBox::about(this, msProgramName, "<p>" + msProgramName + " has been designed and developped as a software tool so that "
                                           "developers can reuse their codes and share their work with others. If you have any comment please "
                                           "feel free to contact <a href=mailto:pished.bunnun@nectec.or.th>pished.bunnun@nectec.or.th</a>.</p>"
                                           "<p>Copyright (C) 2025 <a href=www.nectec.or.th>NECTEC</a> All rights reserved.</p>"
                                           "<p>" + msProgramName + " is made possible by open source softwares.</p>");
}

// ============================================================================
// Helper Methods: Dynamic Query Pattern
// ============================================================================
// These methods implement the "query on demand" pattern instead of caching
// pointers to the current scene's components. This design prevents stale
// pointer issues that can occur when:
// - Tabs are switched (different scene becomes active)
// - Scenes are closed (cached pointers become dangling)
// - Undo/redo operations modify the scene structure
//
// The tab widget (ui->mpTabWidget) is the single source of truth for which
// scene is currently active. All queries start from currentWidget() and
// navigate the object hierarchy: Widget -> View -> Scene -> Model
// ============================================================================

SelectedNodeResult MainWindow::getSelectedNodeId() const
{
    PBFlowGraphicsView* view = getCurrentView();
    if (!view)
        return {false, 0};  // No valid tab/view
    
    auto selectedNodeIds = view->selectedNodes();
    // Only return valid result if exactly one node is selected
    // Multiple selection or no selection returns hasSelection=false
    if (selectedNodeIds.size() == 1)
        return {true, selectedNodeIds[0]};  // Could be NodeId(0) - now unambiguous!
    else
        return {false, 0};  // No selection or multiple selection
}

PBNodeDelegateModel* MainWindow::getSelectedNodeDelegateModel() const
{
    PBDataFlowGraphModel* model = getCurrentModel();
    if (!model)
        return nullptr;  // No valid scene/model
    
    auto result = getSelectedNodeId();
    if (!result.hasSelection)
        return nullptr;  // No valid single selection
    
    return model->delegateModel<PBNodeDelegateModel>(result.nodeId);
}

PBFlowGraphicsView* MainWindow::getCurrentView() const
{
    // The tab widget's current widget IS the view (added in createScene)
    QWidget* currentWidget = ui->mpTabWidget->currentWidget();
    return currentWidget ? static_cast<PBFlowGraphicsView*>(currentWidget) : nullptr;
}

PBDataFlowGraphicsScene* MainWindow::getCurrentScene() const
{
    // Each view owns exactly one scene (set in view constructor)
    PBFlowGraphicsView* view = getCurrentView();
    return view ? static_cast<PBDataFlowGraphicsScene*>(view->scene()) : nullptr;
}

PBDataFlowGraphModel* MainWindow::getCurrentModel() const
{
    // Each scene references exactly one model (set in scene constructor)
    // The model is owned by SceneProperty and outlives the scene
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    return scene ? static_cast<PBDataFlowGraphModel*>(&scene->graphModel()) : nullptr;
}

// ========== Grouping Action Slots ==========

void
MainWindow::
actionGroupSelectedNodes_slot()
{
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphModel* model = getCurrentModel();
    PBDataFlowGraphicsScene* scene = getCurrentScene();
    
    if (!view || !model || !scene) {
        return;
    }
    
    // Get selected nodes
    auto selectedNodes = view->selectedNodes();
    if (selectedNodes.empty()) {
        QMessageBox::information(this, "Group Nodes", 
                                "Please select at least one node to group.");
        return;
    }
    
    // Prompt for group name
    bool ok;
    QString groupName = QInputDialog::getText(this, "Group Nodes",
                                             "Enter group name:",
                                             QLineEdit::Normal,
                                             "New Group", &ok);
    if (!ok || groupName.isEmpty()) {
        return;
    }
    
    // Convert vector to set
    std::set<NodeId> nodeSet(selectedNodes.begin(), selectedNodes.end());
    
    // Create group
    auto* cmd = new GroupCreateCommand(scene, model, groupName, nodeSet);
    scene->undoStack().push(cmd);
    statusBar()->showMessage(QString("Created group '%1' with %2 nodes")
                            .arg(groupName)
                            .arg(selectedNodes.size()), 3000);
}

void
MainWindow::
actionUngroupSelectedNodes_slot()
{
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphModel* model = getCurrentModel();
    
    if (!view || !model) {
        return;
    }
    
    GroupId groupId = InvalidGroupId;
    
    // First, check if a group graphics item is selected
    auto* scene = dynamic_cast<PBDataFlowGraphicsScene*>(view->scene());
    if (scene) {
        auto selectedItems = scene->selectedItems();
        for (auto* item : selectedItems) {
            if (auto* groupItem = qgraphicsitem_cast<PBNodeGroupGraphicsItem*>(item)) {
                groupId = groupItem->groupId();
                break;
            }
        }
    }
    
    // If no group item selected, try to find group from selected nodes
    if (groupId == InvalidGroupId) {
        auto selectedNodes = view->selectedNodes();
        if (selectedNodes.empty()) {
            QMessageBox::information(this, "Ungroup Nodes",
                                    "Please select a group or a node in the group to ungroup.");
            return;
        }
        
        // Find the group containing the first selected node
        groupId = model->getPBNodeGroup(selectedNodes[0]);
        if (groupId == InvalidGroupId) {
            QMessageBox::information(this, "Ungroup Nodes",
                                    "Selected node is not in a group.");
            return;
        }
    }
    
    // Get group name for confirmation
    const PBNodeGroup* group = model->getGroup(groupId);
    QString groupName = group ? group->name() : QString("Group %1").arg(groupId);
    
    // Confirm dissolution
    auto reply = QMessageBox::question(this, "Ungroup Nodes",
                                      QString("Dissolve group '%1'?").arg(groupName),
                                      QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (scene) {
            scene->undoStack().push(new GroupDissolveCommand(scene, model, *group));
        }
        statusBar()->showMessage(QString("Dissolved group '%1'").arg(groupName), 3000);
    }
}

void
MainWindow::
actionRenameGroup_slot()
{
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphModel* model = getCurrentModel();
    
    if (!view || !model) {
        return;
    }
    
    // Get selected nodes
    auto selectedNodes = view->selectedNodes();
    if (selectedNodes.empty()) {
        QMessageBox::information(this, "Rename Group",
                                "Please select a node in the group to rename.");
        return;
    }
    
    // Find the group containing the first selected node
    GroupId groupId = model->getPBNodeGroup(selectedNodes[0]);
    if (groupId == InvalidGroupId) {
        QMessageBox::information(this, "Rename Group",
                                "Selected node is not in a group.");
        return;
    }
    
    const PBNodeGroup* group = model->getGroup(groupId);
    QString currentName = group ? group->name() : "Group";
    
    // Prompt for new name
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Group",
                                           "Enter new group name:",
                                           QLineEdit::Normal,
                                           currentName, &ok);
    
    if (ok && !newName.isEmpty() && newName != currentName) {
        model->setGroupName(groupId, newName);
        statusBar()->showMessage(QString("Renamed group to '%1'").arg(newName), 3000);
    }
}

void
MainWindow::
actionChangeGroupColor_slot()
{
    PBFlowGraphicsView* view = getCurrentView();
    PBDataFlowGraphModel* model = getCurrentModel();
    
    if (!view || !model) {
        return;
    }
    
    // Get selected nodes
    auto selectedNodes = view->selectedNodes();
    if (selectedNodes.empty()) {
        QMessageBox::information(this, "Change Group Color",
                                "Please select a node in the group.");
        return;
    }
    
    // Find the group containing the first selected node
    GroupId groupId = model->getPBNodeGroup(selectedNodes[0]);
    if (groupId == InvalidGroupId) {
        QMessageBox::information(this, "Change Group Color",
                                "Selected node is not in a group.");
        return;
    }
    
    const PBNodeGroup* group = model->getGroup(groupId);
    QColor currentColor = group ? group->color() : QColor(100, 150, 200, 80);
    
    // Open color dialog with alpha channel support
    QColor newColor = QColorDialog::getColor(currentColor, this,
                                            "Choose Group Color",
                                            QColorDialog::ShowAlphaChannel);
    
    if (newColor.isValid() && newColor != currentColor) {
        model->setGroupColor(groupId, newColor);
        statusBar()->showMessage("Changed group color", 3000);
    }
}
