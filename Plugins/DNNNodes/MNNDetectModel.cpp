#include "MNNDetectModel.hpp"
#include "CVImageData.hpp"
#include "InformationData.hpp"
#include "qtvariantproperty_p.h"

#include <QFile>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <QDebug>

const QString MNNDetectModel::_category = QString("DNN");
const QString MNNDetectModel::_model_name = QString("MNN YOLO Detector");

// =============================================================================
// Part 1: Thread Implementation
// =============================================================================

MNNDetectModelThread::MNNDetectModelThread(QObject* parent) : QThread(parent) {}
MNNDetectModelThread::~MNNDetectModelThread() { abort(); }

void MNNDetectModelThread::abort() {
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}

void MNNDetectModelThread::detect(const cv::Mat& img) {
    if (mLockMutex.tryLock()) {
        img.copyTo(mCVImage);
        mWaitingSemaphore.release();
        mLockMutex.unlock();
    }
}

bool MNNDetectModelThread::read_net(const QString& model_path, const QString& input_node, const QString& output_node) {
    QMutexLocker l(&mLockMutex);
    
    // โหลดโมเดลผ่าน Interpreter
    mNet.reset(MNN::Interpreter::createFromFile(model_path.toStdString().c_str()));
    if (!mNet) return mbModelReady = false;
    
    // ตั้งค่าการประมวลผล (ใช้ CPU และ 4 Threads)
    MNN::ScheduleConfig config;
    config.type = MNN_FORWARD_CPU; 
    config.numThread = 4;
    
    mSession = mNet->createSession(config);
    return mbModelReady = (mSession != nullptr);
}

void MNNDetectModelThread::setParams(const MnnDetectParameters& p) {
    QMutexLocker locker(&mLockMutex);
    mParams = p;
}

