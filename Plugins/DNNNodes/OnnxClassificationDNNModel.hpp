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
 * @file OnnxClassificationDNNModel.hpp
 * @brief ONNX-based image classification model.
 *
 * This file implements a DNN-based image classifier using ONNX (Open Neural
 * Network Exchange) format models. ONNX provides framework-independent model
 * deployment, allowing models trained in PyTorch, TensorFlow, or other
 * frameworks to be used in OpenCV.
 *
 * **ONNX Format Advantages:**
 * - Framework independence (PyTorch, TensorFlow, Keras, etc.)
 * - Standardized model format
 * - Optimized inference engines
 * - Wide hardware support
 * - Easy model portability
 *
 * **Supported Architectures:**
 * - ResNet (18, 34, 50, 101, 152)
 * - MobileNet (V2, V3)
 * - EfficientNet
 * - VGG (16, 19)
 * - DenseNet
 * - Custom trained models
 *
 * **Key Applications:**
 * - ImageNet classification (1000 classes)
 * - Custom object categorization
 * - Transfer learning applications
 * - Production deployment of trained models
 *
 * @see NecMLClassificationModel
 * @see cv::dnn::readNetFromONNX()
 * @see https://onnx.ai/
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
 * @struct OnnxClassificationDNNBlobImageParameters
 * @brief Image preprocessing parameters for ONNX classification.
 *
 * Standard ImageNet preprocessing for ONNX models.
 *
 * @see NecMLClassificationBlobImageParameters
 */
typedef struct OnnxClassificationDNNBlobImageParameters{
    double mdInvScaleFactor{255.};                                ///< Scale to [0,1]
    cv::Size mCVSize{ cv::Size(224,224) };                        ///< Network input size
    cv::Scalar mCVScalarMean{ cv::Scalar(0.485, 0.456, 0.406) };  ///< ImageNet mean
    cv::Scalar mCVScalarStd{ cv::Scalar(0.229, 0.224, 0.225) };   ///< ImageNet std
} OnnxClassificationDNNBlobImageParameters;

/**
 * @class OnnxClassificationDNNThread
 * @brief Worker thread for asynchronous ONNX classification.
 *
 * Performs image classification using ONNX models in a separate thread.
 *
 * **ONNX Model Loading:**
 * @code
 * cv::dnn::Net net = cv::dnn::readNetFromONNX("model.onnx");
 * @endcode
 *
 * @see OnnxClassificationDNNModel
 */
class OnnxClassificationDNNThread : public QThread
{
    Q_OBJECT
public:
    explicit
    OnnxClassificationDNNThread( QObject *parent = nullptr );

    ~OnnxClassificationDNNThread() override;

    void
    detect( const cv::Mat & );

    /**
     * @brief Loads ONNX model and class labels.
     * @param Model file path (.onnx).
     * @param Classes file path (.txt, one class per line).
     * @return true if successful.
     */
    bool
    readNet( QString & , QString & );

    void
    setParams( OnnxClassificationDNNBlobImageParameters &);

    OnnxClassificationDNNBlobImageParameters &
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
    cv::dnn::Net mOnnxClassificationDNN;
    std::vector<std::string> mvStrClasses;
    bool mbModelReady {false};
    bool mbAbort {false};

    OnnxClassificationDNNBlobImageParameters mParams;
};

/**
 * @class OnnxClassificationDNNModel
 * @brief Node model for ONNX-based image classification.
 *
 * Provides framework-independent image classification using ONNX models.
 * Compatible with models exported from PyTorch, TensorFlow, Keras, etc.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Input image to classify
 *
 * **Output Ports:**
 * 1. **CVImageData** - Annotated image with class label
 * 2. **SyncData** - Synchronization signal
 *
 * **Properties:**
 * - **model_filename:** Path to .onnx model file
 * - **classes_filename:** Path to class names .txt file
 * - **Preprocessing parameters** (size, mean, std)
 *
 * **Example ONNX Export (PyTorch):**
 * @code
 * import torch
 * model = torchvision.models.resnet50(pretrained=True)
 * dummy_input = torch.randn(1, 3, 224, 224)
 * torch.onnx.export(model, dummy_input, "resnet50.onnx")
 * @endcode
 *
 * **Example ONNX Export (TensorFlow/Keras):**
 * @code
 * import tf2onnx
 * import onnx
 * onnx_model, _ = tf2onnx.convert.from_keras(model)
 * onnx.save(onnx_model, "model.onnx")
 * @endcode
 *
 * **Pre-trained Models:**
 * - ONNX Model Zoo: https://github.com/onnx/models
 * - ImageNet classifiers (ResNet, MobileNet, etc.)
 * - Custom trained models
 *
 * **Performance:**
 * - Similar to native OpenCV models
 * - GPU acceleration supported
 * - ONNX Runtime provides optimizations
 *
 * @see OnnxClassificationDNNThread
 * @see cv::dnn::readNetFromONNX()
 */
class OnnxClassificationDNNModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    OnnxClassificationDNNModel();

    virtual
    ~OnnxClassificationDNNModel() override
    {
        if( mpOnnxClassificationDNNThread )
            delete mpOnnxClassificationDNNThread;
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

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override{ return _minPixmap; }

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

    OnnxClassificationDNNThread * mpOnnxClassificationDNNThread { nullptr };

    QString msDNNModel_Filename;
    QString msClasses_Filename;

    void processData(const std::shared_ptr< CVImageData > & in);
    void load_model();
    QPixmap _minPixmap;
};
