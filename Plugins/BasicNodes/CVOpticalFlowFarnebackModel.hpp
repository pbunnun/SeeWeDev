//Copyright © 2025, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#pragma once

#include <QtCore/QObject>

#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"
#include "CVImagePool.hpp"
#include "SyncData.hpp"

using CVDevLibrary::CVImagePool;
using CVDevLibrary::FrameSharingMode;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;

/**
 * @brief Parameters for Farneback dense optical flow computation
 */
typedef struct CVOpticalFlowFarnebackParameters
{
    double mdPyrScale{0.5};
    int miLevels{3};
    int miWinsize{15};
    int miIterations{3};
    int miPolyN{5};
    double mdPolySigma{1.1};
    int miFlags{0};
    bool mbShowMagnitude{false};
    int miColorMapType{2};
} CVOpticalFlowFarnebackParameters;

Q_DECLARE_METATYPE(CVOpticalFlowFarnebackParameters)

/**
 * @brief Worker for Farneback optical flow computation
 */
class CVOpticalFlowFarnebackWorker : public QObject
{
    Q_OBJECT

public:
    CVOpticalFlowFarnebackWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat currentFrame,
                      cv::Mat previousFrame,
                      CVOpticalFlowFarnebackParameters params,
                      FrameSharingMode mode,
                      std::shared_ptr<CVImagePool> pool,
                      long frameId,
                      QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> outputImage);
};

/**
 * @brief Farneback dense optical flow model with async processing
 */
class CVOpticalFlowFarnebackModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVOpticalFlowFarnebackModel();
    ~CVOpticalFlowFarnebackModel() override = default;

    QJsonObject save() const override;
    void load(const QJsonObject &json) override;

    QWidget *embeddedWidget() override { return nullptr; }

    void setModelProperty(QString &id, const QVariant &value) override;

    QPixmap minPixmap() const override { return _minPixmap; }

    static const QString _category;
    static const QString _model_name;

protected:
    QObject *createWorker() override;
    void connectWorker(QObject *worker) override;
    void dispatchPendingWork() override;

private:
    void process_cached_input() override;

    CVOpticalFlowFarnebackParameters mParams;
    QPixmap _minPixmap;

    // Pending data for backpressure
    cv::Mat mPendingCurrentFrame;
    cv::Mat mPendingPreviousFrame;
    CVOpticalFlowFarnebackParameters mPendingParams;

    cv::Mat mPreviousFrame;
    bool mbHasPreviousFrame{false};
};
