#include "NCNNDetectModel.hpp"
#include "CVImageData.hpp"
#include "InformationData.hpp"
#include "qtvariantproperty_p.h"

#include <QFile>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QDebug> 

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
        QElapsedTimer etimer;
        etimer.start();

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

        QString result_information = "";
        if (!out.empty()) {
            std::vector<NanoDetObject> objects;
            if (out.w == 6) {
                for (int i = 0; i < out.h; i++) {
                    const float* v = out.row(i);
                    // ใช้ค่า Confidence จาก Property
                    if (v[1] > p.mConfThreshold) {
                        NanoDetObject obj; obj.label = (int)v[0]; obj.score = v[1];
                        obj.rect = cv::Rect_<float>(v[2], v[3], v[4], v[5]);
                        objects.push_back(obj);
                    }
                }
                std::sort(objects.begin(), objects.end(), [](const NanoDetObject& a, const NanoDetObject& b) { return a.score > b.score; });
            }
            else if (out.h == 5 || out.w == 8400) decode_yolov8(out, p.mTargetSize.width, p.mTargetSize.height, objects);
            else decode_yolov5_raw(out, p.mTargetSize.width, p.mTargetSize.height, objects);

            for (auto& obj : objects) {
                NCNNDetectModel::draw_object(frame, obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height, obj.label, obj.score, p);
            }

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
            result_information += "    ]\n";
            result_information += "}";

        } else {
            result_information = "{\n    \"Total Leads\" : 0,\n    \"Details\" : []\n}";
        }

        Q_EMIT result_ready(frame, result_information);
    }
}

void NCNNDetectModelThread::detect(const cv::Mat& img) { if (mLockMutex.tryLock()) { img.copyTo(mCVImage); mWaitingSemaphore.release(); mLockMutex.unlock(); } }
bool NCNNDetectModelThread::read_net(const QString& param, const QString& bin) { 
    QMutexLocker l(&mLockMutex); mNet.clear();
    if (mNet.load_param(param.toLocal8Bit().data()) == 0 && mNet.load_model(bin.toLocal8Bit().data()) == 0) return mbModelReady = true;
    return mbModelReady = false;
}
void NCNNDetectModelThread::setParams(const NcnnDetectParameters& p) { QMutexLocker l(&mLockMutex); mParams = p; }

void NCNNDetectModelThread::decode_yolov5_raw(const ncnn::Mat& out, int /*img_w*/, int /*img_h*/, std::vector<NanoDetObject>& objects) {
    mLockMutex.lock();
    float conf_thresh = mParams.mConfThreshold;
    mLockMutex.unlock();

    std::vector<NanoDetObject> proposals;
    for (int i = 0; i < out.h; i++) {
        const float* ptr = out.row(i);
        if (ptr[4] > conf_thresh) {
            float max_score = -1.f; int label = 0;
            for (int j = 5; j < out.w; j++) { if (ptr[j] > max_score) { max_score = ptr[j]; label = j - 5; } }
            if (ptr[4] * max_score > conf_thresh) {
                NanoDetObject obj; obj.rect = cv::Rect_<float>(ptr[0] - ptr[2] / 2, ptr[1] - ptr[3] / 2, ptr[2], ptr[3]);
                obj.label = label; obj.score = ptr[4] * max_score; proposals.push_back(obj);
            }
        }
    }
    nms_sorted_bboxes(proposals, objects, 0.3f);
}

void NCNNDetectModelThread::decode_yolov8(const ncnn::Mat& out, int /*img_w*/, int /*img_h*/, std::vector<NanoDetObject>& objects) {
    mLockMutex.lock();
    float conf_thresh = mParams.mConfThreshold;
    mLockMutex.unlock();

    std::vector<NanoDetObject> proposals;
    const int num_anchors = out.w; 
    const int num_classes = out.h - 4;

    const float* ptr_cx = out.row(0); 
    const float* ptr_cy = out.row(1); 
    const float* ptr_w = out.row(2); 
    const float* ptr_h = out.row(3);

    for (int i = 0; i < num_anchors; i++) {
        float max_score = -1.f;
        int label = -1;

        for (int c = 0; c < num_classes; c++) {
            float score = out.row(4 + c)[i];
            if (score > max_score) {
                max_score = score;
                label = c;
            }
        }

        if (max_score > conf_thresh) { 
            NanoDetObject obj;
            obj.rect.x = ptr_cx[i] - ptr_w[i] * 0.5f;
            obj.rect.y = ptr_cy[i] - ptr_h[i] * 0.5f;
            obj.rect.width = ptr_w[i];
            obj.rect.height = ptr_h[i];
            obj.label = label;
            obj.score = max_score;
            proposals.push_back(obj);
        }
    } 

    nms_sorted_bboxes(proposals, objects, 0.25f); 
}

