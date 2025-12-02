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

/**
 * @file CVNormalizationModel.hpp
 * @brief Provides image intensity normalization and range scaling operations.
 *
 * This file implements a node that normalizes image pixel values to a specified range
 * using OpenCV's cv::normalize function. Normalization is essential for:
 * - Standardizing image intensity ranges across different sources
 * - Improving visualization (expanding contrast to full display range)
 * - Preprocessing for algorithms expecting specific value ranges
 * - Converting between different bit depths or representations
 *
 * Normalization Types Supported:
 *
 * 1. NORM_MINMAX (Most Common):
 *    - Linearly scale values to [min, max] range
 *    - Formula: out = (in - in_min) × (max - min) / (in_max - in_min) + min
 *    - Use case: Display enhancement, range conversion
 *    - Example: [50, 200] → [0, 255]
 *
 * 2. NORM_INF (Infinity Norm):
 *    - Scale by maximum absolute value
 *    - out = in / max(|in|)
 *    - Use case: Normalize vectors, preserve relative magnitudes
 *
 * 3. NORM_L1 (L1 Norm):
 *    - Scale so sum of absolute values = 1
 *    - out = in / Σ|in|
 *    - Use case: Probability distributions, feature vectors
 *
 * 4. NORM_L2 (L2 Norm):
 *    - Scale so Euclidean norm = 1
 *    - out = in / √(Σin²)
 *    - Use case: Unit vectors, normalized feature descriptors
 *
 * Common Applications:
 * - Contrast enhancement (expand narrow intensity range to [0, 255])
 * - 16-bit to 8-bit conversion (map [0, 65535] → [0, 255])
 * - Float image display (map [0.0, 1.0] → [0, 255])
 * - Preprocessing for machine learning (standardize input ranges)
 * - Histogram equalization alternative (simple linear scaling)
 *
 * The node supports optional input ports for dynamic range specification,
 * allowing runtime control of normalization parameters.
 *
 * @see CVNormalizationModel, cv::normalize, cv::NormTypes
 */

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
 * @struct NormalizationParameters
 * @brief Configuration parameters for normalization operation.
 *
 * Controls the target range and normalization method:
 *
 * mdRangeMax / mdRangeMin:
 * - Target value range after normalization
 * - For NORM_MINMAX: Output will be in [mdRangeMin, mdRangeMax]
 * - For other norms: Interpretation varies
 * - Common settings:
 *   * [0, 255]: Standard 8-bit display range
 *   * [0.0, 1.0]: Normalized float range
 *   * [-1.0, 1.0]: Signed normalized range
 *
 * miNormType:
 * - NORM_MINMAX: Linear rescaling to [min, max]
 * - NORM_INF: Scale by max absolute value
 * - NORM_L1: Scale so L1 norm = 1 (sum of absolute values)
 * - NORM_L2: Scale so L2 norm = 1 (Euclidean norm)
 *
 * Default: NORM_MINMAX [0, 255] (standard 8-bit conversion)
 */
typedef struct NormalizationParameters{
    double mdRangeMax{255.0};     ///< Maximum value of target range
    double mdRangeMin{0.0};       ///< Minimum value of target range
    int miNormType{cv::NORM_MINMAX};  ///< Normalization type (cv::NormTypes)
} NormalizationParameters;

/**
 * @brief Worker for async normalization processing
 */
class CVNormalizationWorker : public QObject
{
    Q_OBJECT

public:
    CVNormalizationWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat frame,
                      double rangeMin,
                      double rangeMax,
                      int normType,
                      FrameSharingMode mode,
                      std::shared_ptr<CVImagePool> pool,
                      long frameId,
                      QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> outputImage);
};

/**
 * @class CVNormalizationModel
 * @brief Node for normalizing image intensity values with async processing.
 *
 * This model provides flexible normalization using cv::normalize with PBAsyncDataModel
 * for non-blocking processing.
 */
class CVNormalizationModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVNormalizationModel();
    ~CVNormalizationModel() override = default;

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

    NormalizationParameters mParams;
    QPixmap _minPixmap;

    // Pending data for backpressure
    cv::Mat mPendingFrame;
    NormalizationParameters mPendingParams;
};


