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

#ifndef INFORMATIONDATA_HPP
#define INFORMATIONDATA_HPP

#pragma once

#include <nodes/NodeDataModel>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class InformationData : public NodeData
{
public:
    InformationData()
    {
        mQSData = "";
    }

    InformationData( QString & info )
        : mQSData( info )
    {}

    virtual NodeDataType
    type() const override
    {
        return { "Information", "Inf" };
    }

    virtual void
    set_information() {};

    void
    set_information(const QString& inf)
    {
        mQSData = inf;
    }

    void
    set_timestamp( long int timestamp )
    {
        miMillisecsSinceEpoch = timestamp;
    }

    void
    set_timestamp()
    {
        miMillisecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();
    }

    QString
    info() const
    {
        return mQSData;
    }

    long int
    timestamp() const
    {
        return miMillisecsSinceEpoch;
    }

protected:
    QString mQSData{};
    long int miMillisecsSinceEpoch;
};

#endif // INFORMATIONDATA_HPP
