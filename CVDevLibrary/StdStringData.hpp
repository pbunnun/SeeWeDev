#ifndef STDSTRINGDATA_HPP
#define STDSTRINGDATA_HPP

#pragma once

#include <nodes/NodeDataModel>
#include "InformationData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class StdStringData : public InformationData
{
public:
    StdStringData()
        : msData()
    {
    }

    StdStringData( std::string const & string )
        : msData( string )
    {}

    NodeDataType
    type() const override
    {
        return { "information", "Str" };
    }

    std::string
    text() const
    {
        return msData;
    }

    void set_information() override
    {
        mQSData = QString::fromStdString(msData);
    }

private:
    std::string msData;
};

#endif // STDSTRINGDATA_HPP
