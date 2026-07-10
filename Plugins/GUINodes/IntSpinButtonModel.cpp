//Copyright © 2026, NECTEC, all rights reserved

#include "IntSpinButtonModel.hpp"
#include <QtGui/QPainter>
#include <QtCore/QJsonValue>
#include <QtCore/QDebug>

const QString IntSpinButtonModel::_category = "GUI";
const QString IntSpinButtonModel::_model_name = "Integer Spin Button";

IntSpinButtonModel::IntSpinButtonModel()
    : PBNodeDelegateModel(_model_name, true),
      mpEmbeddedWidget(new QSpinBox(qobject_cast<QWidget*>(this))),
      mpIntData(std::make_shared<IntegerData>(miValue))
{
    // Configure spinbox properties
    mpEmbeddedWidget->setMinimum(miMin);
    mpEmbeddedWidget->setMaximum(miMax);
    mpEmbeddedWidget->setSingleStep(miStep);
    mpEmbeddedWidget->setValue(miValue);

    // Apply custom styling
    mpEmbeddedWidget->setStyleSheet(
        "QSpinBox { "
        "  background-color: #ffffff; "
        "  border: 1px solid #8f8f91; "
        "  border-radius: 4px; "
        "  color: black; "
        "  padding: 2px; "
        "}"
        "QSpinBox:disabled { "
        "  background-color: #d0d0d0; "
        "  color: #808080; "
        "}"
    );

    // Connect value change signal
    connect(mpEmbeddedWidget, QOverload<int>::of(&QSpinBox::valueChanged), this, &IntSpinButtonModel::em_value_changed);

    // Properties Setup for Property Browser
    QString propId = "value";
    IntPropertyType intValProp;
    intValProp.miMin = miMin;
    intValProp.miMax = miMax;
    intValProp.miValue = miValue;
    auto propValue = std::make_shared<TypedProperty<IntPropertyType>>("Value", propId, QMetaType::Int, intValProp);
    mvProperty.push_back(propValue);
    mMapIdToProperty[propId] = propValue;

    propId = "min";
    IntPropertyType intMinProp;
    intMinProp.miMin = -999999;
    intMinProp.miMax = 999999;
    intMinProp.miValue = miMin;
    auto propMin = std::make_shared<TypedProperty<IntPropertyType>>("Minimum", propId, QMetaType::Int, intMinProp);
    mvProperty.push_back(propMin);
    mMapIdToProperty[propId] = propMin;

    propId = "max";
    IntPropertyType intMaxProp;
    intMaxProp.miMin = -999999;
    intMaxProp.miMax = 999999;
    intMaxProp.miValue = miMax;
    auto propMax = std::make_shared<TypedProperty<IntPropertyType>>("Maximum", propId, QMetaType::Int, intMaxProp);
    mvProperty.push_back(propMax);
    mMapIdToProperty[propId] = propMax;

    propId = "step";
    IntPropertyType intStepProp;
    intStepProp.miMin = 1;
    intStepProp.miMax = 999999;
    intStepProp.miValue = miStep;
    auto propStep = std::make_shared<TypedProperty<IntPropertyType>>("Step", propId, QMetaType::Int, intStepProp);
    mvProperty.push_back(propStep);
    mMapIdToProperty[propId] = propStep;

    propId = "fontsize";
    IntPropertyType intFontSizeProp;
    intFontSizeProp.miMin = 6;
    intFontSizeProp.miMax = 72;
    intFontSizeProp.miValue = miFontSize;
    auto propFontSize = std::make_shared<TypedProperty<IntPropertyType>>("Font Size", propId, QMetaType::Int, intFontSizeProp);
    mvProperty.push_back(propFontSize);
    mMapIdToProperty[propId] = propFontSize;

    // Minimization Pixmap Drawing
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(100, 100, 100));
    painter.setPen(QPen(QColor(50, 50, 50), 2));
    painter.drawRoundedRect(2, 4, 20, 16, 4, 4);
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    painter.drawLine(15, 4, 15, 20);
    _minPixmap = pix;

    // Default sizing
    mpEmbeddedWidget->setMinimumSize(80, 26);
    mpEmbeddedWidget->resize(100, 30);
}

unsigned int IntSpinButtonModel::nPorts(PortType portType) const
{
    switch (portType) {
    case PortType::In:
        return 0;
    case PortType::Out:
        return 1;
    default:
        return 0;
    }
}

