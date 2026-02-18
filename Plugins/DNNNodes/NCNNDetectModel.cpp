#include "NCNNDetectModel.hpp"
#include <QFile>
#include <QElapsedTimer>
#include <QJsonArray>

#include "qtvariantproperty_p.h"

#include <QDebug> // ไว้ดูว่าบัคอะไร

const QString NCNNDetectModel::_category = QString("DNN");
const QString NCNNDetectModel::_model_name = QString("NCNN YOLO Detector");

// =============================================================================
// Part 1: Thread Implementation
// =============================================================================

NCNNDetectModelThread::NCNNDetectModelThread(QObject* parent) : QThread(parent) {}
NCNNDetectModelThread::~NCNNDetectModelThread() { abort(); }

void NCNNDetectModelThread::abort() {
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}

void NCNNDetectModelThread::run() {
    while (!mbAbort) {
        mWaitingSemaphore.acquire();
        if (mbAbort) break;
        if (!mbModelReady) continue;

        mLockMutex.lock();
        cv::Mat frame = mCVImage.clone();
        NcnnDetectParameters p = mParams;
        mLockMutex.unlock();

        if (frame.empty()) continue;

        ncnn::Mat in = ncnn::Mat::from_pixels_resize(frame.data, ncnn::Mat::PIXEL_BGR2RGB,
            frame.cols, frame.rows, p.mTargetSize.width, p.mTargetSize.height);
        float mean_vals[3], norm_vals[3];
        std::copy(p.meanVals.begin(), p.meanVals.begin() + 3, mean_vals);
        std::copy(p.normVals.begin(), p.normVals.begin() + 3, norm_vals);
        in.substract_mean_normalize(mean_vals, norm_vals);

        ncnn::Extractor ex = mNet.create_extractor();
        ex.input(p.msInputBlob.toStdString().c_str(), in);

        ncnn::Mat out;
        ex.extract(p.msOutputBlob.toStdString().c_str(), out);

        if (!out.empty()) {
            std::vector<NanoDetObject> objects;

            // --- เช็คเงื่อนไขเลือก Decode ---
            if (out.w == 6) {
                // สำหรับ NanoDet (Logic เดิม)
                for (int i = 0; i < out.h; i++) {
                    const float* v = out.row(i);
                    if (v[1] > 0.45f) draw_object(frame, v[2], v[3], v[4], v[5], (int)v[0], v[1], p);
                }
            }
            else if (out.h == 5 || out.w == 8400) {
                // [NEW] สำหรับ YOLOv8 (Shape ของคุณเข้าเงื่อนไขนี้)
                // qDebug() << "Detecting with YOLOv8 Logic...";
                decode_yolov8(out, p.mTargetSize.width, p.mTargetSize.height, objects);
            }
            else {
                // สำหรับ YOLOv5 (Logic เดิม)
                decode_yolov5_raw(out, p.mTargetSize.width, p.mTargetSize.height, objects);
            }
            // -----------------------------

            // วาดกรอบสี่เหลี่ยม
            for (auto& obj : objects) {
                draw_object(frame, obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height, obj.label, obj.score, p);
            }
        }

        Q_EMIT result_ready(frame.clone());
    }
}

void NCNNDetectModelThread::detect(const cv::Mat& img) {
    if (mLockMutex.tryLock()) {
        img.copyTo(mCVImage);
        mWaitingSemaphore.release();
        mLockMutex.unlock();
    }
}

bool NCNNDetectModelThread::read_net(const QString& param, const QString& bin) {
    QMutexLocker l(&mLockMutex);
    mNet.clear();
    if (mNet.load_param(param.toLocal8Bit().data()) == 0 && mNet.load_model(bin.toLocal8Bit().data()) == 0) {
        return mbModelReady = true;
    }
    return mbModelReady = false;
}

void NCNNDetectModelThread::setParams(const NcnnDetectParameters& p) {
    QMutexLocker l(&mLockMutex);
    mParams = p;
}

