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

#ifndef CVIMAGEDATA_HPP
#define CVIMAGEDATA_HPP

#pragma once

#include <opencv2/core/core.hpp>

#include <nodes/NodeDataModel>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class CVImageData : public NodeData
{
public:

    CVImageData()
        : mCVImage()
    {}

    CVImageData( const cv::Mat &image )
    {
        mCVImage = image.clone();
    }

    NodeDataType
    type() const override
    {
        //       id      name
        return { "image", "Mat" };
    }

    void
    set_image (const cv::Mat &image )
    {
        mCVImage = image.clone();
    }

    cv::Mat &
    image()
    {
        return mCVImage;
    }

    //cv::Mat
    //image() const { return mCVImage; }

private:

    cv::Mat mCVImage;
};

#endif
