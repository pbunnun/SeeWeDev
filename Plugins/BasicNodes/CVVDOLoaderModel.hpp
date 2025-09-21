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

#ifndef CVVDOLOADERMODEL_HPP
#define CVVDOLOADERMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QTimer>
#include <QElapsedTimer>
#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

#include "CVImageData.hpp"
#include <opencv2/videoio.hpp>
#include "InformationData.hpp"
#include "CVSizeData.hpp"
#include "CVVDOLoaderEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class CVVDOLoaderModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CVVDOLoaderModel();

    virtual
    ~CVVDOLoaderModel() {}

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
    setInData(std::shared_ptr<NodeData>, int) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    bool
    resizable() const override { return true; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;

    static const QString _model_name;

    int getMaxNoFrames() const;
    void setMaxNoFrames(int newMaxNoFrames);

private Q_SLOTS:
    void
    enable_changed( bool ) override;

    void
    em_button_clicked( int button );

    void
    no_frame_changed( int no_frame );

    void
    next_frame( );

    void
    inputConnectionCreated(QtNodes::Connection const&) override;

    void
    inputConnectionDeleted(QtNodes::Connection const&) override;

private:
    void
    set_video_filename(QString &);

    QString msVideoFilename {""};
    int miFlipPeriodInMillisecond{100};
    QTimer mTimer;
    bool mbLoop {true};
    bool mbCapturing {false};
    QString msImage_Format{ "CV_8UC3" };
    cv::Size mcvImage_Size{ cv::Size(320,240) };
    int miNextFrame{ 0 };
    int miMaxNoFrames{ 0 };


    CVVDOLoaderEmbeddedWidget * mpEmbeddedWidget;
    cv::VideoCapture mcvVideoCapture;

    std::shared_ptr< CVImageData > mpCVImageData;

    bool mbUseSyncSignal{false};
    bool mbSyncSignal{false};
};
#endif
