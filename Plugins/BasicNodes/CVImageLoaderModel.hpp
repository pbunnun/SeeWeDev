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

#ifndef CVIMAGELOADERMODEL_HPP
#define CVIMAGELOADERMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"

#include "CVImageData.hpp"
#include "InformationData.hpp"
#include "CVSizeData.hpp"
#include "CVImageLoaderEmbeddedWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class CVImageLoaderModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CVImageLoaderModel();

    virtual
    ~CVImageLoaderModel() {}

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

private Q_SLOTS:
    void
    em_button_clicked( int button );

    void
    flip_image( );

    void
    inputConnectionCreated(QtNodes::Connection const&) override;

    void
    inputConnectionDeleted(QtNodes::Connection const&) override;

private:
    void
    set_image_filename(QString &);
    void
    set_dirname(QString &);

    QString msImageFilename {""};
    QString msDirname {""};
    std::vector<QString> mvsImageFilenames;
    int miFilenameIndex{ 0 };

    int miFlipPeriodInMillisecond{ 1000 };
    QTimer mTimer;
    bool mbLoop{true};

    CVImageLoaderEmbeddedWidget * mpEmbeddedWidget;

    std::shared_ptr< CVImageData > mpCVImageData;
    std::shared_ptr< InformationData > mpInformationData;
    std::shared_ptr< CVSizeData > mpCVSizeData ;

    bool mbInfoTime{true};
    bool mbInfoImageType{true};
    bool mbInfoImageFormat{true};
    bool mbInfoImageSize{true};
    bool mbInfoImageFilename{true};

    bool mbUseSyncSignal{false};
    bool mbSyncSignal{false};
};
#endif
