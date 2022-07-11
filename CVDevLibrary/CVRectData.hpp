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

#ifndef CVRECTDATA_HPP
#define CVRECTDATA_HPP

#pragma once

#include <opencv2/core/core.hpp>

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class CVRectData : public InformationData
{
public:

    CVRectData()
        : mCVRect()
    {}

    CVRectData( const cv::Rect & rect )
        : mCVRect( rect )
    {}

    NodeDataType
    type() const override
    {
        return { "information", "Rct" };
    }

    cv::Rect &
    rect()
    {
        return mCVRect;
    }

    void set_information() override
    {
        mQSData = QString("[%1 px x %2 px] @ (%3 , %4)")
                  .arg(mCVRect.width).arg(mCVRect.height)
                  .arg(mCVRect.x).arg(mCVRect.y);
    }

private:
    cv::Rect mCVRect;

};

#endif // CVRECTDATA_HPP
