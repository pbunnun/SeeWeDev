#ifndef INTEGERDATA_HPP
#define INTEGERDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class IntegerData : public InformationData
{
public:
    IntegerData()
        : miData( 0 )
    {}

    IntegerData( const int data )
        : miData( data )
    {}

    NodeDataType
    type() const override
    {
        return { "Integer", "Int" };
    }

    int &
    number()
    {
        return miData;
    }

    void set_information() override
    {
        mQSData = QString::number(miData);
    }

private:
    int miData;
};

#endif // INTEGERDATA_HPP
