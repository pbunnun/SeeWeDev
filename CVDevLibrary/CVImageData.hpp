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
