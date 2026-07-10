//Copyright © 2026, NECTEC, all rights reserved

#include "TableViewModel.hpp"
#include <QPainter>
#include <QRegularExpression>
#include <QDebug>
#include "qtvariantproperty_p.h"

// ==========================================
// TableEmbeddedWidget Implementation
// ==========================================

TableEmbeddedWidget::TableEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      mTableWidget(new QTableWidget(this)),
      mColumnsSpinBox(new QSpinBox(this)),
      mSaveButton(new QPushButton("Save", this)),
      mClearButton(new QPushButton("Clear", this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Controls top layout
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(6);

    QLabel *colLabel = new QLabel("Columns:", this);
    controlsLayout->addWidget(colLabel);

    mColumnsSpinBox->setRange(1, 50);
    mColumnsSpinBox->setValue(3);
    controlsLayout->addWidget(mColumnsSpinBox);

    controlsLayout->addStretch();

    controlsLayout->addWidget(mSaveButton);
    controlsLayout->addWidget(mClearButton);

    mainLayout->addLayout(controlsLayout);

    // Configure Table Widget
    mTableWidget->setRowCount(0);
    mTableWidget->setColumnCount(3);
    mTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTableWidget->setAlternatingRowColors(true);
    mTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Initialize default header items
    for (int i = 0; i < 3; ++i) {
        mTableWidget->setHorizontalHeaderItem(i, new QTableWidgetItem(QString("Col %1").arg(i + 1)));
    }

    mainLayout->addWidget(mTableWidget);

    // Size Policy
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Connections
    connect(mColumnsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TableEmbeddedWidget::onSpinBoxChanged);
    connect(mTableWidget->horizontalHeader(), &QHeaderView::sectionDoubleClicked, this, &TableEmbeddedWidget::onHeaderDoubleClicked);
    connect(mSaveButton, &QPushButton::clicked, this, &TableEmbeddedWidget::savePressed);
    connect(mClearButton, &QPushButton::clicked, this, &TableEmbeddedWidget::clearPressed);
}

int TableEmbeddedWidget::columnCount() const
{
    return mTableWidget->columnCount();
}

void TableEmbeddedWidget::setColumnCount(int count)
{
    int current = mTableWidget->columnCount();
    if (current == count) return;

    mTableWidget->setColumnCount(count);
    mColumnsSpinBox->blockSignals(true);
    mColumnsSpinBox->setValue(count);
    mColumnsSpinBox->blockSignals(false);

    // Set default headers for any added columns
    for (int i = current; i < count; ++i) {
        mTableWidget->setHorizontalHeaderItem(i, new QTableWidgetItem(QString("Col %1").arg(i + 1)));
    }
}

QStringList TableEmbeddedWidget::headers() const
{
    QStringList list;
    for (int i = 0; i < mTableWidget->columnCount(); ++i) {
        QTableWidgetItem *item = mTableWidget->horizontalHeaderItem(i);
        list << (item ? item->text() : QString("Col %1").arg(i + 1));
    }
    return list;
}

void TableEmbeddedWidget::setHeaders(const QStringList& headers)
{
    for (int i = 0; i < headers.size() && i < mTableWidget->columnCount(); ++i) {
        mTableWidget->setHorizontalHeaderItem(i, new QTableWidgetItem(headers.at(i)));
    }
}

int TableEmbeddedWidget::rowCount() const
{
    return mTableWidget->rowCount();
}

QTableWidgetItem* TableEmbeddedWidget::item(int row, int column) const
{
    return mTableWidget->item(row, column);
}

void TableEmbeddedWidget::addRow(const QStringList& rowData)
{
    int row = mTableWidget->rowCount();
    mTableWidget->insertRow(row);
    for (int col = 0; col < rowData.size() && col < mTableWidget->columnCount(); ++col) {
        mTableWidget->setItem(row, col, new QTableWidgetItem(rowData.at(col)));
    }
}

void TableEmbeddedWidget::clearTableRows()
{
    mTableWidget->setRowCount(0);
}

QList<QStringList> TableEmbeddedWidget::tableData() const
{
    QList<QStringList> data;
    for (int r = 0; r < mTableWidget->rowCount(); ++r) {
        QStringList row;
        for (int c = 0; c < mTableWidget->columnCount(); ++c) {
            QTableWidgetItem *it = mTableWidget->item(r, c);
            row << (it ? it->text() : "");
        }
        data << row;
    }
    return data;
}

void TableEmbeddedWidget::setTableData(const QList<QStringList>& data)
{
    mTableWidget->setRowCount(0);
    for (const QStringList& row : data) {
        addRow(row);
    }
}

void TableEmbeddedWidget::onHeaderDoubleClicked(int logicalIndex)
{
    QTableWidgetItem *headerItem = mTableWidget->horizontalHeaderItem(logicalIndex);
    QString currentText = headerItem ? headerItem->text() : QString("Col %1").arg(logicalIndex + 1);
    bool ok;
    QString newText = QInputDialog::getText(this, "Edit Column Header",
                                           QString("Enter header for Column %1:").arg(logicalIndex + 1),
                                           QLineEdit::Normal, currentText, &ok);
    if (ok && !newText.isEmpty()) {
        mTableWidget->setHorizontalHeaderItem(logicalIndex, new QTableWidgetItem(newText));
        Q_EMIT headersChanged(headers());
    }
}

void TableEmbeddedWidget::onSpinBoxChanged(int val)
{
    setColumnCount(val);
    Q_EMIT columnCountChanged(val);
    Q_EMIT headersChanged(headers());
}


// ==========================================
// TableViewModel Implementation
// ==========================================

const QString TableViewModel::_category = QString("GUI");
const QString TableViewModel::_model_name = QString("Table View");

TableViewModel::TableViewModel()
    : PBNodeDelegateModel(_model_name),
      mpEmbeddedWidget(new TableEmbeddedWidget(qobject_cast<QWidget*>(this)))
{
    // Programmatic minimized icon
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(120, 120, 120), 1.5));
    painter.drawRect(2, 2, 20, 20);
    painter.drawLine(2, 8, 22, 8);
    painter.drawLine(2, 14, 22, 14);
    painter.drawLine(8, 2, 8, 22);
    painter.drawLine(15, 2, 15, 22);
    _minPixmap = pix;

    // Minimum size to prevent rendering issues in node graphics
    mpEmbeddedWidget->setMinimumSize(180, 120);
    mpEmbeddedWidget->resize(250, 200);

    // Connections
    connect(mpEmbeddedWidget, &TableEmbeddedWidget::columnCountChanged, this, &TableViewModel::onWidgetColumnCountChanged);
    connect(mpEmbeddedWidget, &TableEmbeddedWidget::headersChanged, this, &TableViewModel::onWidgetHeadersChanged);
    connect(mpEmbeddedWidget, &TableEmbeddedWidget::savePressed, this, &TableViewModel::onWidgetSavePressed);
    connect(mpEmbeddedWidget, &TableEmbeddedWidget::clearPressed, this, &TableViewModel::onWidgetClearPressed);

    // Property: Dirname (save directory path)
    PathPropertyType pathPropertyType;
    pathPropertyType.msPath = msDirname;
    QString propId = "dirname";
    auto propDirName = std::make_shared<TypedProperty<PathPropertyType>>(
        "Dirname", propId, QtVariantPropertyManager::pathTypeId(), pathPropertyType
    );
    mvProperty.push_back(propDirName);
    mMapIdToProperty[propId] = propDirName;

    // Property: Prefix Filename
    propId = "prefix_filename";
    auto propPrefix = std::make_shared<TypedProperty<QString>>(
        "Prefix Filename", propId, QMetaType::QString, msPrefixFilename
    );
    mvProperty.push_back(propPrefix);
    mMapIdToProperty[propId] = propPrefix;

    // Property: Max Rows
    IntPropertyType intPropertyType;
    intPropertyType.miMax = 100000;
    intPropertyType.miMin = 1;
    intPropertyType.miValue = miMaxRows;
    propId = "max_rows";
    auto propMaxRows = std::make_shared<TypedProperty<IntPropertyType>>(
        "Max Rows", propId, QMetaType::Int, intPropertyType
    );
    mvProperty.push_back(propMaxRows);
    mMapIdToProperty[propId] = propMaxRows;
}

