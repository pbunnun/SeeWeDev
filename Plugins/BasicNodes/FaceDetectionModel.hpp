#ifndef FACEDETECTIONMODEL_H
#define FACEDETECTIONMODEL_H

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "FaceDetectionEmbeddedWidget.hpp"

#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class FaceDetectionModel : public PBNodeDataModel {
    Q_OBJECT

    public:
        FaceDetectionModel();

        virtual
        ~FaceDetectionModel() override {}

        unsigned int
        nPorts(PortType portType) const override;

        NodeDataType
        dataType(PortType portType, PortIndex portIndex) const override;

        std::shared_ptr<NodeData>
        outData(PortIndex port) override;

        void
        setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

        QWidget *
        embeddedWidget() override { return mpEmbeddedWidget; }
    
        void
        setModelProperty( QString &, const QVariant & ) override;
    
        QJsonObject
        save() const override;

        QPixmap
        minPixmap() const override {
            return _minPixmap;
        }

        static const QString _category;

        static const QString _model_name;
    
    private Q_SLOTS:
        void
        em_button_clicked( int button );

    private:
        FaceDetectionEmbeddedWidget * mpEmbeddedWidget;
        std::shared_ptr<CVImageData> mpCVImageData {
            nullptr
        };
        QPixmap _minPixmap;
        static cv::Mat processData(const std::shared_ptr<CVImageData> &p);
};

#endif