void MNNDetectModelThread::run() {
    while (!mbAbort) {
        mWaitingSemaphore.acquire();
        if (mbAbort || !mbModelReady) continue;

        mLockMutex.lock();
        cv::Mat frame = mCVImage.clone(); 
        MnnDetectParameters p = mParams;
        mLockMutex.unlock();

        if (frame.empty() || !mNet || !mSession) continue;

        // 1. ดึง Input Tensor
        auto inputTensor = mNet->getSessionInput(mSession, p.msInputBlob.toStdString().c_str());
        
        // 2. ปรับขนาด Tensor ให้ตรงกับ Target Size
        mNet->resizeTensor(inputTensor, {1, 3, p.mTargetSize.height, p.mTargetSize.width});
        mNet->resizeSession(mSession);

        // 3. Preprocessing ด้วย MNN ImageProcess (เร็วกว่าใช้ OpenCV สลับสี/ลดขนาดเอง)
        MNN::CV::ImageProcess::Config img_config;
        img_config.filterType = MNN::CV::BILINEAR;
        ::memcpy(img_config.mean, p.meanVals.data(), 3 * sizeof(float));
        ::memcpy(img_config.normal, p.normVals.data(), 3 * sizeof(float)); 
        img_config.sourceFormat = MNN::CV::BGR;
        img_config.destFormat = MNN::CV::RGB;

        std::shared_ptr<MNN::CV::ImageProcess> pretreat(MNN::CV::ImageProcess::create(img_config));
        
        MNN::CV::Matrix trans;
        trans.setScale((float)frame.cols / p.mTargetSize.width, (float)frame.rows / p.mTargetSize.height);
        pretreat->setMatrix(trans);
        
        // แปลงภาพจาก cv::Mat เข้า Tensor โดยตรง
        pretreat->convert(frame.data, frame.cols, frame.rows, frame.step[0], inputTensor);

        // 4. สั่งรันโมเดล (Forward Pass)
        mNet->runSession(mSession);
        
        // 5. ดึง Output Tensor ออกมา
        auto outputTensor = mNet->getSessionOutput(mSession, p.msOutputBlob.toStdString().c_str());
        
        // ก๊อปปี้ข้อมูลจาก Device (GPU/NPU) ลงมาที่ Host (CPU RAM) เพื่อให้เราอ่านค่าได้
        MNN::Tensor outputHost(outputTensor, outputTensor->getDimensionType());
        outputTensor->copyToHostTensor(&outputHost);
        
        float* ptr = outputHost.host<float>(); 
        
        // โครงสร้างมาตรฐานของ YOLOv8 คือ [1, 4 + classes, 8400]
        int num_channels = outputHost.channel(); // จำนวนช่อง (เช่น 5 สำหรับ 1 คลาส)
        int num_anchors = outputHost.width() > 1 ? outputHost.width() : outputHost.height(); // 8400
        int num_classes = num_channels - 4;

        std::vector<MnnDetObject> proposals;
        std::vector<MnnDetObject> objects;

        for (int i = 0; i < num_anchors; i++) {
            float max_score = -1.f;
            int label = -1;

            // หาคลาสที่คะแนนสูงสุด
            for (int c = 0; c < num_classes; c++) {
                float score = ptr[(4 + c) * num_anchors + i];
                if (score > max_score) {
                    max_score = score;
                    label = c;
                }
            }

            // กรองความเชื่อมั่น (ปรับค่า 0.50f ได้ตามต้องการ)
            if (max_score > 0.50f) {
                MnnDetObject obj;
                float cx = ptr[0 * num_anchors + i];
                float cy = ptr[1 * num_anchors + i];
                float w  = ptr[2 * num_anchors + i];
                float h  = ptr[3 * num_anchors + i];

                obj.rect.x = cx - w * 0.5f;
                obj.rect.y = cy - h * 0.5f;
                obj.rect.width = w;
                obj.rect.height = h;
                obj.label = label;
                obj.score = max_score;
                proposals.push_back(obj);
            }
        }

        // คัดกรองกรอบที่ซ้อนกัน (NMS)
        nms_sorted_bboxes(proposals, objects, 0.25f);

        // --- ส่วนวาดภาพและสร้าง JSON คงเดิม ---
        for (auto& obj : objects) {
            MNNDetectModel::draw_object(frame, obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height, obj.label, obj.score, p);
        }

        QString result_information = "";
        if (objects.size() > 0) {
            result_information = "{\n";
            result_information += "    \"Total Leads\" : " + QString::number(objects.size()) + ",\n";
            result_information += "    \"Details\" : [\n";
            for (size_t i = 0; i < objects.size(); ++i) {
                QString className = (objects[i].label < p.mClassNames.size()) ? p.mClassNames[objects[i].label] : QString::number(objects[i].label);
                result_information += "        {\n";
                result_information += "            \"No\" : " + QString::number(i + 1) + ",\n";
                result_information += "            \"Class\" : \"" + className + "\",\n";
                result_information += "            \"Confidence\" : \"" + QString::number(objects[i].score * 100, 'f', 2) + "%\"\n";
                result_information += "        }";
                if (i < objects.size() - 1) result_information += ",";
                result_information += "\n";
            }
            result_information += "    ]\n}";
        } else {
            result_information = "{\n    \"Total Leads\" : 0,\n    \"Details\" : []\n}";
        }

        Q_EMIT result_ready(frame, result_information);
    }
}

// =============================================================================
// Part 2: Node Model Implementation
// =============================================================================

MNNDetectModel::MNNDetectModel() : PBNodeDelegateModel(Name()) {
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mpInfData = std::make_shared<InformationData>(); // เพิ่มพอร์ตที่ 2
    mpSyncData = std::make_shared<SyncData>(true);

    QString msInputBlob = "input";
    QString msOutputBlob = "output";
    QString msMeanVals = "0.0, 0.0, 0.0";
    QString msNormVals = "0.00392, 0.00392, 0.00392";
    QString msClassNames = "lead";

    // Network Group
    FilePathPropertyType fType; fType.msFilter = "*.mnn"; fType.msMode = "open";
    mvProperty.push_back(std::make_shared<TypedProperty<FilePathPropertyType>>("MNN Model", "model_file", QtVariantPropertyManager::filePathTypeId(), fType, "Network"));
    mMapIdToProperty["model_file"] = mvProperty.back();
    
    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Input Blob", "in_blob", QMetaType::QString, msInputBlob, "Network"));
    mMapIdToProperty["in_blob"] = mvProperty.back();
    
    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Output Blob", "out_blob", QMetaType::QString, msOutputBlob, "Network"));
    mMapIdToProperty["out_blob"] = mvProperty.back();

    // Config Group
    SizePropertyType sType; sType.miWidth = 640; sType.miHeight = 640;
    mvProperty.push_back(std::make_shared<TypedProperty<SizePropertyType>>("Target Size", "size", QMetaType::QSize, sType, "Config"));
    mMapIdToProperty["size"] = mvProperty.back();
    
    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Mean (R,G,B)", "mean_vals", QMetaType::QString, msMeanVals, "Config"));
    mMapIdToProperty["mean_vals"] = mvProperty.back();
    
    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Norm (R,G,B)", "norm_vals", QMetaType::QString, msNormVals, "Config"));
    mMapIdToProperty["norm_vals"] = mvProperty.back();
    
    // เพิ่ม Class Names ให้เหมือน NCNN
    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Class Names", "class_names", QMetaType::QString, msClassNames, "Config"));
    mMapIdToProperty["class_names"] = mvProperty.back();
}