void NCNNDetectModelThread::nms_sorted_bboxes(std::vector<NanoDetObject>& inputs, std::vector<NanoDetObject>& outputs, float nms_threshold) {
    outputs.clear(); std::sort(inputs.begin(), inputs.end(), [](const NanoDetObject& a, const NanoDetObject& b) { return a.score > b.score; });
    std::vector<bool> suppressed(inputs.size(), false);
    for (size_t i = 0; i < inputs.size(); i++) {
        if (suppressed[i]) continue;
        outputs.push_back(inputs[i]);
        for (size_t j = i + 1; j < inputs.size(); j++) { if (!suppressed[j] && calculate_iou(inputs[i], inputs[j]) > nms_threshold) suppressed[j] = true; }
    }
}

float NCNNDetectModelThread::calculate_iou(const NanoDetObject& a, const NanoDetObject& b) {
    float inter = (a.rect & b.rect).area(); return inter / (a.rect.area() + b.rect.area() - inter);
}

// =============================================================================
// Part 2: Node Model Implementation
// =============================================================================

NCNNDetectModel::NCNNDetectModel() : PBNodeDelegateModel(Name()) {
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mpInfData = std::make_shared<InformationData>(); 
    mpSyncData = std::make_shared<SyncData>(true);

    // กำหนดค่า Default ไว้ที่นี่
    msInputBlob = "in0"; 
    msOutputBlob = "out0"; 
    msMeanVals = "0,0,0"; 
    msNormVals = "0.00392,0.00392,0.00392"; 
    msClassNames = "person";
    msConfThresh = "0.45";

    FilePathPropertyType fType; fType.msFilter = "*.param"; fType.msMode = "open";
    auto pParam = std::make_shared<TypedProperty<FilePathPropertyType>>("Model Param", "param_file", QtVariantPropertyManager::filePathTypeId(), fType);
    mvProperty.push_back(pParam); mMapIdToProperty["param_file"] = pParam;

    fType.msFilter = "*.bin";
    auto pBin = std::make_shared<TypedProperty<FilePathPropertyType>>("Model Bin", "bin_file", QtVariantPropertyManager::filePathTypeId(), fType);
    mvProperty.push_back(pBin); mMapIdToProperty["bin_file"] = pBin;

    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Input Blob", "in_blob", QMetaType::QString, msInputBlob, "Network"));
    mMapIdToProperty["in_blob"] = mvProperty.back();
    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Output Blob", "out_blob", QMetaType::QString, msOutputBlob, "Network"));
    mMapIdToProperty["out_blob"] = mvProperty.back();

    SizePropertyType sType; sType.miWidth = 640; sType.miHeight = 640;
    mvProperty.push_back(std::make_shared<TypedProperty<SizePropertyType>>("Target Size", "size", QMetaType::QSize, sType, "Config"));
    mMapIdToProperty["size"] = mvProperty.back();

    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Confidence", "conf_thresh", QMetaType::QString, msConfThresh, "Config"));
    mMapIdToProperty["conf_thresh"] = mvProperty.back();

    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Mean (R,G,B)", "mean_vals", QMetaType::QString, msMeanVals, "Config"));
    mMapIdToProperty["mean_vals"] = mvProperty.back();
    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Norm (R,G,B)", "norm_vals", QMetaType::QString, msNormVals, "Config"));
    mMapIdToProperty["norm_vals"] = mvProperty.back();
    mvProperty.push_back(std::make_shared<TypedProperty<QString>>("Class Names", "class_names", QMetaType::QString, msClassNames, "Config"));
    mMapIdToProperty["class_names"] = mvProperty.back();
}

NCNNDetectModel::~NCNNDetectModel() { if (mpNCNNDetectModelThread) mpNCNNDetectModelThread->abort(); }

unsigned int NCNNDetectModel::nPorts(QtNodes::PortType t) const { return (t == QtNodes::PortType::In) ? 1 : 3; }
QtNodes::NodeDataType NCNNDetectModel::dataType(QtNodes::PortType t, QtNodes::PortIndex i) const {
    if (t == QtNodes::PortType::In) return CVImageData().type();
    if (i == 0) return CVImageData().type(); else if (i == 1) return InformationData().type(); return SyncData().type();
}
std::shared_ptr<QtNodes::NodeData> NCNNDetectModel::outData(QtNodes::PortIndex p) {
    if (isEnable()) { if (p == 0) return mpCVImageData; else if (p == 1) return mpInfData; return mpSyncData; } return nullptr;
}

void NCNNDetectModel::setInData(std::shared_ptr<QtNodes::NodeData> d, QtNodes::PortIndex) {
    late_constructor(); if (!isEnable()) return;
    if (d && mpSyncData->data()) {
        mpSyncData->data() = false; auto img = std::dynamic_pointer_cast<CVImageData>(d);
        if (img) { mpInfData->set_timestamp(img->timestamp()); mpNCNNDetectModelThread->detect(img->data()); }
    }
}

