#include "FloodFillEmbeddedWidget.hpp"
#include "ui_FloodFillEmbeddedWidget.h"

FloodFillEmbeddedWidget::FloodFillEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FloodFillEmbeddedWidget)
{
    ui->setupUi(this);
    ui->mpLowerBSpinbox->setRange( 0, 255 );
    ui->mpLowerGSpinbox->setRange( 0, 255 );
    ui->mpLowerRSpinbox->setRange( 0, 255 );
    ui->mpLowerGraySpinbox->setRange( 0, 255 );
    ui->mpUpperBSpinbox->setRange( 0, 255 );
    ui->mpUpperGSpinbox->setRange( 0, 255 );
    ui->mpUpperRSpinbox->setRange( 0, 255 );
    ui->mpUpperGraySpinbox->setRange( 0, 255 );
}

FloodFillEmbeddedWidget::~FloodFillEmbeddedWidget()
{
    delete ui;
}

void FloodFillEmbeddedWidget::on_mpLowerBSpinbox_valueChanged( int value )
{
    Q_EMIT spinbox_clicked_signal( 0, value );
}

void FloodFillEmbeddedWidget::on_mpLowerGSpinbox_valueChanged( int value )
{
    Q_EMIT spinbox_clicked_signal( 1, value );
}

void FloodFillEmbeddedWidget::on_mpLowerRSpinbox_valueChanged( int value )
{
    Q_EMIT spinbox_clicked_signal( 2, value );
}

void FloodFillEmbeddedWidget::on_mpLowerGraySpinbox_valueChanged( int value )
{
    Q_EMIT spinbox_clicked_signal( 3, value );
}

void FloodFillEmbeddedWidget::on_mpUpperBSpinbox_valueChanged( int value )
{
    Q_EMIT spinbox_clicked_signal( 4, value );
}

void FloodFillEmbeddedWidget::on_mpUpperGSpinbox_valueChanged( int value )
{
    Q_EMIT spinbox_clicked_signal( 5, value );
}

void FloodFillEmbeddedWidget::on_mpUpperRSpinbox_valueChanged( int value )
{
    Q_EMIT spinbox_clicked_signal( 6, value );
}

void FloodFillEmbeddedWidget::on_mpUpperGraySpinbox_valueChanged( int value )
{
    Q_EMIT spinbox_clicked_signal( 7, value );
}

void FloodFillEmbeddedWidget::set_maskStatus_label(const bool active)
{
    ui->mpMaskStatusLabel->setText(active? "Active" : "Inactive");
}

void FloodFillEmbeddedWidget::toggle_widgets(const int channels)
{
    bool isGray = channels == 1? true : false ;

    ui->mpLowerBLabel->setEnabled(!isGray);
    ui->mpLowerGLabel->setEnabled(!isGray);
    ui->mpLowerRLabel->setEnabled(!isGray);
    ui->mpUpperBLabel->setEnabled(!isGray);
    ui->mpUpperGLabel->setEnabled(!isGray);
    ui->mpUpperRLabel->setEnabled(!isGray);

    ui->mpLowerBSpinbox->setEnabled(!isGray);
    ui->mpLowerGSpinbox->setEnabled(!isGray);
    ui->mpLowerRSpinbox->setEnabled(!isGray);
    ui->mpUpperBSpinbox->setEnabled(!isGray);
    ui->mpUpperGSpinbox->setEnabled(!isGray);
    ui->mpUpperRSpinbox->setEnabled(!isGray);

    ui->mpLowerGrayLabel->setEnabled(isGray);
    ui->mpUpperGrayLabel->setEnabled(isGray);

    ui->mpLowerGraySpinbox->setEnabled(isGray);
    ui->mpUpperGraySpinbox->setEnabled(isGray);
}

void FloodFillEmbeddedWidget::set_lower_upper(const int (&lower)[4], const int (&upper)[4])
{
    ui->mpLowerBSpinbox->setValue( lower[0] );
    ui->mpLowerGSpinbox->setValue( lower[1] );
    ui->mpLowerRSpinbox->setValue( lower[2] );
    ui->mpLowerGraySpinbox->setValue( lower[3] );
    ui->mpUpperBSpinbox->setValue( upper[0] );
    ui->mpUpperGSpinbox->setValue( upper[1] );
    ui->mpUpperRSpinbox->setValue( upper[2] );
    ui->mpUpperGraySpinbox->setValue( upper[3] );
}
