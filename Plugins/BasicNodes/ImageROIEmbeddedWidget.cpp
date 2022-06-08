#include "ImageROIEmbeddedWidget.hpp"
#include "ui_ImageROIEmbeddedWidget.h"

ImageROIEmbeddedWidget::ImageROIEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageROIEmbeddedWidget)
{
    ui->setupUi(this);
    ui->mpApplyButton->setEnabled(false);
    ui->mpResetButton->setEnabled(false);
}

ImageROIEmbeddedWidget::~ImageROIEmbeddedWidget()
{
    delete ui;
}

void ImageROIEmbeddedWidget::on_mpApplyButton_clicked()
{
    Q_EMIT button_clicked_signal(1);
}

void ImageROIEmbeddedWidget::on_mpResetButton_clicked()
{
    Q_EMIT button_clicked_signal(0);
}

void ImageROIEmbeddedWidget::enable_apply_button(const bool enable)
{
    ui->mpApplyButton->setEnabled(enable);
}

void ImageROIEmbeddedWidget::enable_reset_button(const bool enable)
{
    ui->mpResetButton->setEnabled(enable);
}
