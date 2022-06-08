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
