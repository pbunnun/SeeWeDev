//Copyright © 2026, NECTEC, all rights reserved

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include "PBNodeDelegateModel.hpp"
#include "IntegerData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

class IntSpinButtonModel : public PBNodeDelegateModel
{
    Q_OBJECT
public:
    IntSpinButtonModel();
    virtual ~IntSpinButtonModel() override { }

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
    void em_value_changed(int val);

private:
    QSpinBox *mpEmbeddedWidget;
    std::shared_ptr<IntegerData> mpIntData;
    QPixmap _minPixmap;

    int miValue{0};
    int miMin{0};
    int miMax{100};
    int miStep{1};
    int miFontSize{12};
};
