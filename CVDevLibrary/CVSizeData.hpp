#ifndef CVSIZEDATA_HPP
#define CVSIZEDATA_HPP

#include <opencv2/core/core.hpp>

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class CVSizeData : public InformationData
{
public:

    CVSizeData()
        : mCVSize()
    {}

    CVSizeData( const cv::Size & size )
        : mCVSize( size )
    {}

    NodeDataType
    type() const override
    {
        return { "Size", "Sze" };
    }

    cv::Size &
    size()
    {
        return mCVSize;
    }

    void set_information() override
    {
        mQSData = QString("[%1 px x %2 px]").arg(mCVSize.height).arg(mCVSize.width);
    }

private:
    cv::Size mCVSize;

};

#endif // CVSIZEDATA_HPP
