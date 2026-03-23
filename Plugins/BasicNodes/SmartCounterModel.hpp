#ifndef SMARTCOUNTERMODEL_HPP
#define SMARTCOUNTERMODEL_HPP

#include "PBNodeDelegateModel.hpp"
#include "StdVectorNumberData.hpp"
#include <QLabel>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

class SmartCounterModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    SmartCounterModel();
    virtual ~SmartCounterModel() override = default;

    static const QString _model_name;
    static const QString _category;

    // ส่วนกำหนดชื่อพอร์ต Sync และ Cnt
    QString portCaption(PortType portType, PortIndex portIndex) const override;

    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex portIndex) override;
    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;

    QWidget* embeddedWidget() override;

private:
    std::shared_ptr<StdVectorFloatData> mOutputCountData;
    QLabel* mLabel;

    int mTotalCount = 0; // ตัวแปรสำหรับเก็บเลขสะสม
};


#endif // SMARTCOUNTERMODEL_HPP
