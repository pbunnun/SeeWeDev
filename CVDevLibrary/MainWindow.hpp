#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once

#include "CVDevLibrary.hpp"

#include <FlowScene>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include "PBNodeDataModel.hpp"
#include "PBFlowScene.hpp"
#include "PBFlowView.hpp"
#include "qtpropertymanager.h"
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QtVariantProperty;
class QtProperty;
using QtNodes::Node;
using QtNodes::DataModelRegistry;

struct SceneProperty
{
    QString sFilename;
    PBFlowScene * pFlowScene;
    PBFlowView  * pFlowView;
};

class CVDEVSHAREDLIB_EXPORT MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow( QWidget *parent = nullptr );
    ~MainWindow();

private slots:
    void editorPropertyChanged( QtProperty *property, const QVariant &value );
    void nodePropertyChanged( std::shared_ptr<Property> );
    void nodeCreated( Node & );
    void nodeDeleted( Node & );
    void nodeInSceneSelectionChanged();
    void nodeListClicked( QTreeWidgetItem *, int );
    void nodeListDoubleClicked( QTreeWidgetItem *, int );
    void tabPageChanged( int );
    void nodeChanged();

    void on_mpActionNew_triggered();
    void on_mpActionSave_triggered();
    void on_mpActionLoad_triggered();
    void on_mpActionQuit_triggered();
    void on_mpActionSaveAs_triggered();

    void on_mpActionSceneOnly_triggered();
    void on_mpActionAllPanels_triggered();
    void on_mpActionZoomReset_triggered();

    void on_mpActionCopy_triggered();
    void on_mpActionCut_triggered();
    void on_mpActionPaste_triggered();
    void on_mpActionDelete_triggered();

    void on_mpActionUndo_triggered();
    void on_mpActionRedo_triggered();

    void on_mpActionDisableAll_triggered();
    void on_mpActionEnableAll_triggered();

    void on_mpActionSnapToGrid_toggled(bool);

    void on_mpActionLoadPlugin_triggered();

    void on_mpActionFocusView_toggled(bool);

    void on_mpActionFullScreen_toggled(bool);

    void on_mpActionAbout_triggered();
protected:
    void closeEvent(QCloseEvent *);

private:
    void setupPropertyBrowserDockingWidget();

    void setupNodeCategoriesDockingWidget();
    void updateNodeCategoriesDockingWidget();

    void setupNodeListDockingWidget();
    void loadScene(QString &);
    void createScene(QString const & _filename, std::shared_ptr<DataModelRegistry> & pDataModelRegistry);
    bool closeScene( int );
    void addToNodeTree( Node & );
    void removeFromNodeTree( Node & );

    void loadSettings();
    void saveSettings();

    Ui::MainWindow *ui;
    std::shared_ptr<DataModelRegistry> mpDataModelRegistry;
    std::list<struct SceneProperty> mlSceneProperty;
    std::list<struct SceneProperty>::iterator mitSceneProperty;

    PBFlowScene * mpFlowScene{ nullptr };
    PBFlowView * mpFlowView{ nullptr };

    QtNodes::Node * mpSelectedNode{ nullptr };
    PBNodeDataModel * mpSelectedNodeDataModel{ nullptr };

    QMap< QString, QTreeWidgetItem* > mMapModelCategoryToNodeTreeWidgetItem;

    QMap< QString, QTreeWidgetItem* > mMapModelNameToNodeTreeWidgetItem;

    QMap< QString, QTreeWidgetItem* > mMapNodeIDToNodeTreeWidgetItem;
    QMap< QString, Node* > mMapNodeIDToNode;
    //////////////////////////////////////////////////////////
    /// \brief mpVariantManager for Node Property Editor
    //////////////////////////////////////////////////////////
    class QtVariantPropertyManager * mpVariantManager;
    class QtTreePropertyBrowser * mpPropertyEditor;
    QMap< QtProperty *, QString > mMapQtPropertyToPropertyId;
    QMap< QString, QtProperty * > mMapPropertyIdToQtProperty;
    QMap< QString, bool > mMapPropertyIdToExpanded;
    QList< QtGroupPropertyManager * > mListGroupPropertyManager;

    QString msSettingFilename;
    const QString msProgramName{ "CVDev" };

    void updatePropertyExpandState();
    void addProperty( QtVariantProperty *property, const QString & prop_id, const QString & sub_text );
    void clearPropertyBrowser();
};
#endif // MAINWINDOW_H
