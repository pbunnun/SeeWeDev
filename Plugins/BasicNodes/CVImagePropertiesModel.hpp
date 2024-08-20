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

#ifndef CVIMAGEPROPERTIESMODEL_HPP
#define CVIMAGEPROPERTIESMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include <opencv2/core.hpp>
#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;


typedef struct CVImagePropertiesProperties
{
    std::string msImageName;
    int miChannels;
    cv::Size mCVMSizeImage;

    bool mbIsBinary;
    bool mbIsBAndW;
    bool mbIsContinuous;
    std::string msDescription;
    CVImagePropertiesProperties()
        : msImageName("ImageName"),
          miChannels(0),
          mCVMSizeImage(cv::Size(0,0)),
          mbIsBinary(true),
          mbIsBAndW(true),
          mbIsContinuous(true),
          msDescription("-")
    {
    }
} CVImagePropertiesProperties;


class CVImagePropertiesModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CVImagePropertiesModel();

    virtual
    ~CVImagePropertiesModel() override {}

    QJsonObject
    save() const override;

    void
    restore(const QJsonObject &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:

    void processData(const std::shared_ptr< CVImageData > & in, CVImagePropertiesProperties & props );

    CVImagePropertiesProperties mProps;

    std::shared_ptr<CVImageData> mpCVImageInData {nullptr};

    QPixmap _minPixmap;
};

#endif // CVIMAGEPROPERTIESMODEL_HPP