void NCNNDetectModelThread::draw_object(cv::Mat& frame, float x, float y, float w, float h, int label, float score, const NcnnDetectParameters& p) {
    float x1 = x * frame.cols / p.mTargetSize.width, y1 = y * frame.rows / p.mTargetSize.height;
    float w1 = w * frame.cols / p.mTargetSize.width, h1 = h * frame.rows / p.mTargetSize.height;
    cv::Rect roi = cv::Rect(x1, y1, w1, h1) & cv::Rect(0, 0, frame.cols, frame.rows);
    if (roi.width <= 0 || roi.height <= 0) return;
    cv::rectangle(frame, roi, cv::Scalar(0, 255, 0), 4);
    QString txt = QString("%1: %2").arg((label < p.mClassNames.size()) ? p.mClassNames[label] : QString::number(label)).arg(score, 0, 'f', 2);
    cv::putText(frame, txt.toStdString(), cv::Point(roi.x, std::max(15, roi.y - 5)), 0, 0.5, cv::Scalar(0, 255, 0), 1);
}

// [FIXED] Removed unused parameter names to silence warnings
void NCNNDetectModelThread::decode_yolov5_raw(const ncnn::Mat& out, int /*img_w*/, int /*img_h*/, std::vector<NanoDetObject>& objects) {
    std::vector<NanoDetObject> proposals;
    for (int i = 0; i < out.h; i++) {
        const float* ptr = out.row(i);
        if (ptr[4] > 0.45f) {
            float max_score = -1.f; int label = 0;
            for (int j = 5; j < out.w; j++) { if (ptr[j] > max_score) { max_score = ptr[j]; label = j - 5; } }
            if (ptr[4] * max_score > 0.45f) {
                NanoDetObject obj; obj.rect = cv::Rect_<float>(ptr[0] - ptr[2] / 2, ptr[1] - ptr[3] / 2, ptr[2], ptr[3]);
                obj.label = label; obj.score = ptr[4] * max_score; proposals.push_back(obj);
            }
        }
    }
    nms_sorted_bboxes(proposals, objects, 0.45f);
}

void NCNNDetectModelThread::decode_yolov8(const ncnn::Mat& out, int /*img_w*/, int /*img_h*/, std::vector<NanoDetObject>& objects) {
    std::vector<NanoDetObject> proposals;

    // YOLOv8 Output Layout (Transposed):
    // Row 0: Center X
    // Row 1: Center Y
    // Row 2: Width
    // Row 3: Height
    // Row 4..n: Class Scores

    const int num_anchors = out.w;      // 8400
    const int num_classes = out.h - 4;  // 5 - 4 = 1 Class

    // ดึง Pointer ของแต่ละแถวออกมาก่อน เพื่อความเร็วในการวนลูป
    const float* ptr_cx = out.row(0);
    const float* ptr_cy = out.row(1);
    const float* ptr_w = out.row(2);
    const float* ptr_h = out.row(3);

    for (int i = 0; i < num_anchors; i++) {
        float max_score = -1.f;
        int label = -1;

        // วนลูปหา Class ที่คะแนนเยอะที่สุดใน Anchor นี้
        for (int c = 0; c < num_classes; c++) {
            // Class Score เริ่มที่แถวที่ 4
            float score = out.row(4 + c)[i];
            if (score > max_score) {
                max_score = score;
                label = c;
            }
        }

        // กรองด้วย Threshold (0.45)
        if (max_score > 0.45f) {
            NanoDetObject obj;

            float cx = ptr_cx[i];
            float cy = ptr_cy[i];
            float w = ptr_w[i];
            float h = ptr_h[i];

            // แปลงจาก cx,cy,w,h เป็น x,y,w,h (มุมบนซ้าย)
            obj.rect.x = cx - w * 0.5f;
            obj.rect.y = cy - h * 0.5f;
            obj.rect.width = w;
            obj.rect.height = h;

            obj.label = label;
            obj.score = max_score;

            proposals.push_back(obj);
        }
    }

    // ใช้ NMS ตัวเดิมเพื่อตัดกล่องที่ซ้อนทับกัน
    nms_sorted_bboxes(proposals, objects, 0.45f);
}

