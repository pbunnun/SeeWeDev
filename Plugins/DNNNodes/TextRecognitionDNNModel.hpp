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
 * @file TextRecognitionDNNModel.hpp
 * @brief CRNN-based text recognition (OCR) model.
 *
 * This file implements a DNN-based text recognizer using CRNN (Convolutional
 * Recurrent Neural Network) architecture. It converts detected text regions
 * into readable character sequences.
 *
 * **CRNN Architecture:**
 * - CNN layers: Extract visual features from text images
 * - RNN layers: Model sequential character dependencies
 * - CTC decoder: Map features to character sequences
 * - No explicit segmentation needed (reads full text line)
 *
 * **Key Features:**
 * - End-to-end text recognition
 * - Variable-length text support
 * - Multi-language capable (with appropriate vocabulary)
 * - Cursive and printed text
 * - Case-sensitive recognition
 *
 * **Applications:**
 * - OCR (Optical Character Recognition)
 * - Document digitization
 * - License plate reading
 * - Product label reading
 * - Sign translation
 *
 * @see TextDetectionDNNModel (for text localization)
 * @see cv::dnn::TextRecognitionModel
 * @see https://arxiv.org/abs/1507.05717
 */

#pragma once


#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QMutex>

#include "PBNodeDelegateModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"
#include "InformationData.hpp"
#include <opencv2/dnn.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class TextRecognitionThread
 * @brief Worker thread for asynchronous CRNN text recognition.
 *
 * Performs text recognition using CRNN models in a separate thread,
 * converting text images into character sequences.
 *
 * **Recognition Pipeline:**
 * 1. Receive text image (typically from TextDetection)
 * 2. Preprocess: resize, normalize
 * 3. Forward pass through CRNN
 * 4. CTC decoding to character sequence
 * 5. Map indices to vocabulary characters
 * 6. Emit recognized text string
 * 7. Draw text on result image
 *
 * **Vocabulary File Format:**
 * Plain text file with one character per line:
 * @code
 * 0
 * 1
 * 2
 * ...
 * 9
 * A
 * B
 * ...
 * Z
 * a
 * b
 * ...
 * z
 * @endcode
 *
 * @see TextRecognitionDNNModel
 * @see cv::dnn::TextRecognitionModel
 */
class TextRecognitionThread : public QThread
{
    Q_OBJECT
public:
    explicit
    TextRecognitionThread( QObject *parent = nullptr );

    ~TextRecognitionThread() override;

    void
    detect( const cv::Mat & );

    /**
     * @brief Loads CRNN text recognition model.
     * @param Model file path (.onnx or other DNN format).
     * @return true if successful.
     *
     * **Recommended Models:**
     * - CRNN_VGG_BiLSTM_CTC.onnx (high accuracy)
     * - CRNN_MobileNet.onnx (faster, embedded)
     */
    bool
    readNet( QString & );

    /**
     * @brief Sets vocabulary file for character mapping.
     * @param Vocabulary file path (.txt, one character per line).
     *
     * Vocabulary defines the character set the model can recognize.
     * Order must match model training.
     */
    void
    setParams( QString & );

Q_SIGNALS:
    /**
     * @brief Signal emitted when recognition completes.
     * @param Annotated image with recognized text.
     * @param Recognized text string.
     */
    void
    result_ready( cv::Mat &, QString );

protected:
    void
    run() override;

private:
    QSemaphore mWaitingSemaphore;
    QMutex mLockMutex;

    QString msVocabulary_Filename{""};  ///< Path to vocabulary file

    cv::Mat mCVImage;
    cv::dnn::TextRecognitionModel mTextRecognitionDNN; ///< CRNN text recognizer
    bool mbModelReady {false};
    bool mbAbort {false};
};