TableViewModel::~TableViewModel()
{
    saveToCsv();
}

QJsonObject TableViewModel::save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["dirname"] = msDirname;
    cParams["prefix_filename"] = msPrefixFilename;
    cParams["max_rows"] = miMaxRows;
    cParams["column_count"] = mpEmbeddedWidget->columnCount();

    QJsonArray headersArray;
    for (const QString& header : mpEmbeddedWidget->headers()) {
        headersArray.append(header);
    }
    cParams["headers"] = headersArray;

    QJsonArray rowsArray;
    for (const QStringList& row : mpEmbeddedWidget->tableData()) {
        QJsonArray rowArray;
        for (const QString& val : row) {
            rowArray.append(val);
        }
        rowsArray.append(rowArray);
    }
    cParams["rows"] = rowsArray;
    cParams["first_row_datetime"] = msFirstRowDateTime;

    modelJson["cParams"] = cParams;
    return modelJson;
}

void TableViewModel::load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        if (paramsObj.contains("dirname")) {
            msDirname = paramsObj["dirname"].toString();
            auto prop = mMapIdToProperty["dirname"];
            if (prop) {
                auto typedProp = std::static_pointer_cast<TypedProperty<PathPropertyType>>(prop);
                typedProp->getData().msPath = msDirname;
            }
        }
        if (paramsObj.contains("prefix_filename")) {
            msPrefixFilename = paramsObj["prefix_filename"].toString();
            auto prop = mMapIdToProperty["prefix_filename"];
            if (prop) {
                auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
                typedProp->getData() = msPrefixFilename;
            }
        }
        if (paramsObj.contains("max_rows")) {
            miMaxRows = paramsObj["max_rows"].toInt();
            auto prop = mMapIdToProperty["max_rows"];
            if (prop) {
                auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
                typedProp->getData().miValue = miMaxRows;
            }
        }
        int colCount = 3;
        if (paramsObj.contains("column_count")) {
            colCount = paramsObj["column_count"].toInt();
            mpEmbeddedWidget->setColumnCount(colCount);
        }
        if (paramsObj.contains("headers")) {
            QJsonArray headersArray = paramsObj["headers"].toArray();
            QStringList headersList;
            for (int i = 0; i < headersArray.size(); ++i) {
                headersList << headersArray.at(i).toString();
            }
            mpEmbeddedWidget->setHeaders(headersList);
        }
        if (paramsObj.contains("rows")) {
            QJsonArray rowsArray = paramsObj["rows"].toArray();
            QList<QStringList> dataList;
            for (int i = 0; i < rowsArray.size(); ++i) {
                QJsonArray rowArray = rowsArray.at(i).toArray();
                QStringList rowList;
                for (int j = 0; j < rowArray.size(); ++j) {
                    rowList << rowArray.at(j).toString();
                }
                dataList << rowList;
            }
            mpEmbeddedWidget->setTableData(dataList);
        }
        if (paramsObj.contains("first_row_datetime")) {
            msFirstRowDateTime = paramsObj["first_row_datetime"].toString();
        }
    }
}

