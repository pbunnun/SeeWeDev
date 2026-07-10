//Copyright © 2026, NECTEC, all rights reserved

#include "FloatSpinButtonModel.hpp"
#include <QtGui/QPainter>
#include <QtCore/QJsonValue>
#include <QtCore/QDebug>

const QString FloatSpinButtonModel::_category = "GUI";
const QString FloatSpinButtonModel::_model_name = "Float Spin Button";

FloatSpinButtonModel::FloatSpinButtonModel()
    : PBNodeDelegateModel(_model_name, true),
      mpEmbeddedWidget(new QDoubleSpinBox(qobject_cast<QWidget*>(this))),
      mpFloatData(std::make_shared<FloatData>(static_cast<float>(mdValue)))
{
    // Configure double spinbox properties
    mpEmbeddedWidget->setMinimum(mdMin);
    mpEmbeddedWidget->setMaximum(mdMax);
    mpEmbeddedWidget->setSingleStep(mdStep);
    mpEmbeddedWidget->setDecimals(miDecimals);
    mpEmbeddedWidget->setValue(mdValue);

    // Apply custom styling
    mpEmbeddedWidget->setStyleSheet(
        "QDoubleSpinBox { "
        "  background-color: #ffffff; "
        "  border: 1px solid #8f8f91; "
        "  border-radius: 4px; "
        "  color: black; "
        "  padding: 2px; "
        "}"
        "QDoubleSpinBox:disabled { "
        "  background-color: #d0d0d0; "
        "  color: #808080; "
        "}"
    );

    // Connect value change signal
    connect(mpEmbeddedWidget, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FloatSpinButtonModel::em_value_changed);

    // Properties Setup for Property Browser
    QString propId = "value";
    DoublePropertyType dblValProp;
    dblValProp.mdMin = mdMin;
    dblValProp.mdMax = mdMax;
    dblValProp.mdValue = mdValue;
    auto propValue = std::make_shared<TypedProperty<DoublePropertyType>>("Value", propId, QMetaType::Double, dblValProp);
    mvProperty.push_back(propValue);
    mMapIdToProperty[propId] = propValue;

    propId = "min";
    DoublePropertyType dblMinProp;
    dblMinProp.mdMin = -999999.0;
    dblMinProp.mdMax = 999999.0;
    dblMinProp.mdValue = mdMin;
    auto propMin = std::make_shared<TypedProperty<DoublePropertyType>>("Minimum", propId, QMetaType::Double, dblMinProp);
    mvProperty.push_back(propMin);
    mMapIdToProperty[propId] = propMin;

    propId = "max";
    DoublePropertyType dblMaxProp;
    dblMaxProp.mdMin = -999999.0;
    dblMaxProp.mdMax = 999999.0;
    dblMaxProp.mdValue = mdMax;
    auto propMax = std::make_shared<TypedProperty<DoublePropertyType>>("Maximum", propId, QMetaType::Double, dblMaxProp);
    mvProperty.push_back(propMax);
    mMapIdToProperty[propId] = propMax;

    propId = "step";
    DoublePropertyType dblStepProp;
    dblStepProp.mdMin = 0.0001;
    dblStepProp.mdMax = 999999.0;
    dblStepProp.mdValue = mdStep;
    auto propStep = std::make_shared<TypedProperty<DoublePropertyType>>("Step", propId, QMetaType::Double, dblStepProp);
    mvProperty.push_back(propStep);
    mMapIdToProperty[propId] = propStep;

    propId = "decimals";
    IntPropertyType intDecimalsProp;
    intDecimalsProp.miMin = 0;
    intDecimalsProp.miMax = 10;
    intDecimalsProp.miValue = miDecimals;
    auto propDecimals = std::make_shared<TypedProperty<IntPropertyType>>("Decimals", propId, QMetaType::Int, intDecimalsProp);
    mvProperty.push_back(propDecimals);
    mMapIdToProperty[propId] = propDecimals;

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

unsigned int FloatSpinButtonModel::nPorts(PortType portType) const
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

NodeDataType FloatSpinButtonModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out && portIndex == 0) {
        return FloatData().type();
    }
    return NodeDataType();
}

QString FloatSpinButtonModel::portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::Out && portIndex == 0) {
        return "Float Output: The current floating-point value of the spin box.";
    }
    return PBNodeDelegateModel::portToolTip(portType, portIndex);
}

std::shared_ptr<NodeData> FloatSpinButtonModel::outData(PortIndex portIndex)
{
    if (isEnable() && portIndex == 0) {
        return mpFloatData;
    }
    return nullptr;
}