void NCNNDetectModelThread::nms_sorted_bboxes(std::vector<NanoDetObject>& inputs, std::vector<NanoDetObject>& outputs, float nms_threshold) {
    outputs.clear();
    std::sort(inputs.begin(), inputs.end(), [](const NanoDetObject& a, const NanoDetObject& b) { return a.score > b.score; });
    std::vector<bool> suppressed(inputs.size(), false);
    for (size_t i = 0; i < inputs.size(); i++) {
        if (suppressed[i]) continue;
        outputs.push_back(inputs[i]);
        for (size_t j = i + 1; j < inputs.size(); j++) {
            if (!suppressed[j] && calculate_iou(inputs[i], inputs[j]) > nms_threshold) suppressed[j] = true;
        }
    }
}

float NCNNDetectModelThread::calculate_iou(const NanoDetObject& a, const NanoDetObject& b) {
    float inter = (a.rect & b.rect).area();
    return inter / (a.rect.area() + b.rect.area() - inter);
}

// =============================================================================
// Part 2: Node Model Implementation
// =============================================================================

NCNNDetectModel::NCNNDetectModel()
    : PBNodeDelegateModel(Name())
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mpSyncData = std::make_shared<SyncData>(true);

    if (msInputBlob.isEmpty()) msInputBlob = "in0";
    if (msOutputBlob.isEmpty()) msOutputBlob = "out0";
    if (msMeanVals.isEmpty()) msMeanVals = "0.0, 0.0, 0.0";
    if (msNormVals.isEmpty()) msNormVals = "0.00392, 0.00392, 0.00392";
    if (msClassNames.isEmpty()) msClassNames = "person, car";

    // --- 1. Model Param File ---
    FilePathPropertyType fType;
    fType.msFilename = msParam_Filename;
    fType.msFilter = "*.param";
    fType.msMode = "open";

    QString propId = "param_file";
    auto pParam = std::make_shared<TypedProperty<FilePathPropertyType>>("Model Param", propId, QtVariantPropertyManager::filePathTypeId(), fType);
    mvProperty.push_back(pParam);
    mMapIdToProperty[propId] = pParam;

    // --- 2. Model Bin File ---
    fType.msFilename = msBin_Filename;
    fType.msFilter = "*.bin";
    fType.msMode = "open";

    propId = "bin_file";
    auto pBin = std::make_shared<TypedProperty<FilePathPropertyType>>("Model Bin", propId, QtVariantPropertyManager::filePathTypeId(), fType);
    mvProperty.push_back(pBin);
    mMapIdToProperty[propId] = pBin;

    // --- 3. Input Blob ---
    propId = "in_blob";
    auto pInBlob = std::make_shared<TypedProperty<QString>>("Input Blob", propId, QMetaType::QString, msInputBlob, "Network");
    mvProperty.push_back(pInBlob);
    mMapIdToProperty[propId] = pInBlob;

    // --- 4. Output Blob ---
    propId = "out_blob";
    auto pOutBlob = std::make_shared<TypedProperty<QString>>("Output Blob", propId, QMetaType::QString, msOutputBlob, "Network");
    mvProperty.push_back(pOutBlob);
    mMapIdToProperty[propId] = pOutBlob;

    // --- 5. Target Size ---
    SizePropertyType sType;
    sType.miWidth = 640;
    sType.miHeight = 640;

    propId = "size";
    auto pSize = std::make_shared<TypedProperty<SizePropertyType>>("Target Size", propId, QMetaType::QSize, sType, "Config");
    mvProperty.push_back(pSize);
    mMapIdToProperty[propId] = pSize;

    // --- 6. Mean Values ---
    propId = "mean_vals";
    auto pMean = std::make_shared<TypedProperty<QString>>("Mean (R,G,B)", propId, QMetaType::QString, msMeanVals, "Config");
    mvProperty.push_back(pMean);
    mMapIdToProperty[propId] = pMean;

    // --- 7. Norm Values ---
    propId = "norm_vals";
    auto pNorm = std::make_shared<TypedProperty<QString>>("Norm (R,G,B)", propId, QMetaType::QString, msNormVals, "Config");
    mvProperty.push_back(pNorm);
    mMapIdToProperty[propId] = pNorm;

    // --- 8. Class Names ---
    propId = "class_names";
    auto pClasses = std::make_shared<TypedProperty<QString>>("Class Names", propId, QMetaType::QString, msClassNames, "Config");
    mvProperty.push_back(pClasses);
    mMapIdToProperty[propId] = pClasses;

} // [FIXED] Added missing closing brace

