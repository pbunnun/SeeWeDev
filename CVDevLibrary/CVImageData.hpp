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
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class CVImageData : public InformationData
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
        //       id       name
        return { "image", "Mat" };
    }

    void
    set_image (const cv::Mat &image )
    {
        image.copyTo( mCVImage );
        //mCVImage = image.clone(); RUT
    }

    cv::Mat &
    data()
    {
        return mCVImage;
    }

    void set_information() override
    {
        mQSData  = QString("Data Type\t : cv::Mat \n");
        if( !mCVImage.empty() )
        {
            mQSData += "Channels\t : " + QString::number( mCVImage.channels() ) + "\n";
            mQSData += "Depth\t : ";
            auto depth = mCVImage.depth();
            if( depth == CV_8U )
                mQSData += "CV_8U \n";
            else if( depth == CV_8S )
                mQSData += "CV_8S \n";
            else if( depth == CV_16U )
                mQSData += "CV_16U \n";
            else if( depth == CV_16S )
                mQSData += "CV_16S \n";
            else if( depth == CV_32S )
                mQSData += "CV_32S \n";
            else if( depth == CV_32F )
                mQSData += "CV_32F \n";
            else if( depth == CV_64F )
                mQSData += "CV_64F \n";
            mQSData += "WxH\t : " + QString::number( mCVImage.cols ) + " x " + QString::number( mCVImage.rows ) + "\n";
        }
    }

private:

    cv::Mat mCVImage;
};

#endif
