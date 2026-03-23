#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include "PBNodeDelegateModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"
#include "PBImageDisplayWidget.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "StdVectorNumberData.hpp" // ต้องมั่นใจว่ามีไฟล์นี้

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class PlotGraphModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    PlotGraphModel();
    virtual ~PlotGraphModel() override = default;

    QJsonObject
    save() const override;

    void
    load(QJsonObject const& p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
        outData(PortIndex) override;

    void
        setInData(std::shared_ptr<NodeData>, PortIndex) override;

    QWidget* embeddedWidget() override { return mpPBImageDisplayWidget; }

    void
    setModelProperty(QString&, const QVariant&) override;

    void
    late_constructor() override;

    static const QString _model_name;
    static const QString _category;

private:
    PBImageDisplayWidget* mpPBImageDisplayWidget;

    std::shared_ptr< CVImageData > mpCVImageData;

    // --- แก้ไขจุดที่ 1: เปลี่ยนชนิดตัวแปรให้ตรงกับ SmartCounter ---
    // จากเดิม StdVectorFloatData เปลี่ยนเป็น StdVectorNumberData<float>
    std::shared_ptr< StdVectorNumberData<float> > mpstdVectorData;

private:
    std::vector<float> mHistory; // เก็บประวัติค่า Count

    // ฟังก์ชันวาดกราฟ
    void renderCombinedGraph();
    void drawConfidenceLine(cv::Mat& roi, const std::vector<float>& history);

    // --- แก้ไขจุดที่ 2: ลบ drawConfidenceHistogram ออก ---
    // void drawConfidenceHistogram(cv::Mat& roi, const std::vector<float>& history);
};
