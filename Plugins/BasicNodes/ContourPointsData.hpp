//Copyright © 2024, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#ifndef CONTOURPOINTSDATA_HPP
#define CONTOURPOINTSDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"
#include <opencv2/core/types.hpp>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class ContourPointsData : public InformationData
{
public:
    ContourPointsData()
        : mvvPoints()
    {}

    ContourPointsData(const std::vector< std::vector<cv::Point> > & data )
        : mvvPoints( data )
    {}

    NodeDataType
    type() const override
    {
        return { "Contours", "Cnt" };
    }

    std::vector< std::vector<cv::Point> > &
    data()
    {
        return mvvPoints;
    }

private:
    std::vector< std::vector<cv::Point> > mvvPoints;
};

#endif // CONTOURPOINTSDATA_HPP