MNNDetectModel::~MNNDetectModel() { if (mpMNNDetectModelThread) mpMNNDetectModelThread->abort(); }

// ปรับให้มี 3 Output Ports
unsigned int MNNDetectModel::nPorts(QtNodes::PortType t) const { return (t == QtNodes::PortType::In) ? 1 : 3; }
QtNodes::NodeDataType MNNDetectModel::dataType(QtNodes::PortType t, QtNodes::PortIndex i) const {
    if (t == QtNodes::PortType::In) return CVImageData().type();
    if (i == 0) return CVImageData().type(); else if (i == 1) return InformationData().type(); return SyncData().type();
}
std::shared_ptr<QtNodes::NodeData> MNNDetectModel::outData(QtNodes::PortIndex i) {
    if (isEnable()) { if (i == 0) return mpCVImageData; else if (i == 1) return mpInfData; return mpSyncData; } return nullptr;
}

void MNNDetectModel::setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex) {
    late_constructor(); if (!isEnable()) return;
    if (data && mpSyncData->data()) {
        mpSyncData->data() = false;
        auto d = std::dynamic_pointer_cast<CVImageData>(data);
        if (d) {
            mpInfData->set_timestamp(d->timestamp()); // Sync Timestamp เหมือน NCNN
            mpMNNDetectModelThread->detect(d->data());
        }
    }
}

void MNNDetectModel::received_result(cv::Mat res, QString text) {
    mpCVImageData->set_image(res);
    mpInfData->set_information(text); // รับค่า JSON
    mpSyncData->data() = true;
    updateAllOutputPorts();
}

void MNNDetectModel::late_constructor() {
    if (!mpMNNDetectModelThread) {
        qRegisterMetaType<cv::Mat>("cv::Mat");
        mpMNNDetectModelThread = new MNNDetectModelThread(this);
        connect(mpMNNDetectModelThread, &MNNDetectModelThread::result_ready, this, &MNNDetectModel::received_result);
        load_model(); mpMNNDetectModelThread->start();
    }
}

