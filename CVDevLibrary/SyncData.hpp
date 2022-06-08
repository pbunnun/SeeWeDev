#ifndef SYNCDATA_HPP
#define SYNCDATA_HPP

#pragma once

#include <nodes/NodeDataModel>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class SyncData : public NodeData
{
public:
    SyncData()
        : mbSync(true)
    {
    }

    SyncData( const bool state )
        : mbSync(state)
    {}

    NodeDataType
    type() const override
    {
        return { "Sync", "Syc" };
    }

    bool&
    state()
    {
        return mbSync;
    }

    QString
    state_str() const
    {
        return mbSync? QString("Active") : QString("Inacive");
    }

private:
    bool mbSync;

};

#endif // SYNCDATA_HPP