NCNNDetectModel::~NCNNDetectModel() {
    if (mpNCNNDetectModelThread) mpNCNNDetectModelThread->abort();
}

unsigned int NCNNDetectModel::nPorts(QtNodes::PortType t) const {
    return (t == QtNodes::PortType::In) ? 1 : 2;
}

QtNodes::NodeDataType NCNNDetectModel::dataType(QtNodes::PortType t, QtNodes::PortIndex i) const {
    if (t == QtNodes::PortType::In) return CVImageData().type();
    return (i == 0) ? CVImageData().type() : SyncData().type();
}

std::shared_ptr<QtNodes::NodeData> NCNNDetectModel::outData(QtNodes::PortIndex p) {
    if (!isEnable()) return nullptr;
    if (p == 0) return std::static_pointer_cast<QtNodes::NodeData>(mpCVImageData);
    return std::static_pointer_cast<QtNodes::NodeData>(mpSyncData);
}

void NCNNDetectModel::setInData(std::shared_ptr<QtNodes::NodeData> d, QtNodes::PortIndex) {
    late_constructor();
    if (d && mpSyncData->data()) {
        mpSyncData->data() = false;
        auto img = std::dynamic_pointer_cast<CVImageData>(d);
        if (img) mpNCNNDetectModelThread->detect(img->data());
    }
}

void NCNNDetectModel::received_result(cv::Mat res) {
    mpCVImageData->set_image(res);
    mpSyncData->data() = true;
    updateAllOutputPorts();
}

void NCNNDetectModel::late_constructor() {
    if (!mpNCNNDetectModelThread) {
        qRegisterMetaType<cv::Mat>("cv::Mat");
        mpNCNNDetectModelThread = new NCNNDetectModelThread(this);
        connect(mpNCNNDetectModelThread, &NCNNDetectModelThread::result_ready, this, &NCNNDetectModel::received_result);
        load_model();
        mpNCNNDetectModelThread->start();
    }
}

std::vector<float> NCNNDetectModel::stringToVecFloat(const QString& str, float defaultVal) {
    std::vector<float> vec = { defaultVal, defaultVal, defaultVal };
    QStringList parts = str.split(",", Qt::SkipEmptyParts);
    for (int i = 0; i < std::min((int)parts.size(), 3); ++i) {
        bool ok;
        float v = parts[i].trimmed().toFloat(&ok);
        if (ok) vec[i] = v;
    }
    return vec;
}

void NCNNDetectModel::load_model(bool) {
    if (msParam_Filename.isEmpty() || msBin_Filename.isEmpty()) {
        qDebug() << "NCNN: Filename is empty!";
        return;
    }

    if (QFile::exists(msParam_Filename) && QFile::exists(msBin_Filename)) {

        NcnnDetectParameters p;
        auto sizeProp = std::static_pointer_cast<TypedProperty<SizePropertyType>>(mMapIdToProperty["size"]);

        p.mTargetSize = cv::Size(sizeProp->getData().miWidth, sizeProp->getData().miHeight);
        p.msInputBlob = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["in_blob"])->getData();
        p.msOutputBlob = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["out_blob"])->getData();

        // --- แก้บรรทัดนี้ครับ (ลบ s ออกจาก msClassNames) ---
        p.mClassNames = std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["class_names"])->getData().split(",", Qt::SkipEmptyParts);
        // ------------------------------------------------

        p.meanVals = stringToVecFloat(std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["mean_vals"])->getData(), 0.f);
        p.normVals = stringToVecFloat(std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["norm_vals"])->getData(), 1.0f / 255.0f);

        if (mpNCNNDetectModelThread) {
            bool success = mpNCNNDetectModelThread->read_net(msParam_Filename, msBin_Filename);

            if (success) {
                qDebug() << "NCNN: Model Loaded Successfully!";
                qDebug() << "NCNN Settings: In =" << p.msInputBlob << ", Out =" << p.msOutputBlob;
            }
            else {
                qDebug() << "NCNN: FAILED to load model! (Check file format or path)";
            }

            mpNCNNDetectModelThread->setParams(p);
        }
    }
    else {
        qDebug() << "NCNN Error: Files do not exist at path!";
        qDebug() << " - Param:" << msParam_Filename;
        qDebug() << " - Bin:" << msBin_Filename;
    }
}

