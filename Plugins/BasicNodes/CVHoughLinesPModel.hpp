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
 * @file CVHoughLinesPModel.hpp
 * @brief Probabilistic Hough Line Transform for detecting line segments.
 *
 * This node implements the Probabilistic Hough Line Transform using cv::HoughLinesP.
 * Unlike the standard version, it returns actual line segments with endpoints (x1,y1,x2,y2)
 * rather than infinite lines in polar coordinates.
 *
 * Advantages:
 * - More efficient (faster) than standard Hough Transform
 * - Returns finite line segments with start and end points
 * - Can filter by minimum line length
 * - Can merge nearby collinear segments using maxLineGap
 *
 * Use Cases:
 * - Lane marking detection for ADAS
 * - Barcode scanning
 * - Document edge detection
 * - Architectural line detection in photos
 * - Industrial part inspection
 *
 * @see cv::HoughLinesP, CVCannyEdgeModel, CVHoughLinesModel
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
 * @struct CVHoughLinesPParameters
 * @brief Parameters for probabilistic Hough Line Transform
 */
typedef struct CVHoughLinesPParameters {
    double mdRho {1.0};                  ///< Distance resolution in pixels
    double mdTheta {CV_PI/180.0};        ///< Angle resolution in radians
    int miThreshold {50};                ///< Accumulator threshold
    double mdMinLineLength {50.0};       ///< Minimum line segment length
    double mdMaxLineGap {10.0};          ///< Maximum gap between collinear segments
    bool mbDisplayLines {true};          ///< Whether to draw detected lines
    unsigned char mucLineColor[3] {0, 255, 0}; ///< Line color (BGR) - green by default
    int miLineThickness {2};             ///< Line thickness
    int miLineType {cv::LINE_AA};        ///< Line type
} CVHoughLinesPParameters;

Q_DECLARE_METATYPE(CVHoughLinesPParameters)

/**
 * @class CVHoughLinesPWorker
 * @brief Worker for asynchronous probabilistic line detection
 */
class CVHoughLinesPWorker : public QObject
{
    Q_OBJECT
public:
    CVHoughLinesPWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     CVHoughLinesPParameters params,
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img, std::shared_ptr<IntegerData> count);
};

/**
 * @class CVHoughLinesPModel
 * @brief Node model for probabilistic Hough Line Transform
 */
class CVHoughLinesPModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    CVHoughLinesPModel();

    virtual
    ~CVHoughLinesPModel() override {}

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

    CVHoughLinesPParameters mParams;
    std::shared_ptr<IntegerData> mpIntegerData { nullptr };
    QPixmap _minPixmap;

    static const std::string color[3];

    // Pending data for backpressure
    cv::Mat mPendingFrame;
    CVHoughLinesPParameters mPendingParams;
};
