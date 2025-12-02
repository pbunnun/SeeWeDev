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
 * @file NomadMLClassificationModel.hpp
 * @brief NOMAD machine learning image classification model.
 *
 * This file implements a specialized DNN-based image classifier for NOMAD
 * (specific project/organization) applications. Similar to NecML, it provides
 * multi-class image classification with standardized ImageNet preprocessing.
 *
 * **Key Differences from NecML:**
 * - Project-specific model architecture or training
 * - Potentially different class sets
 * - Same preprocessing pipeline (ImageNet normalization)
 * - Compatible output format
 *
 * **Typical Applications:**
 * - Custom object categorization
 * - Specialized domain classification
 * - Quality assessment
 * - Pattern recognition
 *
 * @see NecMLClassificationModel
 * @see OnnxClassificationDNNModel
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
 * @struct NomadMLClassificationBlobImageParameters
 * @brief Image preprocessing parameters for NOMAD ML classification.
 *
 * Identical to NecML preprocessing - follows ImageNet normalization standards.
 *
 * @see NecMLClassificationBlobImageParameters
 */
typedef struct NomadMLClassificationBlobImageParameters{
    double mdInvScaleFactor{255.};                                ///< Normalize to [0,1]
    cv::Size mCVSize{ cv::Size(224,224) };                        ///< Network input size
    cv::Scalar mCVScalarMean{ cv::Scalar(0.485, 0.456, 0.406) };  ///< ImageNet mean (RGB)
    cv::Scalar mCVScalarStd{ cv::Scalar(0.229, 0.224, 0.225) };   ///< ImageNet std (RGB)
} NomadMLClassificationBlobImageParameters;

/**
 * @class NomadMLClassificationThread
 * @brief Worker thread for asynchronous NOMAD ML classification.
 *
 * Identical functionality to NecMLClassificationThread with NOMAD-specific models.
 *
 * @see NecMLClassificationThread
 * @see NomadMLClassificationModel
 */
class NomadMLClassificationThread : public QThread
{
    Q_OBJECT
public:
    explicit
    NomadMLClassificationThread( QObject *parent = nullptr );

    ~NomadMLClassificationThread() override;

    void
    detect( const cv::Mat & );

    bool
    read_net( QString & );

    void
    setParams( NomadMLClassificationBlobImageParameters &, std::vector< std::string > &);

    NomadMLClassificationBlobImageParameters &
    getParams( ) { return mParams; }

Q_SIGNALS:
    void
    result_ready( cv::Mat &, QString );

protected:
    void
    run() override;

private:
    QSemaphore mWaitingSemaphore;
    QMutex mLockMutex;

    cv::Mat mCVImage;
    cv::dnn::Net mNomadMLClassification;
    std::vector<std::string> mvStrClasses;
    bool mbModelReady {false};
    bool mbAbort {false};

    NomadMLClassificationBlobImageParameters mParams;
};

/**
 * @class NomadMLClassificationModel
 * @brief Node model for NOMAD ML image classification.
 *
 * Provides the same functionality as NecMLClassificationModel but for
 * NOMAD-specific trained models. Supports custom class sets with
 * standardized preprocessing.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Input image to classify
 *
 * **Output Ports:**
 * 1. **CVImageData** - Annotated image with class label
 * 2. **SyncData** - Synchronization signal
 * 3. **InformationData** - Class name string
 *
 * **Properties:** (Same as NecML)
 * - model_filename
 * - config_filename (class labels)
 * - Preprocessing parameters
 *
 * @see NecMLClassificationModel (for detailed documentation)
 * @see NomadMLClassificationThread
 */
class NomadMLClassificationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    NomadMLClassificationModel();

    virtual
    ~NomadMLClassificationModel() override
    {
        if( mpNomadMLClassificationThread )
            delete mpNomadMLClassificationThread;
    }

    QJsonObject
    save() const override;

    void
    load(QJsonObject const &p) override;

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

    bool
    resizable() const override { return false; }

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

    NomadMLClassificationThread * mpNomadMLClassificationThread { nullptr };

    QString msDNNModel_Filename;
    QString msConfig_Filename;

    void processData(const std::shared_ptr< CVImageData > & in);
    void load_model(bool bUpdateDisplayProperties = false);
};
