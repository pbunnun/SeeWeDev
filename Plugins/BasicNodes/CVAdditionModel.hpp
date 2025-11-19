//Copyright © 2025, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class CVAdditionModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVAdditionModel();
    virtual ~CVAdditionModel() override {}

    QJsonObject save() const override;
    void load(const QJsonObject &p) override;
    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;
    QWidget * embeddedWidget() override { return nullptr; }
    void setModelProperty( QString &, const QVariant & ) override;
    QPixmap minPixmap() const override { return _minPixmap; }

    static const QString _category;
    static const QString _model_name;

private Q_SLOTS:
    void inputConnectionCreated(QtNodes::ConnectionId const&) override;
    void inputConnectionDeleted(QtNodes::ConnectionId const&) override;

private:
    void processData(const std::vector< cv::Mat >&in, std::shared_ptr<CVImageData>& out);

    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::vector<cv::Mat> mvCVImageInData;
    bool mbMaskActive { false };
    QPixmap _minPixmap;
};
