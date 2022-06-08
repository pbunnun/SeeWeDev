#ifndef SCALAROPERATIONMODEL_HPP
#define SCALAROPERATIONMODEL_HPP //Requires VectorOperation for Data types
                                 //such as cv::Rect and cv::Scalar
#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "InformationData.hpp"
#include "IntegerData.hpp"
#include "FloatData.hpp"
#include "DoubleData.hpp"
#include "BoolData.h"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
///

struct SclOps
{
    enum Operators
    {
        PLUS = 0,
        MINUS = 1,
        GREATER_THAN = 2,
        GREATER_THAN_OR_EQUAL = 3,
        LESSER_THAN = 4,
        LESSER_THAN_OR_EQUAL = 5,
        MULTIPLY = 6,
        DIVIDE = 7,
        MAXIMUM = 8,
        MINIMUM = 9,
        EQUAL = 10,
        AND = 11,
        OR = 12,
        XOR = 13,
        NOR = 14,
        NAND = 15
    };
};

typedef struct ScalarOperationParameters{
    int miOperator;
    ScalarOperationParameters()
        : miOperator(SclOps::PLUS)
    {
    }
} ScalarOperationParameters;

class ScalarOperationModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    ScalarOperationModel();

    virtual
    ~ScalarOperationModel() override {}

    QJsonObject
    save() const override;

    void
    restore(const QJsonObject &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    QWidget *
    embeddedWidget() override { return nullptr; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private:
    ScalarOperationParameters mParams;
    std::shared_ptr<InformationData> mpInformationData { nullptr };
    std::shared_ptr<InformationData> mapInformationInData[2] {{ nullptr }};
    QPixmap _minPixmap;

    void processData(std::shared_ptr<InformationData> (&in)[2], std::shared_ptr< InformationData > & out,
                      const ScalarOperationParameters & params );

    template<typename T>
    void info_pointer_cast(const T result, std::shared_ptr<InformationData>& info)
    {
        const std::string type = typeid(result).name();
        if(type == "int")
        {
            auto d = std::make_shared<IntegerData>(result);
            info = std::static_pointer_cast<InformationData>(d);
        }
        else if(type == "float")
        {
            auto d = std::make_shared<FloatData>(result);
            info = std::static_pointer_cast<InformationData>(d);
        }
        else if(type == "double")
        {
            auto d = std::make_shared<DoubleData>(result);
            info = std::static_pointer_cast<InformationData>(d);
        }
        else if(type == "bool")
        {
            auto d = std::make_shared<BoolData>(result);
            info = std::static_pointer_cast<InformationData>(d);
        }
        return;
    }

};

#endif // SCALAROPERATIONMODEL_HPP
