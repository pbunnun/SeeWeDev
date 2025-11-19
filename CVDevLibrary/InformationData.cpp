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

#include "InformationData.hpp"
#include <chrono>

InformationData::InformationData()
    : mQSData(""), mlTimeStamp(0)
{
}

InformationData::InformationData(QString const &text)
    : mQSData(text), mlTimeStamp(0)
{
}

NodeDataType InformationData::type() const
{
    return NodeDataType{"Information", "Inf"};
}

void InformationData::set_information()
{
    // Base implementation does nothing
}

void InformationData::set_information(QString const &text)
{
    mQSData = text;
}

void InformationData::set_timestamp(long int const &time)
{
    mlTimeStamp = time;
}

void InformationData::set_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    mlTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

QString const & InformationData::info() const
{
    return mQSData;
}

long int const & InformationData::timestamp() const
{
    return mlTimeStamp;
}
