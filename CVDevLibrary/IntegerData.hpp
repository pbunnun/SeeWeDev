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

#ifndef INTEGERDATA_HPP
#define INTEGERDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class IntegerData : public InformationData
{
public:
    IntegerData()
        : miData( 0 )
    {}

    IntegerData( const int data )
        : miData( data )
    {}

    NodeDataType
    type() const override
    {
        return { "Integer", "Int" };
    }

    int &
    data()
    {
        return miData;
    }

    void set_information() override
    {
        mQSData = QString("Data Type : int \n");
        mQSData += QString::number(miData);
        mQSData += QString("\n");
    }

private:
    int miData;
};

#endif // INTEGERDATA_HPP
