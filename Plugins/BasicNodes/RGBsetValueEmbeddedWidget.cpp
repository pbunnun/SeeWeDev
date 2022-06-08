#include "RGBsetValueEmbeddedWidget.hpp"
#include "ui_RGBsetValueEmbeddedWidget.h"

RGBsetValueEmbeddedWidget::RGBsetValueEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RGBsetValueEmbeddedWidget)
{
    ui->setupUi(this);
}

RGBsetValueEmbeddedWidget::~RGBsetValueEmbeddedWidget()
{
    delete ui;
}

void RGBsetValueEmbeddedWidget::on_mpResetButton_clicked()
{
    Q_EMIT button_clicked_signal( 0 );
}
