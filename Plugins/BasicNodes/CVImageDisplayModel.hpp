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

#ifndef CVIMAGEDISPLAYMODEL_HPP
#define CVIMAGEDISPLAYMODEL_HPP

#pragma once

#include <QtCore/QObject>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "SyncData.hpp"
#include "PBImageDisplayWidget.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class CVImageDisplayModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    CVImageDisplayModel();

    virtual
    ~CVImageDisplayModel() override {}

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    bool
    resizable() const override { return true; }

    bool
    eventFilter(QObject *object, QEvent *event) override;

    static const QString _category;

    static const QString _model_name;

private:
    void display_image( );

    PBImageDisplayWidget * mpEmbeddedWidget;

    cv::Mat mCVImageDisplay;
    //std::shared_ptr< NodeData > mpNodeData { nullptr };
    std::shared_ptr< SyncData > mpSyncData { nullptr };

    QPixmap _minPixmap;

    int miImageWidth{0};
    int miImageHeight{0};
    int miImageFormat{0};
};

#endif
