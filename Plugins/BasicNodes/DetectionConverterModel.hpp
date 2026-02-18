#pragma once
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "StdVectorNumberData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeDataType;
using QtNodes::NodeData;

class DetectionConverterModel : public PBNodeDelegateModel {
    Q_OBJECT

public:
    DetectionConverterModel();
    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex portIndex) override;
    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;
    QWidget* embeddedWidget() override { return nullptr; }

    static const QString _category;
    static const QString _model_name;


private:
    std::shared_ptr<StdVectorNumberData<float>> mOutputData; // ใช้ Template <float>

    float mCurrentValue = 0.0f;
};
