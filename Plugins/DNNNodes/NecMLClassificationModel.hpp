//Copyright © 2025, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the Control License.

/**
 * @file NecMLClassificationModel.hpp
 * @brief NECTEC machine learning image classification model.
 *
 * This file implements a specialized DNN-based image classifier trained or
 * optimized for NECTEC (National Electronics and Computer Technology Center)
 * applications. It performs multi-class image classification with
 * standardized preprocessing and output format.
 *
 * **Key Features:**
 * - Standardized normalization (ImageNet-style)
 * - Mean/std normalization for consistent preprocessing
 * - Configurable input size (default: 224×224)
 * - Class label output with confidence scores
 * - Threaded inference for non-blocking operation
 *
 * **Typical Applications:**
 * - Custom object categorization
 * - Product classification
 * - Quality control inspection
 * - Scene understanding
 * - Agricultural classification (crop types, diseases)
 *
 * @see NomadMLClassificationModel
 * @see OnnxClassificationDNNModel
 * @see cv::dnn::Net
 */

#pragma once

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
 * @struct NecMLClassificationBlobImageParameters
 * @brief Image preprocessing parameters for NECTEC ML classification.
 *
 * This structure defines normalization parameters following ImageNet
 * conventions, ensuring consistent preprocessing for trained models.
 *
 * **Normalization Process:**
 * @code
 * // 1. Resize to network input size (224×224)
 * // 2. Scale pixel values: pixel / 255.0
 * // 3. Subtract mean: (pixel - mean) 
 * // 4. Divide by std: (pixel - mean) / std
 * // Result: normalized image with zero mean, unit variance per channel
 * @endcode
 *
 * **ImageNet Statistics:**
 * - Mean RGB: (0.485, 0.456, 0.406) - average pixel values in ImageNet
 * - Std RGB: (0.229, 0.224, 0.225) - standard deviations in ImageNet
 *
 * @see cv::dnn::blobFromImage()
 */
typedef struct NecMLClassificationBlobImageParameters{
    double mdInvScaleFactor{255.};                                ///< Inverse scale (255 = normalize to [0,1])
    cv::Size mCVSize{ cv::Size(224,224) };                        ///< Input size (typical: 224×224)
    cv::Scalar mCVScalarMean{ cv::Scalar(0.485, 0.456, 0.406) };  ///< Mean per channel (ImageNet)
    cv::Scalar mCVScalarStd{ cv::Scalar(0.229, 0.224, 0.225) };   ///< Std dev per channel (ImageNet)
} NecMLClassificationBlobImageParameters;

/**
 * @class NecMLClassificationThread
 * @brief Worker thread for asynchronous NECTEC ML classification.
 *
 * This QThread subclass performs image classification using NECTEC-specific
 * DNN models in a separate thread, with standardized preprocessing and
 * class label output.
 *
 * **Classification Pipeline:**
 * 1. Receive image via detect()
 * 2. Resize to input size (224×224)
 * 3. Normalize: (pixel/255 - mean) / std
 * 4. Create blob with channel order handling
 * 5. Forward pass through network
 * 6. Find class with maximum probability
 * 7. Draw class label and confidence on image
 * 8. Emit result_ready() with image and class name
 *
 * @see NecMLClassificationModel
 */
class NecMLClassificationThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a NecMLClassificationThread.
     * @param parent Parent QObject (typically the model).
     */
    explicit
    NecMLClassificationThread( QObject *parent = nullptr );

    /**
     * @brief Destructor.
     *
     * Aborts processing and waits for thread completion.
     */
    ~NecMLClassificationThread() override;

    /**
     * @brief Enqueues an image for classification.
     * @param Image to classify (any size, will be resized).
     *
     * Thread-safe method to submit images for classification.
     */
    void
    detect( const cv::Mat & );

    /**
     * @brief Loads classification model.
     * @param Model file path (.onnx, .pb, .caffemodel, etc.).
     * @return true if model loaded successfully.
     *
     * Supports multiple DNN formats through OpenCV's unified interface.
     */
    bool
    readNet( QString & );

    /**
     * @brief Sets preprocessing parameters and class names.
     * @param Blob creation parameters (size, mean, std).
     * @param Class names vector (e.g., ["cat", "dog", "bird"]).
     *
     * Must be called before classification to ensure proper preprocessing.
     */
    void
    setParams( NecMLClassificationBlobImageParameters &, std::vector< std::string > &);

    /**
     * @brief Gets current preprocessing parameters.
     * @return Reference to parameter structure.
     */
    NecMLClassificationBlobImageParameters &
    getParams( ) { return mParams; }