void NCNNDetectModel::received_result(cv::Mat res, QString text) {
    mpCVImageData->set_image(res); mpInfData->set_information(text); mpSyncData->data() = true; updateAllOutputPorts();
}

void NCNNDetectModel::late_constructor() {
    if (!mpNCNNDetectModelThread) {
        qRegisterMetaType<cv::Mat>("cv::Mat"); mpNCNNDetectModelThread = new NCNNDetectModelThread(this);
        connect(mpNCNNDetectModelThread, &NCNNDetectModelThread::result_ready, this, &NCNNDetectModel::received_result);
        load_model(); mpNCNNDetectModelThread->start();
    }
}

QJsonObject NCNNDetectModel::save() const {
    QJsonObject modelJson = PBNodeDelegateModel::save();
    QJsonObject cParams;
    cParams["param_filename"] = msParam_Filename;
    cParams["bin_filename"] = msBin_Filename;
    cParams["in_blob"] = msInputBlob;
    cParams["out_blob"] = msOutputBlob;
    cParams["class_names"] = msClassNames;
    cParams["mean_vals"] = msMeanVals;
    cParams["norm_vals"] = msNormVals;
    cParams["conf_thresh"] = msConfThresh;

    auto propSize = std::static_pointer_cast<TypedProperty<SizePropertyType>>(mMapIdToProperty.value("size"));
    if (propSize) {
        cParams["target_w"] = propSize->getData().miWidth;
        cParams["target_h"] = propSize->getData().miHeight;
    }

    modelJson["cParams"] = cParams;
    return modelJson;
}

void NCNNDetectModel::load(QJsonObject const& p) {
    PBNodeDelegateModel::load(p);
    late_constructor();
    QJsonObject paramsObj = p["cParams"].toObject();
    if (!paramsObj.isEmpty()) {
        if (paramsObj.contains("param_filename")) {
            msParam_Filename = paramsObj["param_filename"].toString();
            if (mMapIdToProperty.contains("param_file"))
                std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(mMapIdToProperty["param_file"])->getData().msFilename = msParam_Filename;
        }
        if (paramsObj.contains("bin_filename")) {
            msBin_Filename = paramsObj["bin_filename"].toString();
            if (mMapIdToProperty.contains("bin_file"))
                std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(mMapIdToProperty["bin_file"])->getData().msFilename = msBin_Filename;
        }

        // ⭐ เพิ่ม if (paramsObj.contains(...)) คลุมไว้ทุกตัว
        if (paramsObj.contains("in_blob")) {
            msInputBlob = paramsObj["in_blob"].toString();
            if (mMapIdToProperty.contains("in_blob")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["in_blob"])->getData() = msInputBlob;
        }
        if (paramsObj.contains("out_blob")) {
            msOutputBlob = paramsObj["out_blob"].toString();
            if (mMapIdToProperty.contains("out_blob")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["out_blob"])->getData() = msOutputBlob;
        }
        if (paramsObj.contains("class_names")) {
            msClassNames = paramsObj["class_names"].toString();
            if (mMapIdToProperty.contains("class_names")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["class_names"])->getData() = msClassNames;
        }
        if (paramsObj.contains("mean_vals")) {
            msMeanVals = paramsObj["mean_vals"].toString();
            if (mMapIdToProperty.contains("mean_vals")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["mean_vals"])->getData() = msMeanVals;
        }
        if (paramsObj.contains("norm_vals")) {
            msNormVals = paramsObj["norm_vals"].toString();
            if (mMapIdToProperty.contains("norm_vals")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["norm_vals"])->getData() = msNormVals;
        }
        if (paramsObj.contains("conf_thresh")) {
            msConfThresh = paramsObj["conf_thresh"].toString();
            if (mMapIdToProperty.contains("conf_thresh")) std::static_pointer_cast<TypedProperty<QString>>(mMapIdToProperty["conf_thresh"])->getData() = msConfThresh;
        }

        if (paramsObj.contains("target_w") && mMapIdToProperty.contains("size")) {
            auto prop = std::static_pointer_cast<TypedProperty<SizePropertyType>>(mMapIdToProperty["size"]);
            prop->getData().miWidth = paramsObj["target_w"].toInt();
            prop->getData().miHeight = paramsObj["target_h"].toInt();
        }

        load_model();
    }
}

