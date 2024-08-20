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

#include "DataGeneratorModel.hpp"

#include <QDebug> //for debugging using qDebug()

#include <nodes/DataModelRegistry>

#include "qtvariantproperty.h"

DataGeneratorModel::
DataGeneratorModel()
    : PBNodeDataModel( _model_name ),
      mpEmbeddedWidget( new DataGeneratorEmbeddedWidget ),
      _minPixmap( ":DataGenerator.png" )
{
    mpInformationData = std::make_shared<InformationData>();

    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect(mpEmbeddedWidget,&DataGeneratorEmbeddedWidget::widget_clicked_signal,this,&DataGeneratorModel::em_widget_clicked);

    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = mpEmbeddedWidget->get_combobox_string_list();
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "combobox_index";
    auto propComboboxIndex = std::make_shared< TypedProperty< EnumPropertyType > >("", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mMapIdToProperty[ propId ] = propComboboxIndex;

    propId = "text_input";
    QString textInput = mpEmbeddedWidget->get_text_input();
    auto propTextInput = std::make_shared< TypedProperty< QString > >("", propId, QMetaType::QString, textInput);
    mMapIdToProperty[ propId ] = propTextInput;
}

unsigned int
DataGeneratorModel::
nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 0;
        break;

    case PortType::Out:
        result = 1;
        break;

    default:
        break;
    }

    return result;
}


NodeDataType
DataGeneratorModel::
dataType(PortType, PortIndex) const
{
   return InformationData().type();
}


std::shared_ptr<NodeData>
DataGeneratorModel::
outData(PortIndex)
{
    if( isEnable() )
    {
        return mpInformationData;
    }
    return nullptr;
}

QJsonObject
DataGeneratorModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();

    QJsonObject cParams;
    cParams["comboboxIndex"] = mpEmbeddedWidget->get_combobox_index();
    cParams["textInput"] = mpEmbeddedWidget->get_text_input();
    modelJson["cParams"] = cParams;

    return modelJson;
}

void
DataGeneratorModel::
restore(QJsonObject const& p)
{
    PBNodeDataModel::restore(p);

    QJsonObject paramsObj = p[ "cParams" ].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "comboboxIndex" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "combobox_index" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
            typedProp->getData().miCurrentIndex = v.toInt();

            mpEmbeddedWidget->set_combobox_index(v.toInt());
        }
        v =  paramsObj[ "textInput" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "text_input" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< QString > >( prop );
            typedProp->getData() = v.toString();

            mpEmbeddedWidget->set_text_input(v.toString());
        }
    }
}

void DataGeneratorModel::em_widget_clicked()
{
    const int dataType = mpEmbeddedWidget->get_combobox_index();
    const QString input = mpEmbeddedWidget->get_text_input();
    processData(dataType,input,mpInformationData);
    Q_EMIT dataUpdated(0);
}

void
DataGeneratorModel::
processData(const int& dataType, const QString& input,
            std::shared_ptr<InformationData> &out)
{ //Define processes to validate user input
    out.reset();
    out = std::make_shared<InformationData>();
    if(dataType == GenData::INT)
    {
        bool ok;
        const int value = input.toInt(&ok);
        if(ok)
        {
            auto data = std::make_shared<IntegerData>(value);
            out = std::static_pointer_cast<InformationData>(data);
        }
    }
    else if(dataType == GenData::FLOAT)
    {
        bool ok;
        const float value = input.toFloat(&ok);
        if(ok)
        {
            auto data = std::make_shared<FloatData>(value);
            out = std::static_pointer_cast<InformationData>(data);
        }

    }
    else if(dataType == GenData::DOUBLE)
    {
        bool ok;
        const double value = input.toDouble(&ok);
        if(ok)
        {
            auto data = std::make_shared<DoubleData>(value);
            out = std::static_pointer_cast<InformationData>(data);
        }
    }
    else if(dataType == GenData::BOOL)
    {
        bool ok;
        const std::string value = input.toStdString();
        if(value == "0" || value == "false")
        {
            ok = false;
        }
        else if(value == "1" || value == "true")
        {
            ok = true;
        }
        else
        {
            return;
        }
        auto data = std::make_shared<BoolData>(ok);
        out = std::static_pointer_cast<InformationData>(data);

    }
    else if(dataType == GenData::STD_STRING)
    {
        const std::string value = input.toStdString();
        auto data = std::make_shared<StdStringData>(value);
        out = std::static_pointer_cast<InformationData>(data);
    }
    else if(dataType == GenData::CV_RECT)
    {
        const std::string value = input.toStdString();
        StringFormat sFormat("[?i?,?i?]@(?i?,?i?)"); //Object containing
        std::vector<std::string> matched; //string format for this NodeDataType
        sFormat.match(value,matched,true);
        if(matched.size()==4)
        {
            const int number[4] =
            {std::stoi(matched[0]),std::stoi(matched[1]),
            std::stoi(matched[2]),std::stoi(matched[3])};
            auto data = std::make_shared<CVRectData>
            (cv::Rect(number[2],number[3],number[0],number[1]));
            out = std::static_pointer_cast<InformationData>(data);
        }
    }
    else if(dataType == GenData::CV_POINT)
    {
        const std::string value = input.toStdString();
        StringFormat sFormat("(?i?,?i?)");
        std::vector<std::string> matched;
        sFormat.match(value,matched,true);
        if(matched.size()==2)
        {
            const int number[2] =
            {std::stoi(matched[0]),std::stoi(matched[1])};
            auto data = std::make_shared<CVPointData>(cv::Point(number[0],number[1]));
            out = std::static_pointer_cast<InformationData>(data);
        }
    }
    else if(dataType == GenData::CV_SCALAR)
    {
        const std::string value = input.toStdString();
        StringFormat sFormat("(?i?,?i?,?i?,?i?)");
        std::vector<std::string> matched;
        sFormat.match(value,matched,true);
        if(matched.size()==4)
        {
            const int number[4] =
            {std::stoi(matched[0]),std::stoi(matched[1]),
            std::stoi(matched[2]),std::stoi(matched[3])};
            auto data = std::make_shared<CVScalarData>
            (cv::Scalar(number[0],number[1],number[2],number[3]));
            out = std::static_pointer_cast<InformationData>(data);
        }
    }
}

