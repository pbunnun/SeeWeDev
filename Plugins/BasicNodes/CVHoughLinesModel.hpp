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
 * @file CVHoughLinesModel.hpp
 * @brief Standard Hough Line Transform for detecting straight lines in images.
 *
 * This node implements the standard Hough Line Transform using cv::HoughLines.
 * It detects infinite straight lines in edge-detected images and represents them
 * in polar coordinates (rho, theta).
 *
 * Algorithm:
 * - Input: Binary edge image (typically from Canny edge detector)
 * - Output: Lines represented as (rho, theta) where:
 *   * rho: Distance from origin to line
 *   * theta: Angle of the perpendicular from origin to line
 *
 * Use Cases:
 * - Lane detection in autonomous vehicles
 * - Document boundary detection
 * - Architectural line detection
 * - Industrial part alignment verification
 *
 * @see cv::HoughLines, CVCannyEdgeModel, CVHoughLinesPModel
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"
#include "CVImagePool.hpp"
#include "SyncData.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;
using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;

/**
 * @struct CVHoughLinesParameters
 * @brief Parameters for standard Hough Line Transform
 */
typedef struct CVHoughLinesParameters {
    double mdRho {1.0};              ///< Distance resolution in pixels (typically 1.0)
    double mdTheta {CV_PI/180.0};    ///< Angle resolution in radians (typically 1 degree)
    int miThreshold {100};           ///< Accumulator threshold (minimum votes for line)
    bool mbDisplayLines {true};      ///< Whether to draw detected lines
    unsigned char mucLineColor[3] {255, 0, 0}; ///< Line color (BGR)
    int miLineThickness {2};         ///< Line thickness
    int miLineType {cv::LINE_AA};    ///< Line type (LINE_8, LINE_4, LINE_AA)
} CVHoughLinesParameters;

Q_DECLARE_METATYPE(CVHoughLinesParameters)

/**
 * @class CVHoughLinesWorker
 * @brief Worker for asynchronous line detection
 */
class CVHoughLinesWorker : public QObject
{
    Q_OBJECT
public:
    CVHoughLinesWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     CVHoughLinesParameters params,
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img, std::shared_ptr<IntegerData> count);
};

/**
 * @class CVHoughLinesModel
 * @brief Node model for standard Hough Line Transform
 */
class CVHoughLinesModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVHoughLinesModel();

    virtual
    ~CVHoughLinesModel() override {}

    QJsonObject
    save() const override;

    void
    load(const QJsonObject &p) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    // Override base class to handle 3 outputs
    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    static const QString _category;

    static const QString _model_name;

protected:
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;

private:
    void process_cached_input() override;

    CVHoughLinesParameters mParams;
    std::shared_ptr<IntegerData> mpIntegerData { nullptr };
    QPixmap _minPixmap;

    static const std::string color[3];

    // Pending data for backpressure
    cv::Mat mPendingFrame;
    CVHoughLinesParameters mPendingParams;
};
