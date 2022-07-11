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

#ifndef FACEDETECTIONDNNMODEL_HPP
#define FACEDETECTIONDNNMODEL_HPP

#pragma once

#include <iostream>

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

class FaceDetectorThread : public QThread
{
    Q_OBJECT
public:
    explicit
    FaceDetectorThread( QObject *parent = nullptr );

    ~FaceDetectorThread() override;

    void
    detect( const cv::Mat & );

    bool
    readNet( QString & , QString & );

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
    cv::dnn::Net mFaceDetector;
    bool mbModelReady {false};
    bool mbAbort {false};
};

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class FaceDetectionDNNModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    FaceDetectionDNNModel();

    virtual
    ~FaceDetectionDNNModel() override
    {
        if( mpFaceDetectorThread )
            delete mpFaceDetectorThread;
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

    FaceDetectorThread * mpFaceDetectorThread { nullptr };

    QString msDNNModel_Filename;
    QString msDNNConfig_Filename;

    void processData(const std::shared_ptr< CVImageData > & in);
    void load_model();
};
#endif
