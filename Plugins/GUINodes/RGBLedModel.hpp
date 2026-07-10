//Copyright © 2026, NECTEC, all rights reserved

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

class RGBLedEmbeddedWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RGBLedEmbeddedWidget(QWidget *parent = nullptr);

    void setState(bool state);
    bool state() const { return mState; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool mState;
};

class RGBLedModel : public PBNodeDelegateModel
{
    Q_OBJECT
public:
    RGBLedModel();
    virtual ~RGBLedModel() override { }

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

private Q_SLOTS:
    void enable_changed(bool enable) override;

private:
    RGBLedEmbeddedWidget *mpEmbeddedWidget;
    QPixmap _minPixmap;
};
