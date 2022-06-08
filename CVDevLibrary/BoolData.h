#ifndef BOOLDATA_H
#define BOOLDATA_H

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class BoolData : public InformationData
{
public:
    BoolData()
        : mbData()
    {}

    BoolData( const bool & boolean )
        : mbData( boolean )
    {}

    NodeDataType
    type() const override
    {
        return { "information", "Bln" };
    }

    bool &
    boolean()
    {
        return mbData;
    }

    void set_information() override
    {
        mQSData = mbData? QString("true") : QString("false") ;
    }

private:
    bool mbData;
};

#endif // BOOLDATA_H
