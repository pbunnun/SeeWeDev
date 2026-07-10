//Copyright © 2026, NECTEC, all rights reserved

#include "RGBLedModel.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <QRadialGradient>

RGBLedEmbeddedWidget::RGBLedEmbeddedWidget(QWidget *parent)
    : QWidget(parent), mState(false)
{
    setMinimumSize(40, 40);
}

void RGBLedEmbeddedWidget::setState(bool state)
{
    if (mState != state) {
        mState = state;
        update();
    }
}

void RGBLedEmbeddedWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int size = qMin(width(), height()) - 8;
    int x = (width() - size) / 2;
    int y = (height() - size) / 2;
    QRectF rect(x, y, size, size);

    QColor ledColor = mState ? QColor(0, 220, 0) : QColor(220, 0, 0);
    QColor lightColor = mState ? QColor(100, 255, 100) : QColor(255, 100, 100);

    QPen bezelPen(QColor(50, 50, 50), 3);
    painter.setPen(bezelPen);

    QRadialGradient gradient(rect.center(), size / 2, rect.center() - QPointF(size/6.0, size/6.0));
    gradient.setColorAt(0.0, lightColor);
    gradient.setColorAt(0.8, ledColor);
    gradient.setColorAt(1.0, ledColor.darker(150));

    painter.setBrush(gradient);
    painter.drawEllipse(rect);
}

const QString RGBLedModel::_category = QString("GUI");
const QString RGBLedModel::_model_name = QString("RGB LED");

RGBLedModel::RGBLedModel()
    : PBNodeDelegateModel(_model_name),
      mpEmbeddedWidget(new RGBLedEmbeddedWidget(qobject_cast<QWidget*>(this)))
{
    // Draw the minimized icon programmatically
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(220, 0, 0));
    painter.setPen(QPen(QColor(50, 50, 50), 2));
    painter.drawEllipse(2, 2, 20, 20);
    _minPixmap = pix;

    // Set minimum size to 40x40 and initialize size to 100x100 to allow resizability
    mpEmbeddedWidget->setMinimumSize(40, 40);
    mpEmbeddedWidget->resize(100, 100);
}

unsigned int RGBLedModel::nPorts(PortType portType) const
{
    switch (portType) {
    case PortType::In:
        return 1;
    default:
        return 0;
    }
}

NodeDataType RGBLedModel::dataType(PortType portType, PortIndex) const
{
    if (portType == PortType::In)
        return SyncData().type();
    return NodeDataType();
}

QString RGBLedModel::portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == PortType::In && portIndex == 0) {
        return "Sync Input: Controls the LED state (Green when active/true, Red when inactive/false).";
    }
    return PBNodeDelegateModel::portToolTip(portType, portIndex);
}

QJsonObject RGBLedModel::save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["state"] = mpEmbeddedWidget->state();
    modelJson["cParams"] = cParams;
    return modelJson;
}

void RGBLedModel::load(const QJsonObject &p)
{
    PBNodeDelegateModel::load(p);
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        QJsonValue v = paramsObj["state"];
        if (!v.isNull()) {
            mpEmbeddedWidget->setState(v.toBool());
        }
    }
    mpEmbeddedWidget->setEnabled(isEnable());
}

void RGBLedModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (!isEnable())
        return;
    if (portIndex == 0) {
        if (!nodeData) {
            mpEmbeddedWidget->setState(false);
            return;
        }
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
        if (d) {
            mpEmbeddedWidget->setState(d->data());
        } else {
            mpEmbeddedWidget->setState(false);
        }
    }
}

void RGBLedModel::enable_changed(bool enable)
{
    PBNodeDelegateModel::enable_changed(enable);
    mpEmbeddedWidget->setEnabled(enable);
}
