#ifndef TEXTDETECTIONDNNMODEL_HPP
#define TEXTDETECTIONDNNMODEL_HPP

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

typedef struct TextDetectionDBParameters{
    float mfBinaryThreshold{0.3};
    float mfPolygonThreshold{0.5};
    double mdUnclipRatio{2.0};
    int miMaxCandidate{200};
    cv::Size mCVSize{ cv::Size(736,736) };
} TextDetectionDBParameters;

class TextDetectionDBThread : public QThread
{
    Q_OBJECT
public:
    explicit
    TextDetectionDBThread( QObject *parent = nullptr );

    ~TextDetectionDBThread() override;

    void
    detect( const cv::Mat & );

    bool
    readNet( QString & );

    void
    setParams( TextDetectionDBParameters &);

    TextDetectionDBParameters &
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
    cv::dnn::TextDetectionModel_DB mTextDetectionDNN;
    bool mbModelReady {false};
    bool mbAbort {false};

    TextDetectionDBParameters mParams;
};

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class TextDetectionDNNModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    TextDetectionDNNModel();

    virtual
    ~TextDetectionDNNModel() override
    {
        if( mpTextDetectionDNNThread )
            delete mpTextDetectionDNNThread;
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

    TextDetectionDBThread * mpTextDetectionDNNThread { nullptr };

    QString msDBModel_Filename;

    void processData(const std::shared_ptr< CVImageData > & in);
    void load_model();
};
#endif
