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

#ifndef TEST_SHARPENMODEL_H
#define TEST_SHARPENMODEL_H //include once

#pragma once //include once

#include <iostream> //input/output stream

#include <QtCore/QObject> //???
#include <QtWidgets/QLabel> //???

#include <nodes/DataModelRegistry> //???
#include "PBNodeDataModel.hpp" //propbox

#include "CVImageData.hpp" //type image obj

using QtNodes::PortType; //enum class
using QtNodes::PortIndex; //int
using QtNodes::NodeData; //base class of CVImageData
using QtNodes::NodeDataType; //use type() overridden by CVImageData


class Test_SharpenModel : public PBNodeDataModel //gains mbEnable
{
    Q_OBJECT //???

    public :

        Test_SharpenModel();
        virtual ~Test_SharpenModel() override;
        unsigned int nPorts(PortType PortType) const override;
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
        std::shared_ptr<NodeData> outData(PortIndex port) override;
        void setInData(std::shared_ptr<NodeData>nodeData,PortIndex) override;
        QWidget *embeddedWidget() override {return nullptr;}
        QPixmap minPixmap() const override {return _minPixmap;}

        static const QString _category;
        static const QString _model_name;

    private :

        std::shared_ptr<CVImageData> mpCVImageData = nullptr;
        //std::shared_ptr<CVImageData> mpCVImageInData = nullptr;
        QPixmap _minPixmap;

};

#endif // TEST_SHARPENMODEL_H
