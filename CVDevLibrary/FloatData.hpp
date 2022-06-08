#ifndef FLOATDATA_HPP
#define FLOATDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class FloatData : public InformationData
{
public:
    FloatData()
        : mfData( 0 )
    {}

    FloatData( const float data )
        : mfData( data )
    {}

    NodeDataType
    type() const override
    {
        return { "Float", "Flt" };
    }

    float &
    number()
    {
        return mfData;
    }

    void set_information() override
    {
        mQSData = QString::number(mfData);
    }

private:
    float mfData;
};

#endif // FLOATDATA_HPP
