#ifndef TEMPLATEMODEL_HPP
#define TEMPLATEMODEL_HPP

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include <nodes/DataModelRegistry>
#include "PBNodeDataModel.hpp"
#include "CVImageData.hpp"
#include "TemplateEmbeddedWidget.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

class TemplateModel : public PBNodeDataModel
{
    Q_OBJECT

public:
    TemplateModel();

    virtual
    ~TemplateModel() override {}

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /*
     * Recieve signals back from QtPropertyBrowser and use this function to
     * set parameters/variables accordingly.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /*
     * This function will be called automatically after this model is created.
     */
    void
    late_constructor() override;

    /*
     * These two static members must be defined for every models. _category can be duplicate with existing categories.
     * However, _model_name has to be a unique name.
     */
    static const QString _category;

    static const QString _model_name;

private Q_SLOTS:
    
    void
    em_button_clicked( int button );

    void
    enable_changed(bool) override;

private:
    TemplateEmbeddedWidget * mpEmbeddedWidget;

    std::shared_ptr<CVImageData> mpCVImageData;

    bool mbCheckBox{ true };
    QString msDisplayText{ "ComboBox" };
    QSize mSize { QSize( 1, 1 ) };
    QPoint mPoint { QPoint( 7, 7 ) };
};

#endif
