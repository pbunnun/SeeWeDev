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

#ifndef FINDANDDRAWCONTOURMODEL_HPP
#define FINDANDDRAWCONTOURMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <opencv2/imgproc.hpp>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"


using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

typedef struct FindAndDrawContourParameters{
    int miContourMode;
    int miContourMethod;
    int mucBValue;
    int mucGValue;
    int mucRValue;
    int miLineThickness;
    int miLineType;
    FindAndDrawContourParameters()
        : miContourMode(1),
          miContourMethod(1),
          mucBValue(0),
          mucGValue(255),
          mucRValue(0),
          miLineThickness(2),
          miLineType(0)
    {
    }
} FindAndDrawContourParameters;

class FindAndDrawContourModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    FindAndDrawContourModel();

    virtual
    ~FindAndDrawContourModel() override {}

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

    static const QString _category;

    static const QString _model_name;


private:
    FindAndDrawContourParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<IntegerData> mpIntegerData {nullptr};

    void processData(const std::shared_ptr<CVImageData>& in, std::shared_ptr<CVImageData>& outImage,
                     std::shared_ptr<IntegerData> &outInt, const FindAndDrawContourParameters& params);
    float get_stddev(const std::vector<float> & vec, float mean );
};

#endif // FINDANDDRAWCONTOURMODEL_HPP