NodeDataType IntSpinButtonModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out && portIndex == 0) {
        return IntegerData().type();
    }
    return NodeDataType();
}

QString IntSpinButtonModel::portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::Out && portIndex == 0) {
        return "Integer Output: The current integer value of the spin box.";
    }
    return PBNodeDelegateModel::portToolTip(portType, portIndex);
}

std::shared_ptr<NodeData> IntSpinButtonModel::outData(PortIndex portIndex)
{
    if (isEnable() && portIndex == 0) {
        return mpIntData;
    }
    return nullptr;
}

void IntSpinButtonModel::setInData(std::shared_ptr<NodeData>, PortIndex)
{
    // No input ports
}

QJsonObject IntSpinButtonModel::save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["value"] = miValue;
    cParams["min"] = miMin;
    cParams["max"] = miMax;
    cParams["step"] = miStep;
    cParams["fontsize"] = miFontSize;
    modelJson["cParams"] = cParams;
    return modelJson;
}

void IntSpinButtonModel::load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        QJsonValue v = paramsObj["min"];
        if (!v.isNull()) {
            miMin = v.toInt();
            auto prop = mMapIdToProperty["min"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = miMin;
            mpEmbeddedWidget->setMinimum(miMin);
        }
        v = paramsObj["max"];
        if (!v.isNull()) {
            miMax = v.toInt();
            auto prop = mMapIdToProperty["max"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = miMax;
            mpEmbeddedWidget->setMaximum(miMax);
        }
        v = paramsObj["step"];
        if (!v.isNull()) {
            miStep = v.toInt();
            auto prop = mMapIdToProperty["step"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = miStep;
            mpEmbeddedWidget->setSingleStep(miStep);
        }
        v = paramsObj["value"];
        if (!v.isNull()) {
            miValue = v.toInt();
            auto prop = mMapIdToProperty["value"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = miValue;
            mpIntData->data() = miValue;
            mpEmbeddedWidget->setValue(miValue);
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

        // Sync value property bounds
        auto valProp = mMapIdToProperty["value"];
        if (valProp) {
            auto typedValProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(valProp);
            typedValProp->getData().miMin = miMin;
            typedValProp->getData().miMax = miMax;
        }
    }
    mpEmbeddedWidget->setEnabled(isEnable());
}

void IntSpinButtonModel::setModelProperty(QString &id, const QVariant &value)
{
    PBNodeDelegateModel::setModelProperty(id, value);

    if (!mMapIdToProperty.contains(id))
        return;

    auto prop = mMapIdToProperty[id];
    if (id == "value") {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miValue = value.toInt();
        mpIntData->data() = miValue;
        if (mpEmbeddedWidget->value() != miValue) {
            mpEmbeddedWidget->blockSignals(true);
            mpEmbeddedWidget->setValue(miValue);
            mpEmbeddedWidget->blockSignals(false);
        }
        updateAllOutputPorts();
    } else if (id == "min") {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miMin = value.toInt();
        mpEmbeddedWidget->setMinimum(miMin);
        auto valProp = mMapIdToProperty["value"];
        if (valProp) {
            auto typedValProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(valProp);
            typedValProp->getData().miMin = miMin;
        }
    } else if (id == "max") {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miMax = value.toInt();
        mpEmbeddedWidget->setMaximum(miMax);
        auto valProp = mMapIdToProperty["value"];
        if (valProp) {
            auto typedValProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(valProp);
            typedValProp->getData().miMax = miMax;
        }
    } else if (id == "step") {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miStep = value.toInt();
        mpEmbeddedWidget->setSingleStep(miStep);
    } else if (id == "fontsize") {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miFontSize = value.toInt();
        QFont font = mpEmbeddedWidget->font();
        font.setPointSize(miFontSize);
        mpEmbeddedWidget->setFont(font);
    }
}

void IntSpinButtonModel::enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed(enable);
    mpEmbeddedWidget->setEnabled(enable);
}

void IntSpinButtonModel::em_value_changed(int val)
{
    if (!checkSelectionForInteraction()) {
        mpEmbeddedWidget->blockSignals(true);
        mpEmbeddedWidget->setValue(miValue);
        mpEmbeddedWidget->blockSignals(false);
        Q_EMIT selection_request_signal();
        return;
    }
    requestPropertyChange("value", val);
}
