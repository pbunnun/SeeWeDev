//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#ifndef ONNXCLASSIFICATIONDNNMODEL_HPP
#define ONNXCLASSIFICATIONDNNMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QMutex>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

#include "CVImageData.hpp"
#include "SyncData.hpp"
#include <opencv2/dnn.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

typedef struct OnnxClassificationDNNBlobImageParameters{
    double mdInvScaleFactor{255.};
    cv::Size mCVSize{ cv::Size(224,224) };
    cv::Scalar mCVScalarMean{ cv::Scalar(0.485, 0.456, 0.406) };
    cv::Scalar mCVScalarStd{ cv::Scalar(0.229, 0.224, 0.225) };
} OnnxClassificationDNNBlobImageParameters;

class OnnxClassificationDNNThread : public QThread
{
    Q_OBJECT
public:
    explicit
    OnnxClassificationDNNThread( QObject *parent = nullptr );

    ~OnnxClassificationDNNThread() override;

    void
    detect( const cv::Mat & );

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

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class OnnxClassificationDNNModel : public PBNodeDataModel
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
    restore(QJsonObject const &p) override;

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
    received_result( cv::Mat & );

private:
    std::shared_ptr< CVImageData > mpCVImageData { nullptr };
    std::shared_ptr<SyncData> mpSyncData;

    OnnxClassificationDNNThread * mpOnnxClassificationDNNThread { nullptr };

    QString msDNNModel_Filename;
    QString msClasses_Filename;

    void processData(const std::shared_ptr< CVImageData > & in);
    void load_model();
};
#endif
