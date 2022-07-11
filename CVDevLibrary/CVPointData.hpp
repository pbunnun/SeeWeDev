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

#ifndef CVPOINTDATA_HPP
#define CVPOINTDATA_HPP

#include <opencv2/core/core.hpp>

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class CVPointData : public InformationData
{
public:

    CVPointData()
        : mCVPoint()
    {}

    CVPointData( const cv::Point & point )
        : mCVPoint( point )
    {}

    NodeDataType
    type() const override
    {
        return { "information", "Pnt" };
    }

    cv::Point &
    point()
    {
        return mCVPoint;
    }

    void set_information() override
    {
        mQSData = QString("(%1 , %2)").arg(mCVPoint.x).arg(mCVPoint.y);
    }

private:
    cv::Point mCVPoint;

};

#endif // CVPOINTDATA_HPP
