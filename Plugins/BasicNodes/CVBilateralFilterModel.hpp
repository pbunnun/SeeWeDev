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
 * @brief Bilateral filter parameters
 * 
 * Bilateral filter smooths images while preserving edges.
 * It considers both spatial distance and intensity difference.
 */
typedef struct CVBilateralFilterParameters {
    int miDiameter;          // Diameter of pixel neighborhood (0 = auto from sigma)
    double mdSigmaColor;     // Filter sigma in color space
    double mdSigmaSpace;     // Filter sigma in coordinate space
    
    CVBilateralFilterParameters()
        : miDiameter(9)
        , mdSigmaColor(75.0)
        , mdSigmaSpace(75.0)
    {
    }
} CVBilateralFilterParameters;

/**
 * @brief Worker for async bilateral filter processing
 */
class CVBilateralFilterWorker : public QObject
{
    Q_OBJECT
public:
    explicit CVBilateralFilterWorker(QObject *parent = nullptr) : QObject(parent) {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     CVBilateralFilterParameters params,
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);
};

/**
 * @brief Bilateral filter node model
 * 
 * Applies bilateral filtering for edge-preserving smoothing.
 * Effective for noise reduction while maintaining sharp edges.
 */
class CVBilateralFilterModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVBilateralFilterModel();
    virtual ~CVBilateralFilterModel() override = default;

    QJsonObject save() const override;
    void load(const QJsonObject &p) override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;
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

    CVBilateralFilterParameters mParams;

    // Pending frame state
    cv::Mat mPendingFrame;
    CVBilateralFilterParameters mPendingParams;
};