const QString DataGeneratorModel::_category = QString( "Number Operation" );

const QString DataGeneratorModel::_model_name = QString( "Data Generator" );

const std::string StringFormat::Placeholder::placeholder_str = "?s?";
const std::string StringFormat::Placeholder::placeholder_int = "?i?";

const std::vector<std::string> StringFormat::Placeholder::placeholders =
{placeholder_str,placeholder_int};

const std::string StringFormat::Placeholder::placeholder_match = "B0@";

int StringFormat::placeholder_count(const Placeholder::PlaceholderKey type) const
{
    std::string placeholder;
    int count = 0;

    switch(type)
    {
    case Placeholder::ALL :
        break;

    case Placeholder::STD_STRING :
        placeholder = Placeholder::placeholder_str;
        break;

    case Placeholder::INT :
        placeholder = Placeholder::placeholder_int;
        break;
    }

    const auto length = format.length();
    unsigned long long charIndex = 0;

    if(format.find(placeholder,charIndex) == std::string::npos)
    {
        return count;
    }
    while(charIndex < length)
    {
        charIndex += format.find(placeholder,charIndex);
        charIndex += placeholder.length();
    }
    return count;
}

std::string StringFormat::compress(std::string text) const
{
    unsigned long long charIndex = 0;
    while(charIndex < text.length())
    {
        if(text[charIndex] == ' ')
        {
            text.erase(text.begin()+charIndex);
            continue;
        }
        charIndex++;
    }
    return text;
}

void StringFormat::compress(std::string* text) const
{
    unsigned long long charIndex = 0;
    while(charIndex < text->length())
    {
        if((*text)[charIndex] == ' ')
        {
            text->erase(text->begin()+charIndex);
            continue;
        }
        charIndex++;
    }
    return;
}

std::vector<std::string>
StringFormat::
split(const std::string &text, const std::string key) const
{
    auto splitText = std::vector<std::string>();
    const auto length = text.length();
    const auto indexSkip = key.length();
    unsigned long long charIndex = 0;
    while(charIndex < length)
    {
        auto foundIndex = text.find(key, charIndex);
        if(foundIndex == std::string::npos)
        {
            const std::string appendText = text.substr(charIndex,length-charIndex);
            splitText.push_back(appendText);
            return splitText;
        }
        const std::string appendText = text.substr(charIndex,foundIndex-charIndex);
        if(appendText.length() != 0)
        {
            splitText.push_back(appendText);
        }
        charIndex = foundIndex + indexSkip;
    }
    return splitText;
}

void
StringFormat::
split(const std::string& text, std::vector<std::string>& splitText, const std::string key) const
{
    const auto length = text.length();
    const auto indexSkip = key.length();
    unsigned long long charIndex = 0;
    while(charIndex < length)
    {
        auto foundIndex = text.find(key, charIndex);
        if(foundIndex == std::string::npos)
        {
            const std::string appendText = text.substr(charIndex,length-charIndex);
            splitText.push_back(appendText);
            return;
        }
        const std::string appendText = text.substr(charIndex,foundIndex-charIndex);
        if(appendText.length() != 0)
        {
            splitText.push_back(appendText);
        }
        charIndex = foundIndex + indexSkip;
    }
    return;
}