void NCNNDetectModel::setModelProperty(QString& id, const QVariant& v) {
    PBNodeDelegateModel::setModelProperty(id, v);
    if (!mMapIdToProperty.contains(id)) return;
    auto prop = mMapIdToProperty[id];

    if (id == "param_file") {
        msParam_Filename = v.toString();
        std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(prop)->getData().msFilename = msParam_Filename;
    } else if (id == "bin_file") {
        msBin_Filename = v.toString();
        std::static_pointer_cast<TypedProperty<FilePathPropertyType>>(prop)->getData().msFilename = msBin_Filename;
    } 
    // มัดรวมตัวแปรที่เป็น Text กลุ่มเดียวกันไว้ในบล็อกเดียว
    else if (id == "in_blob" || id == "out_blob" || id == "class_names" || id == "mean_vals" || id == "norm_vals" || id == "conf_thresh") {
        QString val = v.toString();
        std::static_pointer_cast<TypedProperty<QString>>(prop)->getData() = val;
        
        if (id == "in_blob") msInputBlob = val;
        else if (id == "out_blob") msOutputBlob = val;
        else if (id == "class_names") msClassNames = val;
        else if (id == "mean_vals") msMeanVals = val;
        else if (id == "norm_vals") msNormVals = val;
        else if (id == "conf_thresh") msConfThresh = val;
    } 
    else if (id == "size") {
        if (v.canConvert<QSize>()) {
            QSize newSize = v.toSize();
            if (newSize.width() > 0 && newSize.height() > 0) {
                auto sizeProp = std::static_pointer_cast<TypedProperty<SizePropertyType>>(prop);
                sizeProp->getData().miWidth = newSize.width();
                sizeProp->getData().miHeight = newSize.height();
            }
        }
    }
    
    load_model(true);
}

void NCNNDetectModel::draw_object(cv::Mat& frame, float x, float y, float w, float h, int label, float score, const NcnnDetectParameters& p) {
    float x1 = x * frame.cols / p.mTargetSize.width, y1 = y * frame.rows / p.mTargetSize.height;
    float w1 = w * frame.cols / p.mTargetSize.width, h1 = h * frame.rows / p.mTargetSize.height;
    cv::Rect roi = cv::Rect(x1, y1, w1, h1) & cv::Rect(0, 0, frame.cols, frame.rows);
    if (roi.width <= 0 || roi.height <= 0) return;

    int dynamic_thickness = std::max(1, (int)(frame.cols * 0.002)); 
    double dynamic_font_scale = frame.cols * 0.0007; 
    
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    int padding = std::max(5, (int)(frame.cols * 0.01)); 

    cv::rectangle(frame, roi, cv::Scalar(0, 255, 0), dynamic_thickness);

    std::string text = QString("%1: %2").arg((label < (int)p.mClassNames.size()) ? p.mClassNames[label] : QString::number(label)).arg(score, 0, 'f', 2).toStdString();

    int baseLine = 0;
    cv::Size textSize = cv::getTextSize(text, fontFace, dynamic_font_scale, dynamic_thickness, &baseLine);
    
    cv::Rect bgRect(roi.x, roi.y - textSize.height - padding, textSize.width + padding, textSize.height + padding);
    cv::rectangle(frame, bgRect, cv::Scalar(0, 100, 0), -1);

    cv::putText(frame, text, cv::Point(roi.x + (padding/2), roi.y - (padding/2)), 
                fontFace, dynamic_font_scale, cv::Scalar(255, 255, 255), dynamic_thickness, cv::LINE_AA);
}

std::vector<float> NCNNDetectModel::stringToVecFloat(const QString& str, float defaultVal) {
    std::vector<float> vec = { defaultVal, defaultVal, defaultVal };
    QStringList parts = str.split(",", Qt::SkipEmptyParts);
    for (int i = 0; i < std::min((int)parts.size(), 3); ++i) {
        bool ok; float v = parts[i].trimmed().toFloat(&ok); if (ok) vec[i] = v;
    }
    return vec;
}

void NCNNDetectModel::load_model(bool) {
    if (msParam_Filename.isEmpty() || msBin_Filename.isEmpty()) return;
    if (QFile::exists(msParam_Filename) && QFile::exists(msBin_Filename)) {
        NcnnDetectParameters p;
        auto propSize = std::static_pointer_cast<TypedProperty<SizePropertyType>>(mMapIdToProperty["size"]);
        p.mTargetSize = cv::Size(propSize->getData().miWidth, propSize->getData().miHeight);
        p.msInputBlob = msInputBlob; p.msOutputBlob = msOutputBlob;
        p.mClassNames = msClassNames.split(",", Qt::SkipEmptyParts);
        p.meanVals = stringToVecFloat(msMeanVals, 0.f);
        p.normVals = stringToVecFloat(msNormVals, 1.0f/255.0f);
        
        // แปลงค่าจาก String ที่รับมาจาก UI ให้เป็น Float ก่อนส่งไปให้ AI ประมวลผล
        p.mConfThreshold = msConfThresh.toFloat();

        if (mpNCNNDetectModelThread) {
            mpNCNNDetectModelThread->read_net(msParam_Filename, msBin_Filename);
            mpNCNNDetectModelThread->setParams(p);
        }
    }
}