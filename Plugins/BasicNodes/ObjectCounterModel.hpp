#pragma once

#include "PBNodeDelegateModel.hpp"
#include "StdVectorNumberData.hpp"
#include <QLabel>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

class ObjectCounterModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    ObjectCounterModel();
    virtual ~ObjectCounterModel() override = default;

    static const QString _model_name;
    static const QString _category;

    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex portIndex) override;
    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;

    QWidget* embeddedWidget() override;

    QJsonObject save() const override;
    void load(QJsonObject const& p) override;

    void late_constructor() override;

private:
    std::shared_ptr<StdVectorNumberData<float>> mOutputCountData;

    QLabel* mLabel = nullptr;
};
