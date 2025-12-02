// Hough Lines Point Set model - detects lines from explicit point set.
#pragma once

#include <QtCore/QObject>
#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"
#include "SyncData.hpp"
#include "CVImagePool.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;

struct CVHoughLinesPointSetParams {
    int miLinesMax {64};
    int miThreshold {50};
    double mdMinRho {-200.0};
    double mdMaxRho {200.0};
    double mdRhoStep {1.0};
    double mdMinThetaDeg {0.0};
    double mdMaxThetaDeg {180.0};
    double mdThetaStepDeg {1.0};
    bool mbDisplayLines {true};
    bool mbStrongestOnly {true};
    unsigned char mucLineColor[3] {255,0,255}; // B,G,R (magenta)
    int miLineThickness {1};
    int miLineType {cv::LINE_8};
};

Q_DECLARE_METATYPE(CVHoughLinesPointSetParams)

class CVHoughLinesPointSetWorker : public QObject {
    Q_OBJECT
public:
    CVHoughLinesPointSetWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                      CVHoughLinesPointSetParams params,
                      FrameSharingMode mode,
                      std::shared_ptr<CVImagePool> pool,
                      long frameId,
                      QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img, std::shared_ptr<IntegerData> count);
};

class CVHoughLinesPointSetModel : public PBAsyncDataModel {
    Q_OBJECT
public:
    CVHoughLinesPointSetModel();
    ~CVHoughLinesPointSetModel() override {}

    QJsonObject save() const override;
    void load(const QJsonObject &p) override;

    QWidget* embeddedWidget() override { return nullptr; }
    void setModelProperty(QString& id, const QVariant& value) override;

    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;

    static const QString _category;
    static const QString _model_name;

protected:
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;

private:
    void process_cached_input() override;

    CVHoughLinesPointSetParams mParams;
    std::shared_ptr<IntegerData> mpIntegerData { nullptr };
    QPixmap _minPixmap;
    cv::Mat mPendingFrame;
    CVHoughLinesPointSetParams mPendingParams;
    static const std::string color[3];
};
