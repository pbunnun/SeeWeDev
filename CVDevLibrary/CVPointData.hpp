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
