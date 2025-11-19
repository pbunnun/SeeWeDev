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

#include "CVCameraCalibrationEmbeddedWidget.hpp"
#include "ui_CVCameraCalibrationEmbeddedWidget.h"
#include <QDebug>

CVCameraCalibrationEmbeddedWidget::CVCameraCalibrationEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CVCameraCalibrationEmbeddedWidget)
{
    ui->setupUi(this);
    connect( ui->mpBackwardButton, &QPushButton::clicked, this, &CVCameraCalibrationEmbeddedWidget::backward_button_clicked );
    connect( ui->mpForwardButton, &QPushButton::clicked, this, &CVCameraCalibrationEmbeddedWidget::forward_button_clicked );
    connect( ui->mpCalibrateButton, &QPushButton::clicked, this, &CVCameraCalibrationEmbeddedWidget::calibrate_button_clicked );
    connect( ui->mpExportButton, &QPushButton::clicked, this, &CVCameraCalibrationEmbeddedWidget::export_button_clicked );
    connect( ui->mpCaptureButton, &QPushButton::clicked, this, &CVCameraCalibrationEmbeddedWidget::capture_button_clicked );
    connect( ui->mpRemoveButton, &QPushButton::clicked, this, &CVCameraCalibrationEmbeddedWidget::remove_button_clicked );
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0) )
    connect( ui->mpAutoCapCheckBox, &QCheckBox::stateChanged, this, &CVCameraCalibrationEmbeddedWidget::auto_checkbox_stateChanged );
#else
    connect( ui->mpAutoCapCheckBox, &QCheckBox::checkStateChanged, this, &CVCameraCalibrationEmbeddedWidget::auto_checkbox_stateChanged );
#endif
}

CVCameraCalibrationEmbeddedWidget::~CVCameraCalibrationEmbeddedWidget()
{
    delete ui;
}

void
CVCameraCalibrationEmbeddedWidget::
backward_button_clicked()
{
    Q_EMIT button_clicked_signal( 1 );
}


void
CVCameraCalibrationEmbeddedWidget::
forward_button_clicked()
{
    Q_EMIT button_clicked_signal( 0 );
}

void
CVCameraCalibrationEmbeddedWidget::
export_button_clicked()
{
    Q_EMIT button_clicked_signal( 2 );
}

void
CVCameraCalibrationEmbeddedWidget::
calibrate_button_clicked()
{
    Q_EMIT button_clicked_signal( 4 );
}

void
CVCameraCalibrationEmbeddedWidget::
capture_button_clicked()
{
    Q_EMIT button_clicked_signal( 3 );
}

void
CVCameraCalibrationEmbeddedWidget::
auto_checkbox_stateChanged(int state)
{
    Q_EMIT button_clicked_signal(10 + state);
}

void
CVCameraCalibrationEmbeddedWidget::
remove_button_clicked()
{
    Q_EMIT button_clicked_signal( 5 );
}

void
CVCameraCalibrationEmbeddedWidget::
set_active( bool )
{
/*
    ui->mpPlayPauseButton->setChecked( false );

    if( active )
    { 
        ui->mpPlayPauseButton->setEnabled( true );
        ui->mpForwardButton->setEnabled( true );
        ui->mpCaptureButton->setEnabled( true );
    }
    else
    {
        ui->mpPlayPauseButton->setEnabled( false );
        ui->mpForwardButton->setEnabled( false );
        ui->mpCaptureButton->setEnabled( false );
    }
*/
}

void
CVCameraCalibrationEmbeddedWidget::
set_auto_capture_flag(bool flag)
{
    ui->mpAutoCapCheckBox->setChecked( flag );
}

void
CVCameraCalibrationEmbeddedWidget::
update_total_images(int total)
{
    ui->mpTotalImageLCD->display(total);
}

void
CVCameraCalibrationEmbeddedWidget::
set_image_number(int no)
{
    ui->mpNoImageLCD->display(no);
}
