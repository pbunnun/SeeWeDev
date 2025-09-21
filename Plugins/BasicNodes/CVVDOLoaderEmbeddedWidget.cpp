//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "CVVDOLoaderEmbeddedWidget.hpp"
#include "ui_CVVDOLoaderEmbeddedWidget.h"
#include <QDebug>

CVVDOLoaderEmbeddedWidget::CVVDOLoaderEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CVVDOLoaderEmbeddedWidget)
{
    ui->setupUi(this);
}

CVVDOLoaderEmbeddedWidget::~CVVDOLoaderEmbeddedWidget()
{
    delete ui;
}

void
CVVDOLoaderEmbeddedWidget::
set_filename( QString filename )
{
    ui->mpFilenameButton->setText(filename);
}

void
CVVDOLoaderEmbeddedWidget::
on_mpBackwardButton_clicked()
{
    Q_EMIT button_clicked_signal( 0 );
}

void
CVVDOLoaderEmbeddedWidget::
on_mpPlayPauseButton_clicked()
{
    if( ui->mpPlayPauseButton->isChecked() )
    {
        ui->mpSlider->blockSignals(true);
        ui->mpFrameNumberSpinbox->blockSignals(true);
        Q_EMIT button_clicked_signal( 1 );
    }
    else
    {
        ui->mpSlider->blockSignals(false);
        ui->mpFrameNumberSpinbox->blockSignals(true);
        Q_EMIT button_clicked_signal( 2 );
    }
}

void
CVVDOLoaderEmbeddedWidget::
pause_video()
{
    ui->mpPlayPauseButton->setChecked(false);
    on_mpPlayPauseButton_clicked();
}

void
CVVDOLoaderEmbeddedWidget::
on_mpForwardButton_clicked()
{
    Q_EMIT button_clicked_signal( 3 );
}

void
CVVDOLoaderEmbeddedWidget::
on_mpFilenameButton_clicked()
{
    Q_EMIT button_clicked_signal( 4 );
}

void
CVVDOLoaderEmbeddedWidget::
set_active( bool active )
{
    ui->mpPlayPauseButton->setChecked( false );

    if( active )
    { 
        ui->mpPlayPauseButton->setEnabled( true );
        ui->mpForwardButton->setEnabled( true );
        ui->mpBackwardButton->setEnabled( true );
    }
    else
    {
        ui->mpPlayPauseButton->setEnabled( false );
        ui->mpForwardButton->setEnabled( false );
        ui->mpBackwardButton->setEnabled( false );
    }
}

void
CVVDOLoaderEmbeddedWidget::
set_flip_pause( bool pause )
{
    ui->mpPlayPauseButton->setChecked( pause );
}

void
CVVDOLoaderEmbeddedWidget::
set_maximum_slider( int max )
{
    ui->mpSlider->setMaximum( max );
    ui->mpFrameNumberSpinbox->setMaximum( max );
}

void
CVVDOLoaderEmbeddedWidget::
on_mpSlider_valueChanged( int value )
{
    Q_EMIT slider_value_signal( value );
    ui->mpFrameNumberSpinbox->blockSignals( true );
    ui->mpFrameNumberSpinbox->setValue( value );
    ui->mpFrameNumberSpinbox->blockSignals( false );
}

void
CVVDOLoaderEmbeddedWidget::
on_mpFrameNumberSpinbox_valueChanged( int value )
{
    on_mpSlider_valueChanged( value );
}

void
CVVDOLoaderEmbeddedWidget::
set_slider_value(int value )
{
    ui->mpSlider->setValue( value );
    ui->mpFrameNumberSpinbox->setValue( value );
}
