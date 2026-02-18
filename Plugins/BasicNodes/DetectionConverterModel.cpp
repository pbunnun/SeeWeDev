#include "DetectionConverterModel.hpp"

const QString DetectionConverterModel::_category = QString("Utility");
const QString DetectionConverterModel::_model_name = QString("Detection To Float");

DetectionConverterModel::DetectionConverterModel()
    : PBNodeDelegateModel(_model_name)
{
    mOutputData = std::make_shared<StdVectorNumberData<float>>();
}

unsigned int DetectionConverterModel::nPorts(PortType portType) const {
    return 1; // In: 1, Out: 1
}

NodeDataType
    DetectionConverterModel::
    dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In)
    {
        // ขาเข้าพอร์ตที่ 0 รับข้อมูลประเภท SyncData
        return SyncData().type();
    }
    else if (portType == PortType::Out)
    {
        // ขาออกพอร์ตที่ 0 ส่งข้อมูลประเภทตัวเลข (ใช้ ID "Cnt" ตามที่คุณต้องการ)
        if (portIndex == 0)
        {
            return NodeDataType {"Cnt", "Cnt"};
        }
    }

    return NodeDataType();
}

std::shared_ptr<NodeData>
    DetectionConverterModel::
    outData(PortIndex portIndex)
{
    // คืนค่าข้อมูลเฉพาะพอร์ตที่ 0 เท่านั้น
    if (portIndex == 0)
    {
        return mOutputData;
    }
    return nullptr;
}

void
    DetectionConverterModel::
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (portIndex == 0)
    {
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
        if (d)
        {
            // ดึงค่าผลลัพธ์จาก DNN (โดยปกติ d->data() จะคืนค่าเป็น bool หรือ score)
            // ถ้าอยากให้เป็นเส้นคลื่น เราจะใช้การกรองข้อมูล (Low-pass Filter)
            float targetValue = d->data() ? 1.0f : 0.0f;

            // ใช้ค่า mCurrentValue (ต้องประกาศใน .hpp) เพื่อทำเส้นโค้ง
            // 0.1 คือความนุ่มนวล ยิ่งน้อยยิ่งโค้งเป็นเวฟ
            static float mCurrentValue = 0.0f;
            float alpha = 0.1f;
            mCurrentValue = (alpha * targetValue) + ((1.0f - alpha) * mCurrentValue);

            mOutputData->data().clear();
            mOutputData->data().push_back(mCurrentValue); // ส่งค่าที่เป็น "เวฟ" ออกไป

            Q_EMIT dataUpdated(0);
        }
    }
}


