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
 * @file CVYoloDNNModel.hpp
 * @brief YOLO (You Only Look Once) deep neural network object detection model.
 *
 * This file implements a threaded YOLO object detector using OpenCV's DNN module.
 * YOLO is a state-of-the-art real-time object detection system that detects multiple
 * objects in images with bounding boxes, class labels, and confidence scores.
 *
 * **YOLO Algorithm Overview:**
 * - Single-pass detection (faster than region-based methods)
 * - Divides image into grid cells
 * - Each cell predicts bounding boxes and class probabilities
 * - Non-maximum suppression (NMS) removes duplicate detections
 * - Real-time performance (30+ FPS on GPU)
 *
 * **Supported YOLO Versions:**
 * - YOLOv3, YOLOv4 (Darknet framework)
 * - Requires: weights file (.weights), config file (.cfg), class names file (.txt)
 *
 * **Key Applications:**
 * - General object detection (80 COCO classes)
 * - Custom object detection (trained models)
 * - Real-time surveillance
 * - Autonomous vehicles
 * - Retail analytics
 *
 * @see cv::dnn::Net
 * @see cv::dnn::blobFromImage()
 * @see https://pjreddie.com/darknet/yolo/
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
 * @struct CVYoloDNNImageParameters
 * @brief YOLO preprocessing parameters for image blob creation.
 *
 * This structure defines the parameters for converting input images into
 * blob format suitable for YOLO network inference.
 *
 * **Blob Creation Process:**
 * @code
 * cv::Mat blob = cv::dnn::blobFromImage(
 *     image,
 *     1.0 / mdInvScaleFactor,  // Scale factor (normalize 0-255 to 0-1)
 *     mCVSize,                  // Resize to network input size
 *     cv::Scalar(),             // Mean subtraction (YOLO doesn't use)
 *     mbSwapRB,                 // Swap R and B channels
 *     false                     // Crop
 * );
 * @endcode
 *
 * @see cv::dnn::blobFromImage()
 */
typedef struct CVYoloDNNImageParameters{
    double mdInvScaleFactor{255};         ///< Inverse scale factor (255 = normalize to [0,1])
    cv::Size mCVSize{ cv::Size(416,416) }; ///< Network input size (YOLOv3: 416×416, 608×608)
    bool mbSwapRB{ true };                 ///< Swap red and blue channels (BGR to RGB)
} CVYoloDNNImageParameters;

/**
 * @class CVYoloDNNThread
 * @brief Worker thread for asynchronous YOLO object detection.
 *
 * This QThread subclass handles YOLO inference in a separate thread, preventing
 * blocking of the main processing pipeline. It manages model loading, image
 * preprocessing, forward pass, and post-processing (NMS).
 *
 * **Detection Pipeline:**
 * 1. Receive image via detect()
 * 2. Create blob from image (resize, normalize, channel swap)
 * 3. Forward pass through YOLO network
 * 4. Parse output layers (multiple detection scales)
 * 5. Apply confidence thresholding
 * 6. Non-maximum suppression (remove duplicate boxes)
 * 7. Draw bounding boxes and labels
 * 8. Emit result_ready() signal
 *
 * **YOLO Output Parsing:**
 * Each detection contains:
 * - Center X, Center Y (relative coordinates)
 * - Width, Height (relative dimensions)
 * - Confidence score (objectness)
 * - Class scores (80 for COCO dataset)
 *
 * @see CVYoloDNNModel
 * @see cv::dnn::Net
 */
class CVYoloDNNThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a CVYoloDNNThread.
     * @param parent Parent QObject (typically the model).
     */
    explicit
    CVYoloDNNThread( QObject *parent = nullptr );

    /**
     * @brief Destructor.
     *
     * Aborts processing and waits for thread completion.
     */
    ~CVYoloDNNThread() override;

    /**
     * @brief Enqueues an image for detection.
     * @param Image to process (any size, will be resized).
     *
     * Thread-safe method to submit images for detection.
     */
    void
    detect( const cv::Mat & );

    /**
     * @brief Loads YOLO model files.
     * @param Weights file path (.weights, binary trained weights).
     * @param Config file path (.cfg, network architecture).
     * @param Classes file path (.txt, one class name per line).
     * @return true if model loaded successfully.
     *
     * **File Requirements:**
     * - Weights: Darknet format (.weights)
     * - Config: Darknet format (.cfg)
     * - Classes: Plain text, one class per line
     *
     * **Example classes.txt:**
     * @code
     * person
     * bicycle
     * car
     * ...
     * @endcode
     */
    bool
    readNet( QString & , QString & , QString & );

    /**
     * @brief Sets preprocessing parameters.
     * @param Blob creation parameters.
     *
     * Updates input size, scale factor, and channel swap settings.
     */
    void
    setParams( CVYoloDNNImageParameters &);

    /**
     * @brief Gets current preprocessing parameters.
     * @return Reference to parameter structure.
     */
    CVYoloDNNImageParameters &
    getParams( ) { return mParams; }

