#include "PlotGraphModel.hpp"
#include "qtvariantproperty_p.h"
#include <QDebug>
#include "CVImageData.hpp"
#include <algorithm> // สำหรับหาค่า max

const QString PlotGraphModel::_category = QString("Output");
const QString PlotGraphModel::_model_name = QString("Plot Graph");

PlotGraphModel::PlotGraphModel()
    : PBNodeDelegateModel(_model_name)
{
    mpPBImageDisplayWidget = new PBImageDisplayWidget();

    // สร้างภาพพื้นหลังสีดำเริ่มต้น ขนาด 400x300
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat::zeros(300, 400, CV_8UC3));

    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<std::shared_ptr<CVImageData>>("std::shared_ptr<CVImageData>");
}

unsigned int PlotGraphModel::nPorts(PortType portType) const {
    if (portType == PortType::In) return 1;
    return 0;
}

NodeDataType PlotGraphModel::dataType(PortType portType, PortIndex portIndex) const {
    Q_UNUSED(portIndex);
    // รับข้อมูลเป็น Vector Number (ตามที่ Smart Counter ส่งออกมา)
    if (portType == PortType::In) return StdVectorNumberData<float>().type();
    return NodeDataType();
}

std::shared_ptr<NodeData> PlotGraphModel::outData(PortIndex portIndex) {
    Q_UNUSED(portIndex);
    return nullptr;
}

// --- ส่วนรับข้อมูล ---
void PlotGraphModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) {
    Q_UNUSED(portIndex);

    if (nodeData) {
        // Cast ข้อมูลให้ตรงกับที่ส่งมาจาก SmartCounter
        auto d = std::dynamic_pointer_cast<StdVectorNumberData<float>>(nodeData);
        if (d && !d->data().empty()) {
            // ดึงค่า Count ล่าสุด
            float val = d->data().at(0);

            // เก็บประวัติ
            mHistory.push_back(val);

            // เก็บข้อมูลย้อนหลัง 200 เฟรม (ปรับได้ตามชอบ)
            if (mHistory.size() > 200) {
                mHistory.erase(mHistory.begin());
            }

            // สั่งวาดกราฟ
            renderCombinedGraph();
        }
    }
}

// --- ส่วนวาดกราฟหลัก ---
void PlotGraphModel::renderCombinedGraph() {
    // เตรียม Canvas สีดำเต็มจอ
    cv::Mat canvas = cv::Mat::zeros(300, 400, CV_8UC3);

    // เรียกฟังก์ชันวาดกราฟขั้นบันได
    drawConfidenceLine(canvas, mHistory);

    // อัปเดตการแสดงผล
    mpCVImageData->set_image(canvas);
    mpPBImageDisplayWidget->Display(canvas);
}

// --- ฟังก์ชันวาดกราฟขั้นบันได (Step Chart) ---
void PlotGraphModel::drawConfidenceLine(cv::Mat& roi, const std::vector<float>& history) {
    if (history.size() < 2) return;

    // 1. คำนวณหาค่าสูงสุดเพื่อปรับสเกลแกน Y (Auto-Scale)
    float maxVal = 10.0f; // ตั้งค่าต่ำสุดไว้ที่ 10 เพื่อไม่ให้กราฟดูโล่งตอนค่าน้อยๆ
    for(float v : history) {
        if(v > maxVal) maxVal = v;
    }
    maxVal = maxVal * 1.2f; // เผื่อที่ว่างด้านบน 20%

    // 2. เตรียมจุด Polygon สำหรับระบายสีพื้นหลัง
    std::vector<cv::Point> points;
    points.push_back(cv::Point(0, roi.rows)); // จุดเริ่ม (ซ้ายล่าง)

    for (size_t i = 0; i < history.size(); ++i) {
        int x = i * roi.cols / (history.size() - 1);
        int y = roi.rows - (int)((history[i] / maxVal) * roi.rows);

        // เทคนิค Step Chart: เพิ่มจุดมุมฉากเพื่อให้เส้นเป็นขั้นบันได
        if (i > 0) {
            int prevX = (i - 1) * roi.cols / (history.size() - 1);
            // เพิ่มจุดนี้เพื่อให้เกิดมุมฉาก
            points.push_back(cv::Point(x, roi.rows - (int)((history[i-1] / maxVal) * roi.rows)));
        }
        points.push_back(cv::Point(x, y));
    }
    points.push_back(cv::Point(roi.cols, roi.rows)); // จุดจบ (ขวาล่าง)

    // 3. ระบายสีเขียวจางๆ (Filled Area)
    std::vector<std::vector<cv::Point>> fillPolyArr;
    fillPolyArr.push_back(points);
    cv::fillPoly(roi, fillPolyArr, cv::Scalar(0, 50, 0)); // สีพื้นหลังเขียวเข้ม

    // 4. วาดเส้นขอบกราฟแบบขั้นบันได (Step Line)
    for (size_t i = 1; i < history.size(); ++i) {
        int x1 = (i - 1) * roi.cols / (history.size() - 1);
        int x2 = i * roi.cols / (history.size() - 1);

        int y1 = roi.rows - (int)((history[i-1] / maxVal) * roi.rows);
        int y2 = roi.rows - (int)((history[i] / maxVal) * roi.rows);

        // วาดเส้นแนวนอน
        cv::line(roi, cv::Point(x1, y1), cv::Point(x2, y1), cv::Scalar(0, 255, 127), 2, cv::LINE_AA);

        // วาดเส้นแนวตั้ง (เมื่อค่าเปลี่ยน)
        if (y1 != y2) {
            cv::line(roi, cv::Point(x2, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 127), 2, cv::LINE_AA);
        }
    }

    // 5. วาด Grid และตัวเลขบอกสเกล
    int gridSteps = 5;
    for(int i=0; i<=gridSteps; i++) {
        int y = roi.rows - (i * roi.rows / gridSteps);
        // เส้น Grid แนวนอนจางๆ
        cv::line(roi, cv::Point(0, y), cv::Point(roi.cols, y), cv::Scalar(50, 50, 50), 1);

        // ตัวเลขสเกลด้านซ้าย
        float labelVal = i * (maxVal / gridSteps);
        cv::putText(roi, QString::number((int)labelVal).toStdString(),
                    cv::Point(5, y - 5), 0, 0.4, cv::Scalar(150, 150, 150));
    }

    // 6. แสดงผลรวมล่าสุดที่มุมขวาบน
    if (!history.empty()) {
        std::string currentText = "Total: " + std::to_string((int)history.back());
        cv::putText(roi, currentText, cv::Point(roi.cols - 140, 40),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 255), 2);
    }

    // Label แกน X
    cv::putText(roi, "Time ->", cv::Point(roi.cols - 60, roi.rows - 10),
                0, 0.5, cv::Scalar(200, 200, 200));
}

// ฟังก์ชันอื่นๆ คงเดิม
void PlotGraphModel::setModelProperty(QString& id, const QVariant& value) {
    PBNodeDelegateModel::setModelProperty(id, value);
}
void PlotGraphModel::late_constructor() {}
QJsonObject PlotGraphModel::save() const { return PBNodeDelegateModel::save(); }
void PlotGraphModel::load(QJsonObject const& p) { PBNodeDelegateModel::load(p); }
