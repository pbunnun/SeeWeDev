//Copyright © 2026, NECTEC, all rights reserved

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPushButton>
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "BoolData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

class ToggleButtonModel : public PBNodeDelegateModel
{
    Q_OBJECT
public:
    ToggleButtonModel();
    virtual ~ToggleButtonModel() override { }

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
    void em_button_clicked();

private:
    QPushButton *mpEmbeddedWidget;
    std::shared_ptr<SyncData> mpSyncData;
    std::shared_ptr<BoolData> mpBoolData;
    QPixmap _minPixmap;

    QString msLabel{"Toggle"};
    int miFontSize{12};
    bool mbState{false};
};