Q_SIGNALS:
    /**
     * @brief Signal emitted when detection completes.
     * @param image Annotated image with bounding boxes and labels.
     *
     * Emitted from worker thread, received in main thread.
     */
    void
    result_ready( cv::Mat & image );

protected:
    /**
     * @brief Thread execution loop.
     *
     * Continuously processes detection queue, performing YOLO inference
     * and post-processing for each image.
     */
    void
    run() override;

private:
    /**
     * @brief Draws a detection bounding box on the result image.
     * @param classId Detected object class index.
     * @param conf Confidence score (0.0-1.0).
     * @param left Left X coordinate.
     * @param top Top Y coordinate.
     * @param right Right X coordinate.
     * @param bottom Bottom Y coordinate.
     *
     * Draws rectangle and label with class name and confidence percentage.
     */
    void drawPrediction( int classId, float conf, int left, int top, int right, int bottom );
    
    QSemaphore mWaitingSemaphore;              ///< Synchronization semaphore
    QMutex mLockMutex;                         ///< Mutex for thread-safe access

    cv::Mat mCVImage;                          ///< Current processing image
    cv::dnn::Net mCVYoloDNN;                   ///< YOLO neural network
    std::vector<std::string> mvStrClasses;     ///< Class names (e.g., "person", "car")
    bool mbModelReady {false};                 ///< Model loaded status
    bool mbAbort {false};                      ///< Abort flag for graceful shutdown

    std::vector<cv::String> mvStrOutNames;     ///< Output layer names
    CVYoloDNNImageParameters mParams;          ///< Preprocessing parameters
};

/**
 * @class CVYoloDNNModel
 * @brief Node model for YOLO object detection.
 *
 * This model integrates YOLO object detection into the dataflow graph,
 * providing real-time multi-object detection with bounding boxes and
 * class labels. Supports custom trained models.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Input image (any size, color or grayscale)
 *
 * **Output Ports:**
 * 1. **CVImageData** - Annotated image with detections
 * 2. **SyncData** - Synchronization signal
 *
 * **Key Features:**
 * - Threaded inference (non-blocking)
 * - Configurable preprocessing parameters
 * - NMS for duplicate removal
 * - Customizable confidence thresholds
 * - Support for custom trained models
 * - GPU acceleration support (if OpenCV built with CUDA)
 *
 * **Properties (Configurable):**
 * - **weights_filename:** Path to .weights file
 * - **config_filename:** Path to .cfg file
 * - **classes_filename:** Path to class names .txt file
 * - **inv_scale_factor:** Normalization factor (default: 255)
 * - **input_size:** Network input dimensions (default: 416×416)
 * - **swap_rb:** Swap R/B channels (default: true)
 *
 * **Common YOLO Input Sizes:**
 * - 320×320: Faster, lower accuracy
 * - 416×416: Balanced (default)
 * - 608×608: Slower, higher accuracy
 * - Must be multiple of 32
 *
 * **COCO Dataset Classes (80 objects):**
 * person, bicycle, car, motorcycle, airplane, bus, train, truck, boat,
 * traffic light, fire hydrant, stop sign, parking meter, bench, bird, cat,
 * dog, horse, sheep, cow, elephant, bear, zebra, giraffe, backpack, umbrella,
 * handbag, tie, suitcase, frisbee, skis, snowboard, sports ball, kite,
 * baseball bat, baseball glove, skateboard, surfboard, tennis racket, bottle,
 * wine glass, cup, fork, knife, spoon, bowl, banana, apple, sandwich, orange,
 * broccoli, carrot, hot dog, pizza, donut, cake, chair, couch, potted plant,
 * bed, dining table, toilet, tv, laptop, mouse, remote, keyboard, cell phone,
 * microwave, oven, toaster, sink, refrigerator, book, clock, vase, scissors,
 * teddy bear, hair drier, toothbrush
 *
 * **Example Workflows:**
 * @code
 * // Basic object detection
 * [Camera] -> [YoloDNN] -> [ImageDisplay]
 * 
 * // Detection with filtering
 * [Image] -> [YoloDNN] -> [ImageDisplay]
 *         -> [InformationDisplay]  // Show detected classes
 * 
 * // Multi-stage processing
 * [Camera] -> [Preprocess] -> [YoloDNN] -> [TrackingNode] -> [Display]
 * @endcode
 *
 * **Performance Optimization:**
 * - Use smaller input size for speed (320×320)
 * - Enable GPU backend if available
 * - Reduce confidence threshold to minimize false positives
 * - Use INT8 quantized models for edge devices
 * - Consider YOLOv4-tiny for embedded systems
 *
 * **GPU Acceleration:**
 * @code
 * // Enable CUDA backend (requires OpenCV with CUDA)
 * net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
 * net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
 * @endcode
 *
 * **Model Download:**
 * - YOLOv3: https://pjreddie.com/darknet/yolo/
 * - YOLOv4: https://github.com/AlexeyAB/darknet
 * - Pre-trained weights available for COCO, VOC datasets
 *
 * **Custom Training:**
 * - Train with Darknet framework
 * - Export .weights and .cfg files
 * - Create custom classes.txt
 * - Use same preprocessing parameters
 *
 * **Common Issues:**
 *
 * **Slow Inference:**
 * - Use smaller input size
 * - Enable GPU acceleration
 * - Use lighter model (YOLOv4-tiny)
 *
 * **Poor Detection:**
 * - Adjust confidence threshold
 * - Use larger input size
 * - Ensure correct preprocessing parameters
 * - Verify model trained on relevant dataset
 *
 * **Memory Issues:**
 * - Reduce input size
 * - Limit queue size in thread
 * - Use model quantization
 *
 * **Best Practices:**
 * 1. Start with pre-trained COCO weights for testing
 * 2. Match input size to model training size
 * 3. Enable GPU if real-time performance needed
 * 4. Filter detections by confidence threshold (>0.5 typical)
 * 5. Use NMS to remove duplicate boxes
 * 6. Save/load model once (expensive operation)
 * 7. Consider async processing for video streams
 *
 * **Output Format:**
 * Annotated image contains:
 * - Bounding boxes (colored rectangles)
 * - Class labels (text above boxes)
 * - Confidence scores (percentage)
 * - Color-coded by class
 *
 * @see CVYoloDNNThread
 * @see cv::dnn::Net
 * @see cv::dnn::blobFromImage()
 * @see cv::dnn::NMSBoxes()
 */
class CVYoloDNNModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVYoloDNNModel.
     *
     * Initializes with default parameters and null model files.
     */
    CVYoloDNNModel();

    /**
     * @brief Destructor.
     *
     * Stops detection thread and releases resources.
     */
    virtual
    ~CVYoloDNNModel() override
    {
        if( mpCVYoloDNNThread )
            delete mpCVYoloDNNThread;
    }

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing model file paths and parameters.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved configuration.
     */
    void
    restore(QJsonObject const &p);

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 1 for input (image), 2 for output (annotated image + sync).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return CVImageData for image ports, SyncData for sync.
     */
    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Returns output data for a specific port.
     * @param port Output port index (0=annotated image, 1=sync).
     * @return Shared pointer to output data.
     */
    std::shared_ptr< NodeData >
    outData( PortIndex port ) override;

    /**
     * @brief Sets input image and triggers detection.
     * @param nodeData Input CVImageData.
     * @param Port index (0).
     *
     * Enqueues image for YOLO detection in worker thread.
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    /**
     * @brief Returns null (no embedded widget).
     * @return nullptr.
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets a model property.
     * @param Property name.
     * @param QVariant value.
     *
     * Handles configuration changes (model files, parameters).
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    /**
     * @brief Late constructor for thread initialization.
     *
     * Creates and connects the YOLO detection thread.
     */
    void
    late_constructor() override;

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private Q_SLOTS:
    /**
     * @brief Slot to receive detection results from worker thread.
     * @param Annotated image with bounding boxes.
     *
     * Updates output data and triggers downstream propagation.
     */
    void
    received_result( cv::Mat & );

private:
    std::shared_ptr< CVImageData > mpCVImageData { nullptr }; ///< Output annotated image
    std::shared_ptr<SyncData> mpSyncData;                     ///< Output sync signal

    CVYoloDNNThread * mpCVYoloDNNThread { nullptr };          ///< Worker thread

    QString msWeights_Filename;    ///< Path to .weights file
    QString msClasses_Filename;    ///< Path to classes.txt file
    QString msConfig_Filename;     ///< Path to .cfg file

    /**
     * @brief Processes incoming image data.
     * @param in Input CVImageData.
     *
     * Enqueues image for detection.
     */
    void processData(const std::shared_ptr< CVImageData > & in);
    
    /**
     * @brief Loads YOLO model into worker thread.
     *
     * Reads model files and initializes network.
     */
    void load_model();
    QPixmap _minPixmap;
};