// [FIXED] Removed the duplicate function and ensured full property sync
void NCNNDetectModel::setModelProperty(QString& id, const QVariant& v) {
    PBNodeDelegateModel::setModelProperty(id, v);

    if (!mMapIdToProperty.contains(id)) return;
    auto prop = mMapIdToProperty[id];

    if (id == "param_file") {
        auto typedProp = std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(prop);
        msParam_Filename = v.toString();
        typedProp->getData().msFilename = msParam_Filename;
        load_model();
    }
    else if (id == "bin_file") {
        auto typedProp = std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(prop);
        msBin_Filename = v.toString();
        typedProp->getData().msFilename = msBin_Filename;
        load_model();
    }
    else if (id == "in_blob") { msInputBlob = v.toString(); std::static_pointer_cast<TypedProperty<QString>>(prop)->getData() = msInputBlob; load_model(); }
    else if (id == "out_blob") { msOutputBlob = v.toString(); std::static_pointer_cast<TypedProperty<QString>>(prop)->getData() = msOutputBlob; load_model(); }
    else if (id == "mean_vals") { msMeanVals = v.toString(); std::static_pointer_cast<TypedProperty<QString>>(prop)->getData() = msMeanVals; load_model(); }
    else if (id == "norm_vals") { msNormVals = v.toString(); std::static_pointer_cast<TypedProperty<QString>>(prop)->getData() = msNormVals; load_model(); }
    else if (id == "class_names") { msClassNames = v.toString(); std::static_pointer_cast<TypedProperty<QString>>(prop)->getData() = msClassNames; load_model(); }
    else {
        load_model();
    }
}

QJsonObject NCNNDetectModel::save() const {
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["param_filename"] = msParam_Filename;
    cParams["bin_filename"] = msBin_Filename;
    cParams["in_blob"] = msInputBlob;
    cParams["out_blob"] = msOutputBlob;
    cParams["mean_vals"] = msMeanVals;
    cParams["norm_vals"] = msNormVals;
    cParams["class_names"] = msClassNames;

    modelJson["cParams"] = cParams;
    return modelJson;
}

void NCNNDetectModel::load(QJsonObject const& p) {
    PBNodeDelegateModel::load(p);
    late_constructor();

    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        if (!paramsObj["param_filename"].isNull()) {
            msParam_Filename = paramsObj["param_filename"].toString();
            std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(mMapIdToProperty["param_file"])->getData().msFilename = msParam_Filename;
        }
        if (!paramsObj["bin_filename"].isNull()) {
            msBin_Filename = paramsObj["bin_filename"].toString();
            std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(mMapIdToProperty["bin_file"])->getData().msFilename = msBin_Filename;
        }

        if (!paramsObj["in_blob"].isNull()) {
            msInputBlob = paramsObj["in_blob"].toString();
            std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["in_blob"])->getData() = msInputBlob;
        }
        if (!paramsObj["out_blob"].isNull()) {
            msOutputBlob = paramsObj["out_blob"].toString();
            std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["out_blob"])->getData() = msOutputBlob;
        }
        if (!paramsObj["class_names"].isNull()) {
            msClassNames = paramsObj["class_names"].toString();
            std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["class_names"])->getData() = msClassNames;
        }
        if (!paramsObj["mean_vals"].isNull()) {
            msMeanVals = paramsObj["mean_vals"].toString();
            std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["mean_vals"])->getData() = msMeanVals;
        }
        if (!paramsObj["norm_vals"].isNull()) {
            msNormVals = paramsObj["norm_vals"].toString();
            std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["norm_vals"])->getData() = msNormVals;
        }

        load_model();
    }
}