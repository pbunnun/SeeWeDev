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

#ifndef CVCAMERAMODEL_HPP
#define CVCAMERAMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

#include "SyncData.hpp"
#include "CVImageData.hpp"
#include "InformationData.hpp"

#include "CVCameraEmbeddedWidget.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;
using QtNodes::Connection;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class CVCameraThread : public QThread
{
    Q_OBJECT
public:
    explicit
    CVCameraThread( QObject *parent, std::shared_ptr< CVImageData > pCVImageData );

    ~CVCameraThread() override;

    void
    set_camera_id( int camera_id );

    void
    set_single_shot_mode( const bool mode ) { mSingleShotSemaphore.release(); mbSingleShotMode = mode; };

    void
    fire_single_shot( ) { mSingleShotSemaphore.release(); };

    double
    get_fps( ) { return mdFPS; };

Q_SIGNALS:
    void
    image_ready( );

    bool
    camera_ready( bool status );

protected:
    void
    run() override;

private:
    void
    check_camera();

    QSemaphore mCameraCheckSemaphore;
    QSemaphore mSingleShotSemaphore;

    int miCameraID{-1};
    bool mbAbort{false};
    bool mbSingleShotMode{false};
    bool mbConnected{false};
    unsigned long miDelayTime{10};
    double mdFPS{0};
    cv::VideoCapture mCVVideoCapture;

    std::shared_ptr< CVImageData > mpCVImageData;
};

class CVCameraModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CVCameraModel();

    virtual
    ~CVCameraModel() override
    {
        if( mpCVCameraThread )
            delete mpCVCameraThread;
    }

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    void
    late_constructor() override;

    static const QString _category;

    static const QString _model_name;

    void setSelected(bool selected) override;

    bool
    resizable() const override { return true; }

private Q_SLOTS:
    void
    received_image();

    void
    camera_status_changed( bool );

    void
    em_button_clicked( int button );

    void
    enable_changed( bool ) override;

    void
    inputConnectionCreated( QtNodes::Connection const & ) override { mpCVCameraThread->set_single_shot_mode( true ); };

    void
    inputConnectionDeleted( QtNodes::Connection const & ) override { mpCVCameraThread->set_single_shot_mode( false ); };

private:
    CVCameraParameters mParams;
    CVCameraEmbeddedWidget * mpEmbeddedWidget;

    CVCameraThread * mpCVCameraThread { nullptr };

    std::shared_ptr< SyncData > mpSyncInData { nullptr };
    std::shared_ptr< CVImageData > mpCVImageData;
    std::shared_ptr< InformationData > mpInformationData;
};

#endif
