#include "BitwiseOperationEmbeddedWidget.hpp"
#include "ui_BitwiseOperationEmbeddedWidget.h"

BitwiseOperationEmbeddedWidget::BitwiseOperationEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BitwiseOperationEmbeddedWidget)
{
    ui->setupUi(this);
}

BitwiseOperationEmbeddedWidget::~BitwiseOperationEmbeddedWidget()
{
    delete ui;
}

void BitwiseOperationEmbeddedWidget::set_maskStatus_label(bool active)
{
    ui->mpMaskStatusLabel->setText(active? "Active" : "Inactive");
}