/**
 * @class TextRecognitionDNNModel
 * @brief Node model for CRNN-based text recognition (OCR).
 *
 * Integrates text recognition into the dataflow graph, converting text
 * images (typically from TextDetection) into readable character strings.
 * Forms the second stage of a complete OCR pipeline.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Text image (cropped text region)
 *
 * **Output Ports:**
 * 1. **CVImageData** - Annotated image with recognized text
 * 2. **SyncData** - Synchronization signal
 * 3. **InformationData** - Recognized text string
 *
 * **Properties:**
 * - **model_filename:** Path to CRNN model file
 * - **vocabulary_filename:** Path to character vocabulary
 *
 * **Example Workflows:**
 * @code
 * // Full OCR pipeline
 * [Image] -> [TextDetection] -> [ROI Extract] -> [TextRecognition] -> [Display Text]
 * 
 * // License plate reading
 * [Camera] -> [PlateDetection] -> [Deskew] -> [TextRecognition] -> [Database Lookup]
 * 
 * // Multi-language OCR
 * [Document] -> [TextDetection] -> [Language Detect] -> [TextRecognition (EN/TH)] -> [Export]
 * @endcode
 *
 * **Vocabulary Configuration:**
 *
 * **English Alphanumeric:**
 * @code
 * 0123456789
 * ABCDEFGHIJKLMNOPQRSTUVWXYZ
 * abcdefghijklmnopqrstuvwxyz
 * @endcode
 *
 * **With Punctuation:**
 * @code
 * 0123456789
 * ABCDEFGHIJKLMNOPQRSTUVWXYZ
 * abcdefghijklmnopqrstuvwxyz
 * .,;:!?-'"()[]
 * @endcode
 *
 * **Thai Characters:**
 * @code
 * กขฃคฅฆงจฉชซฌญฎฏ...
 * @endcode
 *
 * **Input Image Requirements:**
 * - Cropped text line (not full document)
 * - Horizontal orientation (use deskew if needed)
 * - Clear text, good contrast
 * - Typical height: 32-48 pixels
 * - Variable width (CRNN handles any length)
 *
 * **Preprocessing Recommendations:**
 * - Deskew rotated text
 * - Normalize height (resize to 32 or 48 pixels)
 * - Enhance contrast (CLAHE)
 * - Binarize if needed (adaptive threshold)
 * - Remove borders/padding
 *
 * **Performance:**
 * - CPU: 10-30 recognitions/second
 * - GPU: 100-300 recognitions/second
 * - Scales with text length and model complexity
 *
 * **Model Training:**
 * - Train with synthetic text (various fonts, sizes)
 * - Include real-world samples
 * - Data augmentation (noise, blur, perspective)
 * - CTC loss for sequence learning
 * - Export to ONNX for deployment
 *
 * **Common Character Sets:**
 *
 * **Digits Only (0-9):**
 * - License plates
 * - Postal codes
 * - Serial numbers
 *
 * **Alphanumeric:**
 * - General text
 * - Product labels
 * - Signs
 *
 * **Full ASCII:**
 * - Documents
 * - Forms
 * - Receipts
 *
 * **Unicode (Multilingual):**
 * - International text
 * - Mixed language documents
 *
 * **Accuracy Factors:**
 * - Image quality (resolution, contrast)
 * - Text clarity (font, size)
 * - Vocabulary match (must be in training set)
 * - Model quality (training data, architecture)
 * - Preprocessing quality
 *
 * **Common Issues:**
 *
 * **Wrong Recognition:**
 * - Verify vocabulary matches model training
 * - Check image preprocessing (deskew, resize)
 * - Ensure good image quality
 * - Retrain model with similar samples
 *
 * **Missing Characters:**
 * - Character not in vocabulary
 * - Add missing characters and retrain
 * - Check vocabulary file encoding (UTF-8)
 *
 * **Low Confidence:**
 * - Improve image quality
 * - Better preprocessing (denoise, sharpen)
 * - Use higher resolution
 * - Retrain with more data
 *
 * **Slow Performance:**
 * - Enable GPU acceleration
 * - Use lighter model architecture
 * - Batch processing for multiple texts
 * - Reduce input image size
 *
 * **Best Practices:**
 * 1. Always use with TextDetection for full OCR
 * 2. Match vocabulary to application domain
 * 3. Preprocess text images (deskew, normalize)
 * 4. Validate with test set before deployment
 * 5. Handle edge cases (rotated, low quality)
 * 6. Log confidence scores for quality control
 * 7. Post-process with dictionary/language model
 * 8. GPU highly recommended for real-time
 *
 * **Output Format:**
 * - InformationData: Plain text string (recognized characters)
 * - CVImageData: Original image with text overlay
 * - Confidence scores available in full implementation
 *
 * @see TextRecognitionThread
 * @see TextDetectionDNNModel
 * @see cv::dnn::TextRecognitionModel
 */
class TextRecognitionDNNModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    TextRecognitionDNNModel();

    virtual
    ~TextRecognitionDNNModel() override
    {
        if( mpTextRecognitionDNNThread )
            delete mpTextRecognitionDNNThread;
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

    void
    late_constructor() override;

    static const QString _category;
    static const QString _model_name;

private Q_SLOTS:
    void
    received_result( cv::Mat &, QString );

private:
    std::shared_ptr< CVImageData > mpCVImageData { nullptr };
    std::shared_ptr< SyncData > mpSyncData;
    std::shared_ptr< InformationData > mpInformationData{ nullptr };

    TextRecognitionThread * mpTextRecognitionDNNThread { nullptr };

    QString msModel_Filename;
    QString msVocabulary_Filename;

    void processData(const std::shared_ptr< CVImageData > & in);
    void load_model();
};
