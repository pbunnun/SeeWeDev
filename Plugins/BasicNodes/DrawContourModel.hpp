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

#ifndef DRAWCONTOURMODEL_HPP
#define DRAWCONTOURMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <opencv2/imgproc.hpp>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "ContourPointsData.hpp"


using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

typedef struct DrawContourParameters{
    int mucBValue;
    int mucGValue;
    int mucRValue;
    int miLineThickness;
    int miLineType;
    DrawContourParameters()
        : mucBValue(0),
          mucGValue(255),
          mucRValue(0),
          miLineThickness(2),
          miLineType(0)
    {
    }
} DrawContourParameters;

class DrawContourModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    DrawContourModel();

    virtual
    ~DrawContourModel() override {}

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
    embeddedWidget() override {return nullptr;}

    /*
     * Recieve signals back from QtPropertyBrowser and use this function to
     * set parameters/variables accordingly.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:
    DrawContourParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageOutData { nullptr };
    std::shared_ptr<ContourPointsData> mpContourPointsData {nullptr};
    QPixmap _minPixmap;

    void processData(const std::shared_ptr<CVImageData>& in, std::shared_ptr<CVImageData>& outImage,
                     std::shared_ptr<ContourPointsData> &ctrPnts, const DrawContourParameters& params);
};

#endif // DRAWCONTOURMODEL_HPP
