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
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "DoubleData.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

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
    double mdRangeMax;     ///< Maximum value of target range
    double mdRangeMin;     ///< Minimum value of target range
    int miNormType;        ///< Normalization type (cv::NormTypes)
    NormalizationParameters()
        : mdRangeMax(255),
          mdRangeMin(0),
          miNormType(cv::NORM_MINMAX)
    {
    }
} NormalizationParameters;

/**
 * @class CVNormalizationModel
 * @brief Node for normalizing image intensity values to specified ranges.
 *
 * This model provides flexible normalization using cv::normalize, supporting multiple
 * normalization types and dynamic range specification via optional input ports.
 *
 * Core Operation (NORM_MINMAX):
 * ```cpp
 * cv::normalize(input, output, min, max, cv::NORM_MINMAX);
 * // Maps input range [in_min, in_max] → output range [min, max]
 * ```
 *
 * Common Use Cases:
 *
 * 1. Contrast Enhancement:
 *    ```
 *    LowContrastImage → Normalize(0, 255, MINMAX) → Display
 *    Input: [80, 120] → Output: [0, 255] (full range)
 *    ```
 *
 * 2. 16-bit to 8-bit Conversion:
 *    ```
 *    16bitImage → Normalize(0, 255, MINMAX) → SavePNG
 *    Input: [0, 65535] → Output: [0, 255]
 *    ```
 *
 * 3. Float Display:
 *    ```
 *    FloatImage [0.0, 1.0] → Normalize(0, 255, MINMAX) → Display
 *    ```
 *
 * 4. Dynamic Range from Data:
 *    ```
 *    Image → ┐
 *            ├→ Normalize → Output
 *    Min  →  │  (use dynamic min/max)
 *    Max  → ┘
 *    ```
 *
 * The overwrite() function allows incoming DoubleData to override default min/max.
 *
 * Performance: O(W×H), fast linear transformation
 *
 * @see cv::normalize, NormalizationParameters
 */
class CVNormalizationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVNormalizationModel();

    virtual
    ~CVNormalizationModel() override {}

    QJsonObject
    save() const override;

    void
    load(const QJsonObject &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:
    NormalizationParameters mParams;                   ///< Normalization configuration
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };  ///< Input image
    std::shared_ptr<DoubleData> mapDoubleInData[2] {{nullptr}};  ///< Optional min/max inputs
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };    ///< Normalized output
    QPixmap _minPixmap;

    /**
     * @brief Performs normalization using cv::normalize.
     * @param in Input image
     * @param out Normalized output image
     * @param params Normalization parameters (range, type)
     */
    void processData( const std::shared_ptr< CVImageData> &in, std::shared_ptr<CVImageData> &out,
                      const NormalizationParameters & params);

    /**
     * @brief Overrides range parameters with incoming DoubleData.
     *
     * Allows dynamic control of min/max via input ports:
     * - in[0]: Override mdRangeMin
     * - in[1]: Override mdRangeMax
     *
     * @param in Array of DoubleData inputs (min, max)
     * @param params Parameters to update
     */
    void overwrite(std::shared_ptr<DoubleData> (&in)[2], NormalizationParameters &params);

};

