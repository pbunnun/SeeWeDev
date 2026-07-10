//Copyright © 2026, NECTEC, all rights reserved

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QDoubleSpinBox>
#include "PBNodeDelegateModel.hpp"
#include "FloatData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

class FloatSpinButtonModel : public PBNodeDelegateModel
{
    Q_OBJECT
public:
    FloatSpinButtonModel();
    virtual ~FloatSpinButtonModel() override { }

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;

    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    QString portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex portIndex) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;

    QWidget *embeddedWidget() override { return mpEmbeddedWidget; }

    QPixmap minPixmap() const override { return _minPixmap; }

    static const QString _category;
    static const QString _model_name;

    void setModelProperty(QString &id, const QVariant &value) override;

private Q_SLOTS:
    void enable_changed(bool enable) override;
    void em_value_changed(double val);

private:
    QDoubleSpinBox *mpEmbeddedWidget;
    std::shared_ptr<FloatData> mpFloatData;
    QPixmap _minPixmap;

    double mdValue{0.0};
    double mdMin{0.0};
    double mdMax{100.0};
    double mdStep{1.0};
    int miDecimals{2};
    int miFontSize{12};
};
