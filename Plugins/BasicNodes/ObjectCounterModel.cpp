#include "ObjectCounterModel.hpp"
#include <QLabel>
#include "StdVectorNumberData.hpp"
#include "SyncData.hpp"

const QString ObjectCounterModel::_category = QString("Utility");
const QString ObjectCounterModel::_model_name = QString("Object Counter");

ObjectCounterModel::ObjectCounterModel()
    : PBNodeDelegateModel(_model_name)
{
    // จองหน่วยความจำสำหรับข้อมูลขาออก (Output)
    mOutputCountData = std::make_shared<StdVectorNumberData<float>>();
    mLabel = nullptr;
}

unsigned int ObjectCounterModel::nPorts(PortType portType) const {
    return 1; // In: 1, Out: 1
}

NodeDataType
    ObjectCounterModel::
    dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In)
    {
        if (portIndex == 0)
            // แก้กลับเป็นบรรทัดนี้ เพื่อให้แสดงชื่อ "Cnt" บนหน้าจอโหนด
            return NodeDataType {"Cnt", "Cnt"};
    }
    else if (portType == PortType::Out)
    {
        if (portIndex == 0)
            // ขาออกให้เป็นชื่อมาตรฐาน "Nbs" (Numbers) ไว้ส่งไปวาดกราฟ
            return StdVectorNumberData<float>().type();
    }

    return NodeDataType();
}

std::shared_ptr<NodeData>
    ObjectCounterModel::
    outData(PortIndex portIndex)
{
    // คืนค่าข้อมูลตาม Port Index เหมือนโหนด DNN
    if (portIndex == 0)
    {
        return mOutputCountData;
    }
    return nullptr;
}

void
    ObjectCounterModel::
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    // ตรวจสอบข้อมูลที่เข้าทางพอร์ต 0
    if (portIndex == 0)
    {
        if (nodeData)
        {
            // แงะกล่องข้อมูลแบบ Number Data ตามที่ Converter ส่งมา
            auto d = std::dynamic_pointer_cast<StdVectorNumberData<float>>(nodeData);

            if (d && !d->data().empty())
            {
                float count = d->data()[0];

                // อัปเดตข้อมูลขาออก
                mOutputCountData->data().clear();
                mOutputCountData->data().push_back(count);

                // อัปเดตตัวเลขบน UI ของโหนด
                if (mLabel)
                {
                    mLabel->setText(QString("Count: %1").arg((int)count));
                }

                // แจ้งพอร์ตขาออกที่ 0 ให้ทราบว่าข้อมูลเปลี่ยนแล้ว
                Q_EMIT dataUpdated(0);
            }
        }
    }
}

QWidget* ObjectCounterModel::embeddedWidget() {
    if (!mLabel) {
        mLabel = new QLabel("Count: 0");
        mLabel->setAlignment(Qt::AlignCenter);
        mLabel->setFixedSize(150, 100);
        mLabel->setStyleSheet(
            "QLabel { "
            "    background-color: #2c3e50; "
            "    color: #2ecc71; "
            "    font-size: 28px; "
            "    font-weight: bold; "
            "    border-radius: 12px; "
            "    border: 2px solid #34495e; "
            "}"
            );
    }
    return mLabel;
}

void ObjectCounterModel::late_constructor() {}

QJsonObject ObjectCounterModel::save() const {
    return PBNodeDelegateModel::save();
}

void ObjectCounterModel::load(QJsonObject const& p) {
    PBNodeDelegateModel::load(p);
}
