#ifndef ONNXCLASSIFICATIONDNNMODEL_HPP
#define ONNXCLASSIFICATIONDNNMODEL_HPP

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

typedef struct OnnxClassificationDNNBlobImageParameters{
    double mdInvScaleFactor{255};
    cv::Size mCVSize{ cv::Size(300,300) };
    cv::Scalar mCVScalarMean{ cv::Scalar(127.5, 127.5, 127.5) };
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
