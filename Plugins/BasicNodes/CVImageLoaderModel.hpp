#ifndef CVIMAGELOADERMODEL_HPP
#define CVIMAGELOADERMODEL_HPP

#pragma once

#include <iostream>

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
