#include "SmartCounterModel.hpp"
#include "SyncData.hpp"
#include <QVariant>

// 1. กำหนดชื่อหมวดหมู่และชื่อโหนดที่จะปรากฏในเมนู
const QString SmartCounterModel::_category = QString("Utility");
const QString SmartCounterModel::_model_name = QString("Smart Counter");

SmartCounterModel::
    SmartCounterModel()
    : PBNodeDelegateModel(_model_name)
    , mLabel(nullptr)
    , mTotalCount(0) // เริ่มต้นให้ตัวนับเป็น 0
{
    // จองหน่วยความจำสำหรับข้อมูลที่จะส่งออก (Vector of Float)
    mOutputCountData = std::make_shared<StdVectorNumberData<float>>();
}

// 2. กำหนดจำนวนพอร์ต (In: 1, Out: 1)
unsigned int
    SmartCounterModel::
    nPorts(PortType portType) const {
    return 1;
}

QString SmartCounterModel::
    portCaption(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In)
        return QStringLiteral("Sync");

    return QStringLiteral("Count"); // เปลี่ยนชื่อพอร์ตออกให้สื่อความหมาย
}

NodeDataType SmartCounterModel::
    dataType(PortType portType, PortIndex portIndex)const
{
    if (portType == PortType::In)
    {
        if(portIndex == 0 )
            return SyncData().type();
    }
    else if( portType == PortType::Out )
    {
        if(portIndex == 0)
            return StdVectorNumberData<float>().type();
    }
    return NodeDataType();
}

// 5. ส่งข้อมูลออกไปยังโหนดถัดไป
std::shared_ptr<NodeData>
    SmartCounterModel::
    outData(PortIndex portIndex)
{
    Q_UNUSED(portIndex);
    return mOutputCountData;
}

// 6. ตรรกะการรับข้อมูล (หัวใจของโหนด)
void
    SmartCounterModel::
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (nodeData)
    {
        // รับข้อมูลคะแนนความมั่นใจจาก YOLO
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);

        if (d)
        {
            float conf = d->data(); // ค่าความมั่นใจ (เช่น 0.0 หรือ 0.85)

            // --- ตรรกะการนับสะสม ---
            // ถ้าค่าความมั่นใจมากกว่า 0.5 (แสดงว่าเจอวัตถุ) ให้บวกเลขเพิ่ม 1
            if (conf > 0.5f)
            {
                mTotalCount++;
            }

            // หมายเหตุ: การนับแบบนี้เลขจะวิ่งเร็วมาก (ตามจำนวนเฟรมภาพ)
            // ถ้าคนยืนหน้ากล้อง 1 วินาที เลขจะบวกเพิ่มไปประมาณ 10-30 แล้วแต่ความเร็วเครื่อง

            // ส่งค่า "ยอดรวมสะสม" ออกไปแทนค่าความมั่นใจ
            mOutputCountData->data().clear();
            mOutputCountData->data().push_back((float)mTotalCount);

            // อัปเดตตัวเลขบนหน้าจอโหนด (แสดงเลขจำนวนเต็มสะสม)
            if (mLabel)
            {
                mLabel->setText(QString::number(mTotalCount));
            }

            // แจ้งระบบว่าข้อมูลมีการเปลี่ยนแปลงที่พอร์ตขาออก 0
            Q_EMIT dataUpdated(0);
        }
    }
}

// 7. ส่วนแสดงผลบนโหนด (UI)
QWidget* SmartCounterModel::
    embeddedWidget()
{
    if (!mLabel)
    {
        mLabel = new QLabel("0");
        mLabel->setAlignment(Qt::AlignCenter);
        mLabel->setFixedSize(120, 80);

        mLabel->setStyleSheet(
            "QLabel { "
            "   background-color: #1e272e; "
            "   color: #00d8d6; "     // สีฟ้าสดใส
            "   font-size: 48px; "    // เพิ่มขนาดตัวอักษรให้สะใจ
            "   font-weight: bold; "
            "   border-radius: 8px; "
            "   border: 2px solid #00d8d6; " // ขอบสีเดียวกับตัวหนังสือ
            "}"
            );
    }
    return mLabel;
}
