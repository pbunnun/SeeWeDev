#ifndef DOUBLEDATA_HPP
#define DOUBLEDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class DoubleData : public InformationData
{
public:
    DoubleData()
        : mdData( 0 )
    {}

    DoubleData( const double data )
        : mdData( data )
    {}

    NodeDataType
    type() const override
    {
        return { "Double", "Dbl" };
    }

    double &
    number()
    {
        return mdData;
    }

    void set_information() override
    {
        mQSData = QString::number(mdData);
    }

private:
    double mdData;
};

#endif // DOUBLEDATA_HPP
