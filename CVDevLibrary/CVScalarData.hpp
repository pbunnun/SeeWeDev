#ifndef CVSCALARDATA_H
#define CVSCALARDATA_H

#pragma once

#include <opencv2/core.hpp>

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class CVScalarData : public InformationData
{
public:
    CVScalarData()
        : mCVScalar()
    {}

    CVScalarData( const cv::Scalar & scalar )
        : mCVScalar( scalar )
    {}

    NodeDataType
    type() const override
    {
        return { "information", "Scl" };
    }

    cv::Scalar &
    scalar()
    {
        return mCVScalar;
    }

    void set_information() override
    {
        mQSData = QString("(%1 , %2 , %3 , %4)")
                  .arg(mCVScalar[0]).arg(mCVScalar[1])
                  .arg(mCVScalar[2]).arg(mCVScalar[3]);
    }

private:
    cv::Scalar mCVScalar;
};

#endif // CVSCALARDATA_H
