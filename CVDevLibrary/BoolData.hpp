#ifndef BOOLDATA_HPP
#define BOOLDATA_HPP

#pragma once

#include <nodes/NodeDataModel>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class BoolData : public NodeData
{
public:
    BoolData()
        : mbBool(true)
    {
    }

    BoolData( const bool state )
        : mbBool(state)
    {}

    NodeDataType
    type() const override
    {
        return { "Boolean", "Bln" };
    }

    bool&
    state()
    {
        return mbBool;
    }

    QString
    state_str() const
    {
        return mbBool? QString("True") : QString("False");
    }

private:
    bool mbBool;

};

#endif // BOOLDATA_HPP
