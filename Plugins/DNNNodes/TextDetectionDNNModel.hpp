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
 * @file TextDetectionDNNModel.hpp
 * @brief DB (Differentiable Binarization) text detection model.
 *
 * This file implements a DNN-based text region detector using the DB algorithm,
 * which locates text regions in images regardless of language or orientation.
 * The model outputs polygon boundaries around detected text areas.
 *
 * **DB Algorithm:**
 * - Differentiable Binarization for scene text detection
 * - Detects arbitrary-shaped text (curved, rotated, multi-oriented)
 * - Outputs probability map and threshold map
 * - Post-processing extracts polygon contours
 * - Real-time capable (30+ FPS with GPU)
 *
 * **Key Features:**
 * - Multi-oriented text detection
 * - Arbitrary shapes (not just rectangles)
 * - Scale-invariant (small to large text)
 * - Language-agnostic
 * - Natural scene and document text
 *
 * **Applications:**
 * - OCR preprocessing (text localization)
 * - Document analysis
 * - Sign detection
 * - License plate localization
 * - Retail product label detection
 *
 * @see TextRecognitionDNNModel (for recognizing detected text)
 * @see cv::dnn::TextDetectionModel_DB
 * @see https://arxiv.org/abs/1911.08947
 */

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QMutex>

#include "PBNodeDelegateModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"
#include <opencv2/dnn.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct TextDetectionDBParameters
 * @brief DB text detection algorithm parameters.
 *
 * Controls the post-processing behavior of the DB text detector.
 *
 * **Parameter Tuning Guide:**
 * - **binaryThreshold:** Lower = more sensitive (detects faint text)
 * - **polygonThreshold:** Higher = stricter polygon quality
 * - **unclipRatio:** Larger = bigger bounding polygons (more padding)
 * - **maxCandidate:** More = detect more regions (may include noise)
 */
typedef struct TextDetectionDBParameters{
    float mfBinaryThreshold{0.3};     ///< Binary map threshold (0.2-0.5 typical)
    float mfPolygonThreshold{0.5};    ///< Polygon confidence threshold (0.3-0.7)
    double mdUnclipRatio{2.0};        ///< Polygon expansion ratio (1.5-2.5)
    int miMaxCandidate{200};          ///< Maximum text regions to detect
    cv::Size mCVSize{ cv::Size(736,736) }; ///< Network input size (must be multiple of 32)
} TextDetectionDBParameters;

/**
 * @class TextDetectionDBThread
 * @brief Worker thread for asynchronous DB text detection.
 *
 * Performs text region detection using the DB algorithm in a separate thread.
 *
 * **Detection Pipeline:**
 * 1. Resize image to network input size
 * 2. Forward pass through DB network
 * 3. Generate probability and threshold maps
 * 4. Binary segmentation with adaptive threshold
 * 5. Extract polygon contours
 * 6. Filter by confidence and size
 * 7. Draw polygons on result image
 * 8. Emit annotated image
 *
 * @see TextDetectionDNNModel
 * @see cv::dnn::TextDetectionModel_DB
 */
class TextDetectionDBThread : public QThread
{
    Q_OBJECT
public:
    explicit
    TextDetectionDBThread( QObject *parent = nullptr );

    ~TextDetectionDBThread() override;

    void
    detect( const cv::Mat & );

    /**
     * @brief Loads DB text detection model.
     * @param Model file path (.onnx or other DNN format).
     * @return true if successful.
     *
     * **Recommended Models:**
     * - DB_TD500_resnet50.onnx (ResNet-50 backbone)
     * - DB_IC15_resnet18.onnx (lighter, faster)
     */
    bool
    readNet( QString & );

    /**
     * @brief Sets DB detection parameters.
     * @param Detection parameters (thresholds, sizes).
     */
    void
    setParams( TextDetectionDBParameters &);

    TextDetectionDBParameters &
    getParams( ) { return mParams; }

Q_SIGNALS:
    void
    result_ready( cv::Mat & image );

protected:
    void
    run() override;

private:
    QSemaphore mWaitingSemaphore;
    QMutex mLockMutex;

    cv::Mat mCVImage;
    cv::dnn::TextDetectionModel_DB mTextDetectionDNN; ///< DB text detector
    bool mbModelReady {false};
    bool mbAbort {false};

    TextDetectionDBParameters mParams;
};