Q_SIGNALS:
    /**
     * @brief Signal emitted when classification completes.
     * @param Annotated image with class label.
     * @param Class name string.
     *
     * Emitted from worker thread, received in main thread.
     */
    void
    result_ready( cv::Mat &, QString );

protected:
    /**
     * @brief Thread execution loop.
     *
     * Continuously processes classification queue, performing DNN
     * inference and label annotation for each image.
     */
    void
    run() override;

private:
    QSemaphore mWaitingSemaphore;                     ///< Synchronization semaphore
    QMutex mLockMutex;                                ///< Mutex for thread-safe access

    cv::Mat mCVImage;                                 ///< Current processing image
    cv::dnn::Net mNecMLClassification;                ///< Classification DNN
    std::vector<std::string> mvStrClasses;            ///< Class label strings
    bool mbModelReady {false};                        ///< Model loaded status
    bool mbAbort {false};                             ///< Abort flag for graceful shutdown

    NecMLClassificationBlobImageParameters mParams;   ///< Preprocessing parameters
};

/**
 * @class NecMLClassificationModel
 * @brief Node model for NECTEC ML image classification.
 *
 * This model integrates NECTEC machine learning classification into the
 * dataflow graph, providing class labels and confidence scores for input
 * images. Designed for custom-trained models with standardized preprocessing.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Input image to classify
 *
 * **Output Ports:**
 * 1. **CVImageData** - Annotated image with class label
 * 2. **SyncData** - Synchronization signal
 * 3. **InformationData** - Class name string
 *
 * **Key Features:**
 * - Threaded inference (non-blocking)
 * - ImageNet-standardized preprocessing
 * - Configurable input size and normalization
 * - Multi-output (image + text label)
 * - Support for various model formats
 * - GPU acceleration support
 *
 * **Properties (Configurable):**
 * - **model_filename:** Path to DNN model file
 * - **config_filename:** Path to class labels config
 * - **input_size:** Network input dimensions (default: 224×224)
 * - **mean:** Per-channel mean values
 * - **std:** Per-channel standard deviations
 * - **inv_scale_factor:** Pixel normalization factor
 *
 * **Config File Format:**
 * Plain text file with one class name per line:
 * @code
 * class_0_name
 * class_1_name
 * class_2_name
 * ...
 * @endcode
 *
 * **Example class labels:**
 * @code
 * rice
 * corn
 * soybean
 * wheat
 * barley
 * @endcode
 *
 * **Preprocessing Pipeline:**
 * @code
 * // 1. Resize
 * cv::resize(image, resized, cv::Size(224, 224));
 * 
 * // 2. Convert to float and scale
 * resized.convertTo(float_img, CV_32F, 1.0/255.0);
 * 
 * // 3. Normalize per channel
 * float_img = (float_img - mean) / std;
 * 
 * // 4. Create blob (NCHW format)
 * blob = cv::dnn::blobFromImage(float_img, 1.0, size, mean, swapRB);
 * @endcode
 *
 * **Example Workflows:**
 * @code
 * // Simple classification
 * [Camera] -> [NecMLClassification] -> [ImageDisplay]
 *                                    -> [InformationDisplay]  // Shows class name
 * 
 * // Conditional processing
 * [Image] -> [NecMLClassification] -> [Filter by Class] -> [ProcessingNode]
 * 
 * // Multi-stage analysis
 * [Camera] -> [ObjectDetection] -> [ROI Extract] -> [NecMLClassification]
 * @endcode
 *
 * **Model Training Recommendations:**
 * - Train with same preprocessing (mean, std, input size)
 * - Use data augmentation (rotation, flip, brightness)
 * - Validate with held-out test set
 * - Export to ONNX for portability
 * - Document preprocessing parameters
 *
 * **Performance Optimization:**
 * - Enable GPU backend (CUDA/OpenCL)
 * - Use INT8 quantization for edge devices
 * - Batch processing for multiple images
 * - Cache model loading (load once, classify many)
 * - Use MobileNet architecture for embedded systems
 *
 * **GPU Acceleration:**
 * @code
 * net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
 * net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
 * @endcode
 *
 * **Common Issues:**
 *
 * **Wrong Classifications:**
 * - Verify preprocessing parameters match training
 * - Check class label order matches model output
 * - Ensure input size matches model expectations
 * - Validate model file loaded correctly
 *
 * **Poor Confidence:**
 * - Image quality too low
 * - Object not in training dataset
 * - Preprocessing mismatch
 * - Model needs retraining
 *
 * **Slow Performance:**
 * - Enable GPU acceleration
 * - Use smaller input size
 * - Optimize model architecture
 * - Consider model quantization
 *
 * **Best Practices:**
 * 1. Match preprocessing exactly to training pipeline
 * 2. Validate model with known test images
 * 3. Document all preprocessing parameters
 * 4. Use descriptive class names
 * 5. Save preprocessing config with model
 * 6. Test with edge cases (dark, blurry, occluded)
 * 7. Monitor confidence scores for quality control
 *
 * @see NecMLClassificationThread
 * @see NomadMLClassificationModel
 * @see cv::dnn::Net
 * @see cv::dnn::blobFromImage()
 */
class NecMLClassificationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a NecMLClassificationModel.
     *
     * Initializes with default ImageNet preprocessing parameters.
     */
    NecMLClassificationModel();

    /**
     * @brief Destructor.
     *
     * Stops classification thread and releases resources.
     */
    virtual
    ~NecMLClassificationModel() override
    {
        if( mpNecMLClassificationThread )
            delete mpNecMLClassificationThread;
    }

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing model paths and parameters.
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
     * @return 1 for input, 3 for output (image + sync + label).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return CVImageData, SyncData, or InformationData.
     */
    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Returns output data for a specific port.
     * @param port Output port index (0=image, 1=sync, 2=label).
     * @return Shared pointer to output data.
     */
    std::shared_ptr< NodeData >
    outData( PortIndex port ) override;

    /**
     * @brief Sets input image and triggers classification.
     * @param nodeData Input CVImageData.
     * @param Port index (0).
     *
     * Enqueues image for classification in worker thread.
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
     * Creates and connects the classification thread.
     */
    void
    late_constructor() override;

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private Q_SLOTS:
    /**
     * @brief Slot to receive classification results from worker thread.
     * @param Annotated image with class label.
     * @param Class name string.
     *
     * Updates output data and triggers downstream propagation.
     */
    void
    received_result( cv::Mat &, QString );

private:
    std::shared_ptr< CVImageData > mpCVImageData { nullptr };         ///< Output annotated image
    std::shared_ptr< SyncData > mpSyncData;                           ///< Output sync signal
    std::shared_ptr< InformationData > mpInformationData{ nullptr };  ///< Output class label

    NecMLClassificationThread * mpNecMLClassificationThread { nullptr }; ///< Worker thread

    QString msDNNModel_Filename;  ///< Path to model file
    QString msConfig_Filename;    ///< Path to class labels config

    /**
     * @brief Processes incoming image data.
     * @param in Input CVImageData.
     *
     * Enqueues image for classification.
     */
    void processData(const std::shared_ptr< CVImageData > & in);
    
    /**
     * @brief Loads classification model into worker thread.
     * @param bUpdateDisplayProperties Update UI after loading.
     *
     * Reads model files and initializes DNN with class labels.
     */
    void load_model(bool bUpdateDisplayProperties = false);
    QPixmap _minPixmap;
};
