//Copyright © 2025, NECTEC, all rights reserved
//Licensed under the Apache License, Version 2.0

#pragma once

#include <QtCore/QObject>
#include <opencv2/core.hpp>

#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"
#include "CVImagePool.hpp"

using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;

/**
 * @brief Parameters for Lucas-Kanade sparse optical flow
 */
typedef struct CVOpticalFlowPyrLKParameters
{
    // Detection parameters (for generating features)
    bool mbAutoDetectFeatures{true};
    int miMaxCorners{200};
    double mdQualityLevel{0.01};
    double mdMinDistance{10.0};
    int miBlockSize{3};

    // LK parameters
    int miWinSizeWidth{21};
    int miWinSizeHeight{21};
    int miMaxLevel{3};
    int miMaxCount{30};
    double mdEpsilon{0.01};
    int miFlags{0};
    double mdMinEigThreshold{1e-4};

    // Visualization
    bool mbDrawTracks{true};
    double mdMotionScale{1.0};
    bool mbDrawArrows{true};
    int miTrackColorB{0};
    int miTrackColorG{255};
    int miTrackColorR{0};
    int miTrackThickness{2};
} CVOpticalFlowPyrLKParameters;

Q_DECLARE_METATYPE(CVOpticalFlowPyrLKParameters)

/**
 * @brief Worker for PyrLK sparse optical flow
 */
class CVOpticalFlowPyrLKWorker : public QObject
{
    Q_OBJECT

public:
    CVOpticalFlowPyrLKWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat currentFrame,
                      cv::Mat previousFrame,
                      CVOpticalFlowPyrLKParameters params,
                      FrameSharingMode mode,
                      std::shared_ptr<CVImagePool> pool,
                      long frameId,
                      QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> outputImage);
};

/**
 * @brief Lucas-Kanade sparse optical flow model with async processing
 */
class CVOpticalFlowPyrLKModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVOpticalFlowPyrLKModel();
    ~CVOpticalFlowPyrLKModel() override = default;

    QJsonObject save() const override;
    void load(const QJsonObject& json) override;

    QWidget* embeddedWidget() override { return nullptr; }

    void setModelProperty(QString& id, const QVariant& value) override;

    QPixmap minPixmap() const override { return _minPixmap; }

    static const QString _category;
    static const QString _model_name;

protected:
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;

private:
    void process_cached_input() override;

    CVOpticalFlowPyrLKParameters mParams;
    QPixmap _minPixmap;

    // Pending data for backpressure
    cv::Mat mPendingCurrentFrame;
    cv::Mat mPendingPreviousFrame;
    CVOpticalFlowPyrLKParameters mPendingParams;

    cv::Mat mPreviousFrame;
    bool mbHasPreviousFrame{false};
};