/**
 * @class TextDetectionDNNModel
 * @brief Node model for DB-based text detection.
 *
 * Integrates DB text detection into the dataflow graph, locating text
 * regions in images with polygon boundaries. Often used as preprocessing
 * for OCR (optical character recognition).
 *
 * **Input Ports:**
 * 1. **CVImageData** - Input image (any size, color or grayscale)
 *
 * **Output Ports:**
 * 1. **CVImageData** - Annotated image with text region polygons
 * 2. **SyncData** - Synchronization signal
 *
 * **Properties:**
 * - **model_filename:** Path to DB model file
 * - **binary_threshold:** Probability threshold (default: 0.3)
 * - **polygon_threshold:** Polygon confidence (default: 0.5)
 * - **unclip_ratio:** Boundary expansion (default: 2.0)
 * - **max_candidates:** Maximum regions (default: 200)
 * - **input_size:** Network input dimensions (default: 736×736)
 *
 * **Example Workflows:**
 * @code
 * // Full OCR pipeline
 * [Image] -> [TextDetection] -> [ROI Extract] -> [TextRecognition] -> [Display]
 * 
 * // Document scanning
 * [Scanner] -> [Preprocess] -> [TextDetection] -> [Perspective Correct] -> [OCR]
 * 
 * // Sign detection
 * [Camera] -> [TextDetection] -> [Filter by Size] -> [Recognition]
 * @endcode
 *
 * **Parameter Tuning:**
 *
 * **For Small Text:**
 * - Lower binary_threshold (0.2-0.25)
 * - Use larger input_size (960×960, 1280×1280)
 * - Increase unclip_ratio (2.5-3.0)
 *
 * **For Large Text:**
 * - Higher binary_threshold (0.4-0.5)
 * - Smaller input_size acceptable (640×640)
 * - Lower unclip_ratio (1.5-2.0)
 *
 * **For Noisy Images:**
 * - Higher polygon_threshold (0.6-0.8)
 * - Lower max_candidates (50-100)
 * - Pre-process with denoising
 *
 * **Performance:**
 * - CPU: 5-15 FPS (736×736 input)
 * - GPU: 30-60 FPS
 * - Scales with input size (larger = slower, more accurate)
 *
 * **Model Download:**
 * - OpenCV GitHub: https://github.com/opencv/opencv_extra
 * - Pre-trained on ICDAR, MSRA-TD500 datasets
 * - Custom training with DB framework
 *
 * **Output Format:**
 * Annotated image shows:
 * - Green polygons around detected text
 * - Arbitrary shapes (not just rectangles)
 * - All orientations supported
 *
 * **Common Issues:**
 *
 * **Missed Text:**
 * - Lower binary_threshold
 * - Increase input_size
 * - Check lighting and image quality
 *
 * **False Positives:**
 * - Raise polygon_threshold
 * - Reduce max_candidates
 * - Pre-process to reduce noise
 *
 * **Slow Performance:**
 * - Enable GPU acceleration
 * - Reduce input_size
 * - Use lighter model (ResNet-18)
 *
 * **Best Practices:**
 * 1. Use with TextRecognition for full OCR
 * 2. Pre-process images (denoise, enhance contrast)
 * 3. Tune thresholds for specific use case
 * 4. Consider input size vs speed tradeoff
 * 5. Test on representative samples
 * 6. GPU highly recommended for real-time
 *
 * @see TextDetectionDBThread
 * @see TextRecognitionDNNModel
 * @see cv::dnn::TextDetectionModel_DB
 */
class TextDetectionDNNModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    TextDetectionDNNModel();

    virtual
    ~TextDetectionDNNModel() override
    {
        if( mpTextDetectionDNNThread )
            delete mpTextDetectionDNNThread;
    }

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p);

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr< NodeData >
    outData( PortIndex port ) override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    void
    late_constructor() override;

    static const QString _category;
    static const QString _model_name;

private Q_SLOTS:
    void
    received_result( cv::Mat & );

private:
    std::shared_ptr< CVImageData > mpCVImageData { nullptr };
    std::shared_ptr<SyncData> mpSyncData;

    TextDetectionDBThread * mpTextDetectionDNNThread { nullptr };

    QString msDBModel_Filename;

    void processData(const std::shared_ptr< CVImageData > & in);
    void load_model();
    QPixmap _minPixmap;
};
