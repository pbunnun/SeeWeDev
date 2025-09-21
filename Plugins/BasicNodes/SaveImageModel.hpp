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

#ifndef SAVEIMAGEMODEL_HPP
#define SAVEIMAGEMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QQueue>
#include <QDir>
#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "InformationData.hpp"
#include "SyncData.hpp"
#include "CVImageData.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class SavingImageThread : public QThread
{
    Q_OBJECT
public:
    explicit
    SavingImageThread( QObject *parent = nullptr );

    ~SavingImageThread() override;

    void
    add_new_image( const cv::Mat & image, QString filename );

    void
    set_saving_directory( QString dirname );

protected:
    void
    run() override;

private:
    bool mbAbort{false};
    QSemaphore mNoImageSemaphore;
    QQueue<cv::Mat> mImageQueue;
    QQueue<QString> mFilenameQueue;

    QDir mqDirname;
};

class SaveImageModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    SaveImageModel();

    virtual
    ~SaveImageModel() override
    {
        if( mpSavingImageThread )
            delete mpSavingImageThread;
    }

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p) override;

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex) override;

    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    late_constructor() override;

//    bool
//    resizable() const override { return true; }
    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:
    void
    inputConnectionCreated(QtNodes::Connection const& ) override;

    void
    inputConnectionDeleted(QtNodes::Connection const& ) override;

private:
    SavingImageThread * mpSavingImageThread { nullptr };

    std::shared_ptr< CVImageData > mpCVImageInData { nullptr };
    std::shared_ptr< InformationData > mpFilenameData { nullptr };
    std::shared_ptr< SyncData > mpSyncData{ nullptr };
#ifdef _WIN32
    QString msDirname{"C:\\"};
#else
    QString msDirname{"./"};
#endif
    bool mbUseProvidedFilename{false};
    bool mbSyncData2SaveImage{false};
    int miCounter{10000};

    QString msPrefix_Filename {"image"};
    QString msImage_Format {"jpg"};
};

#endif // SAVEIMAGEMODEL_HPP