void FloatSpinButtonModel::setInData(std::shared_ptr<NodeData>, PortIndex)
{
    // No input ports
}

QJsonObject FloatSpinButtonModel::save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["value"] = mdValue;
    cParams["min"] = mdMin;
    cParams["max"] = mdMax;
    cParams["step"] = mdStep;
    cParams["decimals"] = miDecimals;
    cParams["fontsize"] = miFontSize;
    modelJson["cParams"] = cParams;
    return modelJson;
}

void FloatSpinButtonModel::load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        QJsonValue v = paramsObj["min"];
        if (!v.isNull()) {
            mdMin = v.toDouble();
            auto prop = mMapIdToProperty["min"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = mdMin;
            mpEmbeddedWidget->setMinimum(mdMin);
        }
        v = paramsObj["max"];
        if (!v.isNull()) {
            mdMax = v.toDouble();
            auto prop = mMapIdToProperty["max"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = mdMax;
            mpEmbeddedWidget->setMaximum(mdMax);
        }
        v = paramsObj["step"];
        if (!v.isNull()) {
            mdStep = v.toDouble();
            auto prop = mMapIdToProperty["step"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = mdStep;
            mpEmbeddedWidget->setSingleStep(mdStep);
        }
        v = paramsObj["decimals"];
        if (!v.isNull()) {
            miDecimals = v.toInt();
            auto prop = mMapIdToProperty["decimals"];
            auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
            typedProp->getData().miValue = miDecimals;
            mpEmbeddedWidget->setDecimals(miDecimals);
        }
        v = paramsObj["value"];
        if (!v.isNull()) {
            mdValue = v.toDouble();
            auto prop = mMapIdToProperty["value"];
            auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
            typedProp->getData().mdValue = mdValue;
            mpFloatData->data() = static_cast<float>(mdValue);
            mpEmbeddedWidget->setValue(mdValue);
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
            auto typedValProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(valProp);
            typedValProp->getData().mdMin = mdMin;
            typedValProp->getData().mdMax = mdMax;
        }
    }
    mpEmbeddedWidget->setEnabled(isEnable());
}

void FloatSpinButtonModel::setModelProperty(QString &id, const QVariant &value)
{
    PBNodeDelegateModel::setModelProperty(id, value);

    if (!mMapIdToProperty.contains(id))
        return;

    auto prop = mMapIdToProperty[id];
    if (id == "value") {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mdValue = value.toDouble();
        mpFloatData->data() = static_cast<float>(mdValue);
        if (mpEmbeddedWidget->value() != mdValue) {
            mpEmbeddedWidget->blockSignals(true);
            mpEmbeddedWidget->setValue(mdValue);
            mpEmbeddedWidget->blockSignals(false);
        }
        updateAllOutputPorts();
    } else if (id == "min") {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mdMin = value.toDouble();
        mpEmbeddedWidget->setMinimum(mdMin);
        auto valProp = mMapIdToProperty["value"];
        if (valProp) {
            auto typedValProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(valProp);
            typedValProp->getData().mdMin = mdMin;
        }
    } else if (id == "max") {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mdMax = value.toDouble();
        mpEmbeddedWidget->setMaximum(mdMax);
        auto valProp = mMapIdToProperty["value"];
        if (valProp) {
            auto typedValProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(valProp);
            typedValProp->getData().mdMax = mdMax;
        }
    } else if (id == "step") {
        auto typedProp = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typedProp->getData().mdValue = value.toDouble();
        mdStep = value.toDouble();
        mpEmbeddedWidget->setSingleStep(mdStep);
    } else if (id == "decimals") {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miDecimals = value.toInt();
        mpEmbeddedWidget->setDecimals(miDecimals);
    } else if (id == "fontsize") {
        auto typedProp = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typedProp->getData().miValue = value.toInt();
        miFontSize = value.toInt();
        QFont font = mpEmbeddedWidget->font();
        font.setPointSize(miFontSize);
        mpEmbeddedWidget->setFont(font);
    }
}

void FloatSpinButtonModel::enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed(enable);
    mpEmbeddedWidget->setEnabled(enable);
}

void FloatSpinButtonModel::em_value_changed(double val)
{
    if (!checkSelectionForInteraction()) {
        mpEmbeddedWidget->blockSignals(true);
        mpEmbeddedWidget->setValue(mdValue);
        mpEmbeddedWidget->blockSignals(false);
        Q_EMIT selection_request_signal();
        return;
    }
    requestPropertyChange("value", val);
}