// โครงสร้างการวาดกรอบแบบ Dynamic (ถอดแบบจาก NCNN)
void MNNDetectModel::draw_object(cv::Mat& frame, float x, float y, float w, float h, int label, float score, const MnnDetectParameters& p) {
    float x1 = x * frame.cols / p.mTargetSize.width, y1 = y * frame.rows / p.mTargetSize.height;
    float w1 = w * frame.cols / p.mTargetSize.width, h1 = h * frame.rows / p.mTargetSize.height;
    cv::Rect roi = cv::Rect(x1, y1, w1, h1) & cv::Rect(0, 0, frame.cols, frame.rows);
    if (roi.width <= 0 || roi.height <= 0) return;

    // --- ส่วนตั้งค่าความหนาและขนาด (จากประวัติการสนทนา) ---
    // ปรับ box_thickness และ text_thickness ให้แยกกัน
    int box_thickness = std::max(3, (int)(frame.cols * 0.003)); 
    int text_thickness = std::max(2, (int)(frame.cols * 0.0015)); 
    double font_scale = frame.cols * 0.0006; 
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;

    // วาดกรอบวัตถุด้วยความหนา box_thickness และเปิด Anti-aliasing
    cv::rectangle(frame, roi, cv::Scalar(0, 255, 0), box_thickness, cv::LINE_AA);

    // เตรียมข้อความ (เช่น person: 84.00%)
    QString labelStr = (label < (int)p.mClassNames.size()) ? p.mClassNames[label] : QString::number(label);
    std::string text = labelStr.toStdString() + ": " + QString::number(score * 100, 'f', 2).toStdString() + "%";

    int baseLine = 0;
    // คำนวณขนาดข้อความโดยอิงจาก text_thickness
    cv::Size textSize = cv::getTextSize(text, fontFace, font_scale, text_thickness, &baseLine);
    int padding = 4; // ระยะขอบรอบตัวอักษร

    // --- [แก้ไขจุดนี้] เพื่อย้ายตัวหนังสือไปอยู่ด้านบนกล่อง ---
    
    // คำนวณตำแหน่ง Y สำหรับป้ายกำกับ โดยดักกรณีวัตถุอยู่ชิดขอบบนสุดของภาพ
    int y_label_top = roi.y - textSize.height - padding; // พิกัดด้านบนสุดของป้ายกำกับ
    int y_label; // พิกัด Y สำหรับวาดข้อความ (พิกัดมุมล่างซ้ายของข้อความ)
    cv::Rect bgRect; // กรอบพื้นหลังข้อความ

    if (y_label_top > 0) {
        // กรณีปกติ: วัตถุไม่อยู่ชิดขอบบน วาดป้ายกำกับ "ด้านนอกและด้านบน" ของกล่อง
        // วางด้านล่างของป้ายกำกับให้ชิดกับด้านบนของกรอบวัตถุพอดี
        y_label = roi.y - (padding / 2); // ข้อความจะอยู่เหนือกรอบเล็กน้อยเพื่อให้มองง่าย
        bgRect = cv::Rect(roi.x, roi.y - textSize.height - padding, textSize.width + padding, textSize.height + padding);
    } else {
        // กรณีพิเศษ: วัตถุอยู่ชิดขอบบน วาดป้ายกำกับ "ด้านในและด้านบนสุด" เพื่อไม่ให้หลุดขอบภาพ
        y_label = roi.y + textSize.height + (padding / 2); // ข้อความจะอยู่ด้านในกรอบที่ด้านบนสุด
        bgRect = cv::Rect(roi.x, roi.y, textSize.width + padding, textSize.height + padding);
    }

    // วาดพื้นหลังข้อความแบบโปร่งแสง (จากประวัติการสนทนา)
    cv::Mat overlay;
    frame.copyTo(overlay);
    cv::rectangle(overlay, bgRect, cv::Scalar(0, 100, 0), -1);
    cv::addWeighted(overlay, 0.8, frame, 0.2, 0, frame); 

    // วาดข้อความสีขาวโดยใช้ตำแหน่ง Y ที่คำนวณได้ใหม่ และใช้ text_thickness
    cv::putText(frame, text, cv::Point(roi.x + (padding / 2), y_label), 
                fontFace, font_scale, cv::Scalar(255, 255, 255), text_thickness, cv::LINE_AA);
}

// ฟังก์ชันจำค่า Property
void MNNDetectModel::setModelProperty(QString& id, const QVariant& v) {
    PBNodeDelegateModel::setModelProperty(id, v);
    
    if (!mMapIdToProperty.contains(id)) return;
    auto prop = mMapIdToProperty[id];

    // 1. จัดการช่องไฟล์โมเดล
    if (id == "model_file") {
        msModel_Filename = v.toString();
        std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(prop)->getData().msFilename = msModel_Filename;
    } 
    // 2. จัดการช่องข้อความทั้งหมด (จัดกลุ่มให้อยู่บรรทัดเดียวกันจะได้ดูเป็นระเบียบ)
    else if (id == "in_blob" || id == "out_blob" || id == "class_names" || id == "mean_vals" || id == "norm_vals") {
        std::static_pointer_cast<TypedProperty<QString>>(prop)->getData() = v.toString();
    }
    // 3. จัดการช่องขนาด Size (ใส่ตัวดักจับเพื่อป้องกันไม่ให้ค่ากลายเป็น 0)
    else if (id == "size") {
        if (v.canConvert<QSize>()) {
            QSize newSize = v.toSize();
            // อัปเดตก็ต่อเมื่อค่าที่ส่งมาไม่ใช่เลข 0
            if (newSize.width() > 0 && newSize.height() > 0) {
                auto sizeProp = std::static_pointer_cast<TypedProperty<SizePropertyType>>(prop);
                sizeProp->getData().miWidth = newSize.width();
                sizeProp->getData().miHeight = newSize.height();
            }
        }
    }

    // โหลดโมเดลใหม่และอัปเดตหน้าจอด้วยค่าที่เพิ่งถูกบันทึก
    load_model(true);
}