std::vector<std::string>
StringFormat::
split(const std::string &text, std::vector<std::string> keyList) const
{
    auto splitText = std::vector<std::string>();
    const auto length = text.length();
    unsigned long long charIndex = 0;
    while(charIndex < length)
    {
        bool found = false;
        auto foundIndex = text.length();
        unsigned long long indexSkip;
        for(const std::string& key : keyList)
        {
            auto tempIndex = text.find(key, charIndex);
            if(tempIndex < foundIndex && tempIndex!=std::string::npos)
            {
                indexSkip = key.length();
                foundIndex = tempIndex;
                found = true;
            }
        }
        if(!found)
        {
            const std::string appendText = text.substr(charIndex,length-charIndex);
            if(appendText.length() != 0)
            {
                splitText.push_back(text.substr(charIndex,length-charIndex));
            }
            return splitText;
        }
        const std::string appendText = text.substr(charIndex,foundIndex-charIndex);
        if(appendText.length() != 0)
        {
            splitText.push_back(appendText);
        }
        charIndex = foundIndex + indexSkip;
    }
    return splitText;
}

void
StringFormat::
split(const std::string &text, std::vector<std::string>& splitText, std::vector<std::string> keyList) const
{
    const auto length = text.length();
    unsigned long long charIndex = 0;
    while(charIndex < length)
    {
        bool found = false;
        auto foundIndex = text.length();
        unsigned long long indexSkip;
        for(const std::string& key : keyList)
        {
            auto tempIndex = text.find(key, charIndex);
            if(tempIndex < foundIndex && tempIndex!=std::string::npos)
            {
                indexSkip = key.length();
                foundIndex = tempIndex;
                found = true;
            }
        }
        if(!found)
        {
            const std::string appendText = text.substr(charIndex,length-charIndex);
            if(appendText.length() != 0)
            {
                splitText.push_back(text.substr(charIndex,length-charIndex));
            }
            return;
        }
        const std::string appendText = text.substr(charIndex,foundIndex-charIndex);
        if(appendText.length() != 0)
        {
            splitText.push_back(appendText);
        }
        charIndex = foundIndex + indexSkip;
    }
    return;
}

std::vector<std::string>
StringFormat::
match(std::string text, const bool ignore_gaps) const
{
    std::string f;
    if(ignore_gaps)
    {
        this->compress(&text);
        f = this->compress(format);
    }
    else
    {
        f = format;
    }
    std::vector<std::string> matched;
    std::vector<std::string> splitFormat;
    split(f,splitFormat,Placeholder::placeholders);
    const auto splitPlaceholders = split(f,splitFormat);
    for(const std::string& literal : splitFormat)
    {
        const auto index = text.find(literal);
        if(index == std::string::npos)
        {
            return std::vector<std::string>();
        }
        text.replace(text.begin()+index,
                     text.begin()+literal.length()-1,
                     Placeholder::placeholder_match);
    }
    split(text,matched,Placeholder::placeholder_match);
    if(matched.size()!=splitPlaceholders.size())
    {
        return std::vector<std::string>();
    }
    for(size_t i=0; i<splitPlaceholders.size(); i++)
    {
        QString check = QString::fromStdString(matched[i]);
        bool ok = false;
        if(splitPlaceholders[i] == Placeholder::placeholder_str)
        {
            //lol
        }
        else if(splitPlaceholders[i] == Placeholder::placeholder_int)
        {
            check.toInt(&ok);
        }

        if(!ok)
        {
            return std::vector<std::string>();
        }
    }
    return matched;
}

void
StringFormat::
match(std::string text, std::vector<std::string>& matched, const bool ignore_gaps) const
{
    std::string f;
    if(ignore_gaps)
    {
        this->compress(&text);
        f = this->compress(format);
    }
    else
    {
        f = format;
    }
    std::vector<std::string> splitFormat;
    split(f,splitFormat,Placeholder::placeholders);
    const auto splitPlaceholders = split(f,splitFormat);
    for(const std::string& literal : splitFormat)
    {
        const auto index = text.find(literal);
        if(index == std::string::npos)
        {
            return;
        }
        text.replace(index,
                     literal.length(),
                     Placeholder::placeholder_match);
    }
    split(text,matched,Placeholder::placeholder_match);
    if(matched.size()!=splitPlaceholders.size())
    {
        return;
    }
    for(size_t i=0; i<splitPlaceholders.size(); i++)
    {
        QString check = QString::fromStdString(matched[i]);
        bool ok = false;
        if(splitPlaceholders[i] == Placeholder::placeholder_str)
        {
            //lol
        }
        else if(splitPlaceholders[i] == Placeholder::placeholder_int)
        {
            check.toInt(&ok);
        }

        if(!ok)
        {
            return;
        }
    }
    return;
}
