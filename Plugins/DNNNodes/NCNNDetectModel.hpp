#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QJsonObject>
#include <QVariant>
#include <QString>
#include <vector>

// OpenCV
#include <opencv2/opencv.hpp>

// Node Editor Headers
#include "PBNodeDelegateModel.hpp"
#include "SyncData.hpp"
#include "CVImageData.hpp"
#include "InformationData.hpp"

// NCNN Header
#include "net.h"

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

    float mConfThreshold = 0.45f;
};

class NCNNDetectModelThread : public QThread {
    Q_OBJECT
public:
    explicit NCNNDetectModelThread(QObject* parent = nullptr);
    ~NCNNDetectModelThread();

    void decode_yolov8(const ncnn::Mat& out, int img_w, int img_h, std::vector<NanoDetObject>& objects);
    void run() override;
    void detect(const cv::Mat& img);
    bool read_net(const QString& param, const QString& bin);
    void setParams(const NcnnDetectParameters& p);
    void abort();

Q_SIGNALS:
    void result_ready(cv::Mat result, QString inf_data);

private:
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

class NCNNDetectModel : public PBNodeDelegateModel {
    Q_OBJECT
public:
    NCNNDetectModel();
    virtual ~NCNNDetectModel();

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

    // [แยกออกมาเป็นฟังก์ชันส่วนกลาง]
    static void draw_object(cv::Mat& frame, float x, float y, float w, float h, int label, float score, const NcnnDetectParameters& p);

private Q_SLOTS:
    void received_result(cv::Mat result, QString inf_data);

private:
    void late_constructor() override;
    void load_model(bool bUpdateDisplayProperties = false);
    std::vector<float> stringToVecFloat(const QString& str, float defaultVal);

private:
    std::shared_ptr<CVImageData> mpCVImageData;
    std::shared_ptr<InformationData> mpInfData;
    std::shared_ptr<SyncData> mpSyncData;
    
    NCNNDetectModelThread* mpNCNNDetectModelThread = nullptr;

    QString msParam_Filename;
    QString msBin_Filename;
    QString msInputBlob;
    QString msOutputBlob;
    QString msMeanVals;
    QString msNormVals;
    QString msClassNames;
    QString msConfThresh;       
};