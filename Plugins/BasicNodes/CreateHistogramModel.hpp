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

#ifndef CREATEHISTOGRAMMODEL_HPP
#define CREATEHISTOGRAMMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
//ucharbin(mod(range)==0),ucharrange_max,ucharrange_min,intthic,intlinetype
typedef struct CreateHistogramParameters{
    int miBinCount;
    double mdIntensityMax;
    double mdIntensityMin;
    int miNormType;
    int miLineThickness;
    int miLineType;
    bool mbDrawEndpoints;
    bool mbEnableB;
    bool mbEnableG;
    bool mbEnableR;
    CreateHistogramParameters()
        : miBinCount(256),
          mdIntensityMax(256),
          mdIntensityMin(0),
          miNormType(cv::NORM_MINMAX),
          miLineThickness(2),
          miLineType(cv::LINE_8),
          mbDrawEndpoints(true),
          mbEnableB(true),
          mbEnableG(true),
          mbEnableR(true)
    {
    }
} CreateHistogramParameters;


class CreateHistogramModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CreateHistogramModel();

    virtual
    ~CreateHistogramModel() override {}

    QJsonObject
    save() const override;

    void
    restore(const QJsonObject &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:
    CreateHistogramParameters mParams;
    std::shared_ptr< CVImageData > mpCVImageData { nullptr };
    std::shared_ptr< CVImageData > mpCVImageInData { nullptr };
    QPixmap _minPixmap;

    void
    processData( const std::shared_ptr< CVImageData > & in, std::shared_ptr< CVImageData > & out,
                 const CreateHistogramParameters & params );
};

#endif // CREATEHISTOGRAMMODEL_HPP
