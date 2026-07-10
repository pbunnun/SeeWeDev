//Copyright © 2026, NECTEC, all rights reserved

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"
#include "StdStringData.hpp"
#include "StdVectorNumberData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

class TableEmbeddedWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TableEmbeddedWidget(QWidget *parent = nullptr);
    virtual ~TableEmbeddedWidget() override = default;

    int columnCount() const;
    void setColumnCount(int count);

    QStringList headers() const;
    void setHeaders(const QStringList& headers);

    int rowCount() const;
    QTableWidgetItem* item(int row, int column) const;

    void addRow(const QStringList& rowData);
    void clearTableRows();

    QList<QStringList> tableData() const;
    void setTableData(const QList<QStringList>& data);

Q_SIGNALS:
    void columnCountChanged(int count);
    void headersChanged(const QStringList& headers);
    void savePressed();
    void clearPressed();

private Q_SLOTS:
    void onHeaderDoubleClicked(int logicalIndex);
    void onSpinBoxChanged(int val);

private:
    QTableWidget *mTableWidget;
    QSpinBox *mColumnsSpinBox;
    QPushButton *mSaveButton;
    QPushButton *mClearButton;
};

class TableViewModel : public PBNodeDelegateModel
{
    Q_OBJECT
public:
    TableViewModel();
    virtual ~TableViewModel() override;

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;

    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    QString portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;

    QWidget *embeddedWidget() override { return mpEmbeddedWidget; }

    QPixmap minPixmap() const override { return _minPixmap; }

    static const QString _category;
    static const QString _model_name;

    void setModelProperty(QString &id, const QVariant &value) override;

    void saveToCsv();
    void saveAndReset();
    void clearAndReset();

private Q_SLOTS:
    void enable_changed(bool enable) override;
    void onWidgetSavePressed();
    void onWidgetClearPressed();
    void onWidgetColumnCountChanged(int count);
    void onWidgetHeadersChanged(const QStringList& headers);

private:
    TableEmbeddedWidget *mpEmbeddedWidget;
    QPixmap _minPixmap;

    QString msDirname{""};
    QString msPrefixFilename{"table_data_"};
    int miMaxRows{100};
    QString msFirstRowDateTime{""};

    // Helper to extract a list of strings from incoming node data
    bool parseIncomingRow(std::shared_ptr<NodeData> nodeData, QStringList& outRow);
};
