//Copyright © 2025, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "CVFloodFillEmbeddedWidget.hpp"
#include <QSpinBox>
#include "ui_CVFloodFillEmbeddedWidget.h"

CVFloodFillEmbeddedWidget::CVFloodFillEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CVFloodFillEmbeddedWidget)
{
    ui->setupUi(this);
    connect(ui->mpLowerBSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVFloodFillEmbeddedWidget::lower_b_spinbox_value_changed);
    connect(ui->mpLowerGSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVFloodFillEmbeddedWidget::lower_g_spinbox_value_changed);
    connect(ui->mpLowerRSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVFloodFillEmbeddedWidget::lower_r_spinbox_value_changed);
    connect(ui->mpLowerGraySpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVFloodFillEmbeddedWidget::lower_gray_spinbox_value_changed);
    connect(ui->mpUpperBSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVFloodFillEmbeddedWidget::upper_b_spinbox_value_changed);
    connect(ui->mpUpperGSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVFloodFillEmbeddedWidget::upper_g_spinbox_value_changed);
    connect(ui->mpUpperRSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVFloodFillEmbeddedWidget::upper_r_spinbox_value_changed);
    connect(ui->mpUpperGraySpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVFloodFillEmbeddedWidget::upper_gray_spinbox_value_changed);

    ui->mpLowerBSpinbox->setRange( 0, 255 );
    ui->mpLowerGSpinbox->setRange( 0, 255 );
    ui->mpLowerRSpinbox->setRange( 0, 255 );
    ui->mpLowerGraySpinbox->setRange( 0, 255 );
    ui->mpUpperBSpinbox->setRange( 0, 255 );
    ui->mpUpperGSpinbox->setRange( 0, 255 );
    ui->mpUpperRSpinbox->setRange( 0, 255 );
    ui->mpUpperGraySpinbox->setRange( 0, 255 );
}

CVFloodFillEmbeddedWidget::~CVFloodFillEmbeddedWidget()
{
    delete ui;
}

void CVFloodFillEmbeddedWidget::lower_b_spinbox_value_changed( int value )
{
    Q_EMIT spinbox_clicked_signal( 0, value );
}

void CVFloodFillEmbeddedWidget::lower_g_spinbox_value_changed( int value )
{
    Q_EMIT spinbox_clicked_signal( 1, value );
}

void CVFloodFillEmbeddedWidget::lower_r_spinbox_value_changed( int value )
{
    Q_EMIT spinbox_clicked_signal( 2, value );
}

void CVFloodFillEmbeddedWidget::lower_gray_spinbox_value_changed( int value )
{
    Q_EMIT spinbox_clicked_signal( 3, value );
}

void CVFloodFillEmbeddedWidget::upper_b_spinbox_value_changed( int value )
{
    Q_EMIT spinbox_clicked_signal( 4, value );
}

void CVFloodFillEmbeddedWidget::upper_g_spinbox_value_changed( int value )
{
    Q_EMIT spinbox_clicked_signal( 5, value );
}

void CVFloodFillEmbeddedWidget::upper_r_spinbox_value_changed( int value )
{
    Q_EMIT spinbox_clicked_signal( 6, value );
}

void CVFloodFillEmbeddedWidget::upper_gray_spinbox_value_changed( int value )
{
    Q_EMIT spinbox_clicked_signal( 7, value );
}

void CVFloodFillEmbeddedWidget::set_maskStatus_label(const bool active)
{
    ui->mpMaskStatusLabel->setText(active? "Active" : "Inactive");
}

void CVFloodFillEmbeddedWidget::toggle_widgets(const int channels)
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

void CVFloodFillEmbeddedWidget::set_lower_upper(const int (&lower)[4], const int (&upper)[4])
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
