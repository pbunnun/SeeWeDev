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

#ifndef STDVECTORNUMBERDATA_HPP
#define STDVECTORNUMBERDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

template <class T>
class StdVectorNumberData : public InformationData
{
public:
    StdVectorNumberData()
        : mvData()
    {}

    StdVectorNumberData(const std::vector<T> & data )
        : mvData( data )
    {}

    NodeDataType
    type() const override
    {
        return { "Numbers", "Nbs" };
    }

    std::vector<T> &
    data()
    {
        return mvData;
    }

    void set_information() override
    {
        mQSData  = QString("Data Type : std::vector \n");
        for (T value : mvData)
        {
            mQSData += QString::number(value);
            mQSData += "\n";
        }
    }

private:
    std::vector<T> mvData;
};

#define StdVectorIntData StdVectorNumberData<int>
#define StdVectorFloatData StdVectorNumberData<float>
#define StdVectorDoubleData StdVectorNumberData<double>
#endif // STDVECTORNUMBERDATA_HPP