unsigned int TableViewModel::nPorts(PortType portType) const
{
    return (portType == PortType::In) ? 1 : 0;
}

NodeDataType TableViewModel::dataType(PortType, PortIndex) const
{
    return InformationData().type();
}

QString TableViewModel::portToolTip(PortType portType, PortIndex) const
{
    if (portType == PortType::In) {
        return "Row Data (CSV String, Numbers Vector)";
    }
    return "";
}

void TableViewModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (!isEnable()) return;
    if (!nodeData) return;

    QStringList rowData;
    if (!parseIncomingRow(nodeData, rowData)) {
        return; // Conversion or extraction failed
    }

    int expectedCols = mpEmbeddedWidget->columnCount();
    if (rowData.size() != expectedCols) {
        static bool sWarningActive = false;
        if (!sWarningActive) {
            sWarningActive = true;
            QMessageBox::warning(nullptr, "Data Mismatch",
                                 QString("Incoming row data contains %1 items, but the table expects %2 columns.\nData ignored.")
                                 .arg(rowData.size()).arg(expectedCols));
            sWarningActive = false;
        }
        return;
    }

    // Set first row date-time when row count becomes 1 (i.e. currently 0)
    if (mpEmbeddedWidget->rowCount() == 0 && msFirstRowDateTime.isEmpty()) {
        msFirstRowDateTime = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    }

    mpEmbeddedWidget->addRow(rowData);

    // Trigger save and reset if capacity is reached
    if (mpEmbeddedWidget->rowCount() >= miMaxRows) {
        saveAndReset();
    }
}

