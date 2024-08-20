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

#ifndef DATAGENERATORMODEL_HPP
#define DATAGENERATORMODEL_HPP

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "DataGeneratorEmbeddedWidget.hpp"
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "IntegerData.hpp"
#include "InformationData.hpp"
#include "FloatData.hpp"
#include "DoubleData.hpp"
#include "BoolData.h"
#include "StdStringData.hpp"
#include "CVRectData.hpp"
#include "CVPointData.hpp"
#include "CVScalarData.h"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

typedef struct GenData
{
    enum GenDataType
    {
        INVALID = -1,
        INT = 0,
        FLOAT = 1,
        DOUBLE = 2,
        BOOL = 3,
        STD_STRING = 4,
        CV_RECT = 5,
        CV_POINT = 6,
        CV_SCALAR = 7
    };

} GenData;

class DataGeneratorModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    DataGeneratorModel();

    virtual
    ~DataGeneratorModel() override {}

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
    setInData(std::shared_ptr<NodeData>, PortIndex) override {};

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS :

    void em_widget_clicked();

private:

    std::shared_ptr<InformationData> mpInformationData {nullptr};

    DataGeneratorEmbeddedWidget* mpEmbeddedWidget;

    QPixmap _minPixmap;

    void processData(const int& dataType, const QString& input,
                     std::shared_ptr<InformationData> &out);
};

class StringFormat
{
public :

    typedef struct Placeholder //define str and enum of a placeholder here
    {

        static const std::string placeholder_str;
        static const std::string placeholder_int;

        static const std::vector<std::string> placeholders;

        static const std::string placeholder_match;

        enum PlaceholderKey
        {
            ALL = 0,
            STD_STRING = 1,
            INT = 2
        };

    } Placeholder;

private :

    std::string format;

public :

    StringFormat()
        : format(std::string())
    {
    }

    StringFormat(const std::string f)
        : format(f)
    {
    }

    ~StringFormat() = default;

    int placeholder_count(const Placeholder::PlaceholderKey type) const;

    std::string compress(std::string text) const;
    void compress(std::string* text) const;

    std::vector<std::string> split(const std::string& text, const std::string key) const;
    void split(const std::string& text, std::vector<std::string>& splitText, const std::string key) const;
    std::vector<std::string> split(const std::string& text, const std::vector<std::string> keyList) const;
    void split(const std::string& text, std::vector<std::string>& splitText, const std::vector<std::string> keyList) const;

    std::vector<std::string> match(std::string text, const bool ignore_gaps = false) const;
    void match(std::string text, std::vector<std::string>& matched, const bool ignore_gaps = false) const;

};

#endif // DATAGENERATORMODEL_HPP
