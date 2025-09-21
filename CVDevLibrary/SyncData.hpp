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

#ifndef SYNCDATA_HPP
#define SYNCDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class SyncData : public InformationData
{
public:
    SyncData()
        : mbSync(false)
    {
    }

    SyncData( const bool state )
        : mbSync(state)
    {}

    NodeDataType
    type() const override
    {
        return { "Sync", "Syc" };
    }

    bool&
    data()
    {
        return mbSync;
    }

    void
    set_data( bool data )
    {
        mbSync = data;
        InformationData::set_timestamp();
    }

    QString
    state_str() const
    {
        return mbSync? QString("Active") : QString("Inacive");
    }

    void set_information() override
    {
        mQSData = QString("Data Type : Sync \n");
        mQSData += state_str();
        mQSData += QString("\n");
    }

private:
    bool mbSync;

};

#endif // SYNCDATA_HPP