bool TableViewModel::parseIncomingRow(std::shared_ptr<NodeData> nodeData, QStringList& outRow)
{
    // 1. StdStringData
    auto strData = std::dynamic_pointer_cast<StdStringData>(nodeData);
    if (strData) {
        QString rawStr = QString::fromStdString(strData->data()).trimmed();
        QStringList rawList = rawStr.split(QRegularExpression("[,;\\t]"));
        for (const QString& item : rawList) {
            outRow << item.trimmed();
        }
        return true;
    }

    // 2. Generic InformationData
    auto infoData = std::dynamic_pointer_cast<InformationData>(nodeData);
    if (infoData) {
        QString rawStr = infoData->info().trimmed();
        // Remove formatting header if present
        if (rawStr.startsWith("Data Type : std::string")) {
            int firstNewline = rawStr.indexOf('\n');
            if (firstNewline != -1) {
                rawStr = rawStr.mid(firstNewline + 1).trimmed();
            }
        }
        QStringList rawList = rawStr.split(QRegularExpression("[,;\\t]"));
        for (const QString& item : rawList) {
            outRow << item.trimmed();
        }
        return true;
    }

    // 3. StdVectorIntData
    auto intVecData = std::dynamic_pointer_cast<StdVectorIntData>(nodeData);
    if (intVecData) {
        for (int val : intVecData->data()) {
            outRow << QString::number(val);
        }
        return true;
    }

    // 4. StdVectorFloatData
    auto floatVecData = std::dynamic_pointer_cast<StdVectorFloatData>(nodeData);
    if (floatVecData) {
        for (float val : floatVecData->data()) {
            outRow << QString::number(val);
        }
        return true;
    }

    // 5. StdVectorDoubleData
    auto doubleVecData = std::dynamic_pointer_cast<StdVectorDoubleData>(nodeData);
    if (doubleVecData) {
        for (double val : doubleVecData->data()) {
            outRow << QString::number(val);
        }
        return true;
    }

    return false;
}

void TableViewModel::saveToCsv()
{
    if (msDirname.isEmpty()) {
        qWarning() << "[TableViewModel] Cannot save CSV: directory path is empty.";
        return;
    }

    int rowCount = mpEmbeddedWidget->rowCount();
    int colCount = mpEmbeddedWidget->columnCount();
    if (rowCount == 0 || colCount == 0) {
        qInfo() << "[TableViewModel] Table is empty, skipping CSV save.";
        return;
    }

    QDir dir(msDirname);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "[TableViewModel] Failed to create directory:" << msDirname;
            return;
        }
    }

    if (msFirstRowDateTime.isEmpty()) {
        msFirstRowDateTime = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    }

    QString filename = msPrefixFilename + msFirstRowDateTime + ".csv";
    QString filepath = dir.filePath(filename);

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[TableViewModel] Failed to open CSV file for writing:" << filepath;
        return;
    }

    QTextStream out(&file);

    // Headers
    QStringList headers = mpEmbeddedWidget->headers();
    for (int col = 0; col < colCount; ++col) {
        QString header = headers.value(col);
        if (header.contains(",") || header.contains("\"") || header.contains("\n")) {
            header.replace("\"", "\"\"");
            header = "\"" + header + "\"";
        }
        out << header << (col == colCount - 1 ? "" : ",");
    }
    out << "\n";

    // Data rows
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < colCount; ++col) {
            QTableWidgetItem *item = mpEmbeddedWidget->item(row, col);
            QString text = item ? item->text() : "";
            if (text.contains(",") || text.contains("\"") || text.contains("\n")) {
                text.replace("\"", "\"\"");
                text = "\"" + text + "\"";
            }
            out << text << (col == colCount - 1 ? "" : ",");
        }
        out << "\n";
    }

    file.close();
    qInfo() << "[TableViewModel] Successfully saved CSV to:" << filepath;
}

void TableViewModel::saveAndReset()
{
    saveToCsv();
    mpEmbeddedWidget->clearTableRows();
    msFirstRowDateTime = "";
}

void TableViewModel::clearAndReset()
{
    mpEmbeddedWidget->clearTableRows();
    msFirstRowDateTime = "";
}

void TableViewModel::onWidgetSavePressed()
{
    saveAndReset();
}

void TableViewModel::onWidgetClearPressed()
{
    clearAndReset();
}

void TableViewModel::onWidgetColumnCountChanged(int)
{
    // Graph/layout update if required
}

void TableViewModel::onWidgetHeadersChanged(const QStringList&)
{
    // Graph/layout update if required
}

void TableViewModel::setModelProperty(QString &id, const QVariant &value)
{
    PBNodeDelegateModel::setModelProperty(id, value);
    if (!mMapIdToProperty.contains(id)) return;

    if (id == "dirname") {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<PathPropertyType>>(prop);
        typedProp->getData().msPath = value.toString();
        msDirname = value.toString();
    }
    else if (id == "prefix_filename") {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
        typedProp->getData() = value.toString();
        msPrefixFilename = value.toString();
    }
    else if (id == "max_rows") {
        auto prop = mMapIdToProperty[id];
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miMaxRows = value.toInt();
    }
}

void TableViewModel::enable_changed(bool)
{
    // State change handler
}
