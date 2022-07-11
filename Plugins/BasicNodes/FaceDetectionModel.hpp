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
