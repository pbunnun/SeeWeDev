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

#ifndef DOUBLEDATA_HPP
#define DOUBLEDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class DoubleData : public InformationData
{
public:
    DoubleData()
        : mdData( 0 )
    {}

    DoubleData( const double data )
        : mdData( data )
    {}

    NodeDataType
    type() const override
    {
        return { "Double", "Dbl" };
    }

    double &
    number()
    {
        return mdData;
    }

    void set_information() override
    {
        mQSData = QString::number(mdData);
    }

private:
    double mdData;
};

#endif // DOUBLEDATA_HPP
