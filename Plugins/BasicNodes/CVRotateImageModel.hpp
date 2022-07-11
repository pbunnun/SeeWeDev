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

#ifndef CVROTATEIMAGEMODEL_HPP
#define CVROTATEIMAGEMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class CVRotateImageModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CVRotateImageModel();

    virtual
    ~CVRotateImageModel() override {}

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
    embeddedWidget() override { return nullptr; }

    /*
     * Recieve signals back from QtPropertyBrowser and use this function to
     * set parameters/variables accordingly.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /*
     * These two static members must be defined for every models. _category can be duplicate with existing categories.
     * However, _model_name has to be a unique name.
     */
    static const QString _category;

    static const QString _model_name;

private:
    void processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out );
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageOutData { nullptr };

    double mdAngle { 180.f };
};

#endif
