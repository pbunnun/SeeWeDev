#ifndef INFORMATIONDATA_HPP
#define INFORMATIONDATA_HPP

#pragma once

#include <nodes/NodeDataModel>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class InformationData : public NodeData
{
public:
    InformationData()
    {
        mQSData = "No Information!";
    }

    InformationData( QString & info )
        : mQSData( info )
    {}

    virtual NodeDataType
    type() const override
    {
        return { "Information", "Inf" };
    }

    virtual void
    set_information() {};

    void set_information(const QString& inf)
    {
        mQSData = inf;
    }

    QString
    info() const
    {
        return mQSData;
    }

protected:
    QString mQSData;

};

#endif // INFORMATIONDATA_HPP
