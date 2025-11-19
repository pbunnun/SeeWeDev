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

#include "CVVideoLoaderEmbeddedWidget.hpp"
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include "ui_CVVideoLoaderEmbeddedWidget.h"
#include <QDebug>
#include <QEvent>

CVVideoLoaderEmbeddedWidget::CVVideoLoaderEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CVVideoLoaderEmbeddedWidget)
{
    ui->setupUi(this);
    connect(ui->mpForwardButton, &QPushButton::clicked, this, &CVVideoLoaderEmbeddedWidget::forward_button_clicked);
    connect(ui->mpBackwardButton, &QPushButton::clicked, this, &CVVideoLoaderEmbeddedWidget::backward_button_clicked);
    connect(ui->mpPlayPauseButton, &QPushButton::clicked, this, &CVVideoLoaderEmbeddedWidget::play_pause_button_clicked);
    connect(ui->mpFilenameButton, &QPushButton::clicked, this, &CVVideoLoaderEmbeddedWidget::filename_button_clicked);
    connect(ui->mpSlider, &QSlider::valueChanged, this, &CVVideoLoaderEmbeddedWidget::slider_value_changed);
    connect(ui->mpFrameNumberSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CVVideoLoaderEmbeddedWidget::frame_number_spinbox_value_changed);

    ui->mpFrameNumberSpinbox->installEventFilter(this);
}

CVVideoLoaderEmbeddedWidget::~CVVideoLoaderEmbeddedWidget()
{
    delete ui;
}

void
CVVideoLoaderEmbeddedWidget::
set_filename( QString filename )
{
    ui->mpFilenameButton->setText(filename);
}

void
CVVideoLoaderEmbeddedWidget::
backward_button_clicked()
{
    Q_EMIT button_clicked_signal( 0 );
}

void
CVVideoLoaderEmbeddedWidget::
play_pause_button_clicked()
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
CVVideoLoaderEmbeddedWidget::
pause_video()
{
    ui->mpPlayPauseButton->blockSignals(true);
    ui->mpPlayPauseButton->setChecked(false);
    ui->mpPlayPauseButton->blockSignals(false);

    ui->mpSlider->blockSignals(false);
    ui->mpFrameNumberSpinbox->blockSignals(true);
}

void
CVVideoLoaderEmbeddedWidget::
forward_button_clicked()
{
    Q_EMIT button_clicked_signal( 3 );
}

void
CVVideoLoaderEmbeddedWidget::
filename_button_clicked()
{
    Q_EMIT button_clicked_signal( 4 );
}

void
CVVideoLoaderEmbeddedWidget::
set_flip_pause( bool pause )
{
    ui->mpPlayPauseButton->blockSignals(true);
    ui->mpPlayPauseButton->setChecked( pause );
    ui->mpPlayPauseButton->blockSignals(false);
}

void
CVVideoLoaderEmbeddedWidget::
set_maximum_slider( int max )
{
    ui->mpSlider->setMaximum( max );
    ui->mpFrameNumberSpinbox->setMaximum( max );
}

void
CVVideoLoaderEmbeddedWidget::
slider_value_changed( int value )
{
    ui->mpFrameNumberSpinbox->blockSignals( true );
    ui->mpFrameNumberSpinbox->setValue( value );
    ui->mpFrameNumberSpinbox->blockSignals( false );
    Q_EMIT slider_value_signal( value );
}

void
CVVideoLoaderEmbeddedWidget::
frame_number_spinbox_value_changed( int value )
{
    ui->mpSlider->blockSignals( true );
    ui->mpSlider->setValue( value );
    ui->mpSlider->blockSignals( false );
    Q_EMIT slider_value_signal( value );
}

void
CVVideoLoaderEmbeddedWidget::
set_slider_value(int value )
{
    ui->mpSlider->setValue( value );
    ui->mpFrameNumberSpinbox->setValue( value );
}

void
CVVideoLoaderEmbeddedWidget::
set_toggle_play( bool play )
{
    ui->mpPlayPauseButton->blockSignals(true);
    ui->mpPlayPauseButton->setChecked( play );
    ui->mpPlayPauseButton->blockSignals(false);
}

bool
CVVideoLoaderEmbeddedWidget::
eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->mpFrameNumberSpinbox && event->type() == QEvent::FocusIn)
    {
        Q_EMIT button_clicked_signal( -1 );
    }
    return QWidget::eventFilter(obj, event);
}
void
CVVideoLoaderEmbeddedWidget::
resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    Q_EMIT widget_resized_signal();
}
