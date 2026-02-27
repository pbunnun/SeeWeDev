#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QJsonObject>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <vector>

// OpenCV
#include <opencv2/opencv.hpp>

// Node Editor Headers
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "CVImageData.hpp"
#include "InformationData.hpp" // [เพิ่ม] สำหรับพอร์ตข้อมูล JSON (Inf)

// MNN Headers
#include <MNN/Interpreter.hpp>
#include <MNN/Tensor.hpp>
#include <MNN/ImageProcess.hpp>
#include <memory>

// --- [เพิ่ม] โครงสร้างเก็บข้อมูลวัตถุที่จับได้ (แบบเดียวกับ NanoDetObject ของ NCNN) ---
struct MnnDetObject {
    cv::Rect_<float> rect;
    int label;
    float score;
};

struct MnnDetectParameters {
    cv::Size mTargetSize = cv::Size(640, 640);
    QString msInputBlob = "input";
    QString msOutputBlob = "output";
    QStringList mClassNames;
    std::vector<float> meanVals = { 0.f, 0.f, 0.f };
    std::vector<float> normVals = { 1 / 255.f, 1 / 255.f, 1 / 255.f };
};

class MNNDetectModelThread : public QThread {
    Q_OBJECT
public:
    explicit MNNDetectModelThread(QObject* parent = nullptr);
    ~MNNDetectModelThread();

    void abort();
    void detect(const cv::Mat& img);
    bool read_net(const QString& model_path, const QString& input_node, const QString& output_node);
    void setParams(const MnnDetectParameters& p);
    MnnDetectParameters getParams() { QMutexLocker l(&mLockMutex); return mParams; }

Q_SIGNALS:
    // ส่งออกภาพ พร้อมข้อมูล JSON
    void result_ready(cv::Mat res, QString infoText);

protected:
    void run() override;

private:
    // [เพิ่ม] เผื่อไว้ใช้จัดการกล่องซ้อนกัน
    void nms_sorted_bboxes(std::vector<MnnDetObject>& inputs, std::vector<MnnDetObject>& outputs, float nms_threshold);
    float calculate_iou(const MnnDetObject& a, const MnnDetObject& b);

private:
    mutable QMutex mLockMutex;
    QSemaphore mWaitingSemaphore;
    bool mbAbort = false;
    bool mbModelReady = false;

    cv::Mat mCVImage;
    MnnDetectParameters mParams;
    std::shared_ptr<MNN::Interpreter> mNet;
    MNN::Session* mSession = nullptr;
};

class MNNDetectModel : public PBNodeDelegateModel {
    Q_OBJECT
public:
    MNNDetectModel();
    virtual ~MNNDetectModel();

    static const QString _category;
    static const QString _model_name;

    virtual QString caption() const override { return _model_name; }
    virtual QString name() const override { return Name(); }
    static QString Name() { return "MNNDetectModel"; }

    virtual QWidget* embeddedWidget() override { return nullptr; }

    virtual unsigned int nPorts(QtNodes::PortType portType) const override;
    virtual QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    virtual std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;
    virtual void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override;

    virtual QJsonObject save() const override;
    virtual void load(QJsonObject const& p) override;

    // --- [เพิ่ม] ฟังก์ชันวาดภาพส่วนกลาง (ถอดแบบจาก NCNN) ---
    static void draw_object(cv::Mat& frame, float x, float y, float w, float h, int label, float score, const MnnDetectParameters& p);

protected:
    virtual void setModelProperty(QString& id, const QVariant& v) override;

private Q_SLOTS:
    void received_result(cv::Mat res, QString infoText);

private:
    void late_constructor() override;
    void load_model(bool bUpdateDisplayProperties = false); 
    std::vector<float> stringToVecFloat(const QString& str, float defaultVal);

private:
    std::shared_ptr<CVImageData> mpCVImageData;
    std::shared_ptr<InformationData> mpInfData; // [เพิ่ม] กล่องข้อมูลสำหรับพอร์ตที่ 2
    std::shared_ptr<SyncData> mpSyncData;
    
    MNNDetectModelThread* mpMNNDetectModelThread = nullptr;

    QString msModel_Filename;
};