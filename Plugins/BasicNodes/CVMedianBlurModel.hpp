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
#include <opencv2/core.hpp>

#include <PBAsyncDataModel.hpp>
#include <CVImageData.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @brief Median blur parameters
 * 
 * Kernel size must be odd and > 1 (3, 5, 7, 9, etc.)
 */
typedef struct CVMedianBlurParameters {
    int miKernelSize;
    
    CVMedianBlurParameters()
        : miKernelSize(5)
    {
    }
} CVMedianBlurParameters;

/**
 * @brief Worker for async median blur processing
 */
class CVMedianBlurWorker : public QObject
{
    Q_OBJECT
public:
    explicit CVMedianBlurWorker(QObject *parent = nullptr) : QObject(parent) {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     CVMedianBlurParameters params,
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);
};

/**
 * @brief Median blur node model
 * 
 * Applies median blur for noise reduction while preserving edges.
 * Particularly effective for salt-and-pepper noise.
 */
class CVMedianBlurModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVMedianBlurModel();
    virtual ~CVMedianBlurModel() override = default;

    QJsonObject save() const override;
    void load(const QJsonObject &p) override;

    QWidget * embeddedWidget() override { return nullptr; }
    void setModelProperty(QString &id, const QVariant &value) override;

    static const QString _category;
    static const QString _model_name;

protected:
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;

private:
    void process_cached_input() override;

    CVMedianBlurParameters mParams;

    // Pending frame state
    cv::Mat mPendingFrame;
    CVMedianBlurParameters mPendingParams;
};
