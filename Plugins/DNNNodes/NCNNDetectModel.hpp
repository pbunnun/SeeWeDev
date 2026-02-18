#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QJsonObject>
#include <QVariant>
#include <vector>

// OpenCV
#include <opencv2/opencv.hpp>

// Node Editor Headers
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "CVImageData.hpp"

// NCNN Header
#include "net.h"

// ... (Struct NanoDetObject และ NcnnDetectParameters คงเดิม) ...
struct NanoDetObject {
    cv::Rect_<float> rect;
    int label;
    float score;
};

struct NcnnDetectParameters {
    QString msInputBlob = "in0";
    QString msOutputBlob = "out0";
    cv::Size mTargetSize = cv::Size(640, 640);
    QStringList mClassNames;
    std::vector<float> meanVals = { 0.f, 0.f, 0.f };
    std::vector<float> normVals = { 1 / 255.f, 1 / 255.f, 1 / 255.f };
};

// ... (Class NCNNDetectModelThread คงเดิม) ...
class NCNNDetectModelThread : public QThread {
    Q_OBJECT
public:
    explicit NCNNDetectModelThread(QObject* parent = nullptr);
    ~NCNNDetectModelThread();

    //เพิ่มของ yolo v8
    void decode_yolov8(const ncnn::Mat& out, int img_w, int img_h, std::vector<NanoDetObject>& objects);

    void run() override;
    void detect(const cv::Mat& img);
    bool read_net(const QString& param, const QString& bin);
    void setParams(const NcnnDetectParameters& p);
    // NcnnDetectParameters getParams(); // <-- อันนี้ถ้าไม่ได้ใช้ใน .cpp ลบออกก็ได้ครับ
    void abort();

Q_SIGNALS:
    void result_ready(cv::Mat result);

private:
    void draw_object(cv::Mat& frame, float x, float y, float w, float h, int label, float score, const NcnnDetectParameters& p);
    void decode_yolov5_raw(const ncnn::Mat& out, int img_w, int img_h, std::vector<NanoDetObject>& objects);
    void nms_sorted_bboxes(std::vector<NanoDetObject>& inputs, std::vector<NanoDetObject>& outputs, float nms_threshold);
    float calculate_iou(const NanoDetObject& a, const NanoDetObject& b);

private:
    mutable QMutex mLockMutex;
    QSemaphore mWaitingSemaphore;
    bool mbAbort = false;
    bool mbModelReady = false;

    cv::Mat mCVImage;
    ncnn::Net mNet;
    NcnnDetectParameters mParams;
};

// ... (Class NCNNDetectModel) ...
class NCNNDetectModel : public PBNodeDelegateModel {
    Q_OBJECT
public:
    NCNNDetectModel();
    virtual ~NCNNDetectModel();

    // ... (Public functions คงเดิม) ...
    virtual QWidget* embeddedWidget() override { return nullptr; }

    static const QString _model_name;
    static const QString _category;

    static QString Name() { return _model_name; }
    virtual QString caption() const override { return Name(); }
    virtual QString name() const override { return Name(); }

    virtual unsigned int nPorts(QtNodes::PortType portType) const override;
    virtual NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    virtual std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override;
    virtual void setInData(std::shared_ptr<NodeData> nodeData, QtNodes::PortIndex port) override;

    virtual QJsonObject save() const override;
    virtual void load(QJsonObject const& p) override;
    virtual void setModelProperty(QString& id, const QVariant& value) override;

private Q_SLOTS:
    void received_result(cv::Mat result);

private:
    void late_constructor() override;
    void load_model(bool bUpdateDisplayProperties = false);

    // Helper function ที่เราเพิ่มเข้ามาใหม่
    std::vector<float> stringToVecFloat(const QString& str, float defaultVal);
    QStringList readClassFile(const QString& path); // <-- เพิ่มอันนี้ถ้าจะใช้ไฟล์ class.txt

    // ฟังก์ชันพวกนี้ถ้าไม่ได้ใช้ใน .cpp แล้ว ให้ลบออกได้เลยครับเพื่อความสะอาด
    // void processData(const std::shared_ptr<CVImageData>& in);
    // void autoDetectBlobs(const QString& filePath);
    // bool runOptimizeProcess(const QString& inputParam, const QString& inputBin);

private:
    std::shared_ptr<CVImageData> mpCVImageData;
    std::shared_ptr<SyncData> mpSyncData;
    NCNNDetectModelThread* mpNCNNDetectModelThread = nullptr;

    // --- [ส่วนที่ต้องเพิ่ม!] ตัวแปรสำหรับจำค่า Setting ---
    QString msParam_Filename;
    QString msBin_Filename;

    // เพิ่มให้ครบตามที่ใช้ใน Constructor
    QString msInputBlob;
    QString msOutputBlob;
    QString msMeanVals;
    QString msNormVals;
    QString msClassNames;       // สำหรับเก็บรายชื่อ (person, car)
    QString msClass_Filename;   // สำหรับเก็บ Path ไฟล์ (.txt)
};