QJsonObject MNNDetectModel::save() const {
    QJsonObject m = PBNodeDelegateModel::save(); 
    QJsonObject c;
    c["model_filename"] = msModel_Filename;
    c["in_blob"] = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty.value("in_blob"))->getData();
    c["out_blob"] = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty.value("out_blob"))->getData();
    c["class_names"] = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty.value("class_names"))->getData();
    c["mean"] = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty.value("mean_vals"))->getData();
    c["norm"] = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty.value("norm_vals"))->getData();
    
    auto sz = std::static_pointer_cast<TypedProperty<SizePropertyType>>(mMapIdToProperty.value("size"))->getData();
    c["width"] = sz.miWidth; 
    c["height"] = sz.miHeight;
    m["cParams"] = c; 
    return m;
}

void MNNDetectModel::load(QJsonObject const& p) {
    PBNodeDelegateModel::load(p); 
    late_constructor(); 
    QJsonObject c = p["cParams"].toObject();
    if (!c.isEmpty()) {
        if (c.contains("model_filename")) {
            msModel_Filename = c["model_filename"].toString();
            if (mMapIdToProperty.contains("model_file"))
                std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(mMapIdToProperty["model_file"])->getData().msFilename = msModel_Filename;
        }
        
        if (mMapIdToProperty.contains("in_blob")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["in_blob"])->getData() = c["in_blob"].toString();
        if (mMapIdToProperty.contains("out_blob")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["out_blob"])->getData() = c["out_blob"].toString();
        if (mMapIdToProperty.contains("class_names")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["class_names"])->getData() = c["class_names"].toString();
        if (mMapIdToProperty.contains("mean_vals")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["mean_vals"])->getData() = c["mean"].toString();
        if (mMapIdToProperty.contains("norm_vals")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["norm_vals"])->getData() = c["norm"].toString();
        
        if (c.contains("width") && mMapIdToProperty.contains("size")) {
            auto sz = std::static_pointer_cast<TypedProperty<SizePropertyType>>(mMapIdToProperty["size"]);
            sz->getData().miWidth = c["width"].toInt(); 
            sz->getData().miHeight = c["height"].toInt();
        }
        load_model();
    }
}

std::vector<float> MNNDetectModel::stringToVecFloat(const QString& str, float d) {
    std::vector<float> v = {d, d, d}; QStringList p = str.split(",", Qt::SkipEmptyParts);
    for(int i=0; i<std::min((int)p.size(), 3); ++i) { bool ok; float val = p[i].toFloat(&ok); if(ok) v[i] = val; }
    return v;
}

void MNNDetectModel::load_model(bool) {
    if (msModel_Filename.isEmpty() || !QFile::exists(msModel_Filename)) return;
    MnnDetectParameters p;
    p.msInputBlob = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["in_blob"])->getData();
    p.msOutputBlob = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["out_blob"])->getData();
    p.mClassNames = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["class_names"])->getData().split(",", Qt::SkipEmptyParts);
    p.meanVals = stringToVecFloat(std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["mean_vals"])->getData(), 0.f);
    p.normVals = stringToVecFloat(std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["norm_vals"])->getData(), 1.0f/255.0f);
    auto sz = std::static_pointer_cast<TypedProperty<SizePropertyType>>(mMapIdToProperty["size"])->getData();
    p.mTargetSize = cv::Size(sz.miWidth, sz.miHeight);
    
    if (mpMNNDetectModelThread) { 
        mpMNNDetectModelThread->read_net(msModel_Filename, p.msInputBlob, p.msOutputBlob); 
        mpMNNDetectModelThread->setParams(p); 
    }
}

void MNNDetectModelThread::nms_sorted_bboxes(std::vector<MnnDetObject>& inputs, std::vector<MnnDetObject>& outputs, float nms_threshold) {
    outputs.clear();
    std::sort(inputs.begin(), inputs.end(), [](const MnnDetObject& a, const MnnDetObject& b) { return a.score > b.score; });
    std::vector<bool> suppressed(inputs.size(), false);
    for (size_t i = 0; i < inputs.size(); i++) {
        if (suppressed[i]) continue;
        outputs.push_back(inputs[i]);
        for (size_t j = i + 1; j < inputs.size(); j++) {
            if (!suppressed[j] && calculate_iou(inputs[i], inputs[j]) > nms_threshold) {
                suppressed[j] = true;
            }
        }
    }
}

float MNNDetectModelThread::calculate_iou(const MnnDetObject& a, const MnnDetObject& b) {
    float inter = (a.rect & b.rect).area();
    return inter / (a.rect.area() + b.rect.area() - inter);
}