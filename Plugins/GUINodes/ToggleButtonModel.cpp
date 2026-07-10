//Copyright © 2026, NECTEC, all rights reserved

#include "ToggleButtonModel.hpp"
#include <QPainter>
#include <QVariant>

const QString ToggleButtonModel::_category = QString("GUI");
const QString ToggleButtonModel::_model_name = QString("Toggle Button");

ToggleButtonModel::ToggleButtonModel()
    : PBNodeDelegateModel(_model_name),
      mpEmbeddedWidget(new QPushButton(qobject_cast<QWidget*>(this)))
{
    // Configure checkable properties and styling
    mpEmbeddedWidget->setCheckable(true);
    mpEmbeddedWidget->setText(msLabel);
    
    mpEmbeddedWidget->setStyleSheet(
        "QPushButton { "
        "  background-color: #f44336; "
        "  border: 1px solid #d32f2f; "
        "  border-radius: 4px; "
        "  color: white; "
        "  padding: 4px; "
        "}"
        "QPushButton:disabled { "
        "  background-color: #d0d0d0; "
        "  color: #808080; "
        "}"
        "QPushButton:checked { "
        "  background-color: #4CAF50; "
        "  color: white; "
        "  border: 1px solid #3e8e41; "
        "}"
    );

    QFont font = mpEmbeddedWidget->font();
    font.setPointSize(miFontSize);
    mpEmbeddedWidget->setFont(font);

    connect(mpEmbeddedWidget, &QPushButton::clicked, this, &ToggleButtonModel::em_button_clicked);

    mpSyncData = std::make_shared<SyncData>(mbState);
    mpBoolData = std::make_shared<BoolData>(mbState);

    // Label property setup
    auto propLabel = std::make_shared<TypedProperty<QString>>("Label", "label", QMetaType::QString, msLabel);
    mvProperty.push_back(propLabel);
    mMapIdToProperty["label"] = propLabel;

    // Font size property setup
    IntPropertyType intPropertyType;
    intPropertyType.miMax = 72;
    intPropertyType.miMin = 6;
    intPropertyType.miValue = miFontSize;
    auto propFontSize = std::make_shared<TypedProperty<IntPropertyType>>("Font Size", "fontsize", QMetaType::Int, intPropertyType);
    mvProperty.push_back(propFontSize);
    mMapIdToProperty["fontsize"] = propFontSize;

    // State property setup
    auto propState = std::make_shared<TypedProperty<bool>>("State", "state", QMetaType::Bool, mbState);
    mvProperty.push_back(propState);
    mMapIdToProperty["state"] = propState;

    // Minimization Icon drawing
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(100, 100, 100));
    painter.setPen(QPen(QColor(50, 50, 50), 2));
    painter.drawRoundedRect(2, 4, 20, 16, 4, 4);
    painter.setBrush(QColor(220, 220, 220));
    painter.drawEllipse(4, 6, 12, 12);
    _minPixmap = pix;

    // Size initialization
    mpEmbeddedWidget->setMinimumSize(60, 30);
    mpEmbeddedWidget->resize(100, 40);
}

unsigned int ToggleButtonModel::nPorts(PortType portType) const
{
    switch (portType) {
    case PortType::In:
        return 0;
    case PortType::Out:
        return 2;
    default:
        return 0;
    }
}

NodeDataType ToggleButtonModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out) {
        if (portIndex == 0)
            return SyncData().type();
        else if (portIndex == 1)
            return BoolData().type();
    }
    return NodeDataType();
}

QString ToggleButtonModel::portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::Out) {
        if (portIndex == 0)
            return "State Trigger: Sync data pulse emitted when the toggle changes state.";
        else if (portIndex == 1)
            return "Toggle State: Boolean output representing checked (true) or unchecked (false) state.";
    }
    return PBNodeDelegateModel::portToolTip(portType, portIndex);
}

std::shared_ptr<NodeData> ToggleButtonModel::outData(PortIndex portIndex)
{
    if (isEnable()) {
        if (portIndex == 0)
            return mpSyncData;
        else if (portIndex == 1)
            return mpBoolData;
    }
    return nullptr;
}

QJsonObject ToggleButtonModel::save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["label"] = msLabel;
    cParams["fontsize"] = miFontSize;
    cParams["state"] = mbState;
    modelJson["cParams"] = cParams;
    return modelJson;
}

void ToggleButtonModel::load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        QJsonValue v = paramsObj["label"];
        if (!v.isNull()) {
            msLabel = v.toString();
            auto prop = mMapIdToProperty["label"];
            auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
            typedProp->getData() = msLabel;
            mpEmbeddedWidget->setText(msLabel);
        }
        v = paramsObj["fontsize"];
        if (!v.isNull()) {
            miFontSize = v.toInt();
            auto prop = mMapIdToProperty["fontsize"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = miFontSize;
            QFont font = mpEmbeddedWidget->font();
            font.setPointSize(miFontSize);
            mpEmbeddedWidget->setFont(font);
        }
        v = paramsObj["state"];
        if (!v.isNull()) {
            mbState = v.toBool();
            auto prop = mMapIdToProperty["state"];
            auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
            typedProp->getData() = mbState;
            mpEmbeddedWidget->setChecked(mbState);
            mpBoolData->data() = mbState;
            mpSyncData->data() = mbState;
        }
    }
    mpEmbeddedWidget->setEnabled(isEnable());
}

void ToggleButtonModel::setModelProperty(QString &id, const QVariant &value)
{
    PBNodeDelegateModel::setModelProperty(id, value);

    if (!mMapIdToProperty.contains(id))
        return;

    auto prop = mMapIdToProperty[id];
    if (id == "label") {
        auto typedProp = std::static_pointer_cast<TypedProperty<QString>>(prop);
        typedProp->getData() = value.toString();
        msLabel = value.toString();
        mpEmbeddedWidget->setText(msLabel);
    } else if (id == "fontsize") {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miFontSize = value.toInt();
        QFont font = mpEmbeddedWidget->font();
        font.setPointSize(miFontSize);
        mpEmbeddedWidget->setFont(font);
    } else if (id == "state") {
        auto typedProp = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typedProp->getData() = value.toBool();
        mbState = value.toBool();
        mpEmbeddedWidget->setChecked(mbState);
        mpBoolData->data() = mbState;
        mpSyncData->data() = mbState;
        updateAllOutputPorts();
    }
}

void ToggleButtonModel::enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed(enable);
    mpEmbeddedWidget->setEnabled(enable);
}

void ToggleButtonModel::em_button_clicked()
{
    if (!checkSelectionForInteraction()) {
        // Revert the visual toggle change and request node selection instead
        mpEmbeddedWidget->blockSignals(true);
        mpEmbeddedWidget->setChecked(!mpEmbeddedWidget->isChecked());
        mpEmbeddedWidget->blockSignals(false);
        Q_EMIT selection_request_signal();
        return;
    }
    // Update check state via the undo command framework
    requestPropertyChange("state", mpEmbeddedWidget->isChecked());
}

void ToggleButtonModel::setInData(std::shared_ptr<NodeData>, PortIndex)
{
    // No input ports to handle
}
