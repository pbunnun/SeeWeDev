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

#include "CVImageLoaderEmbeddedWidget.hpp"
#include "ui_CVImageLoaderEmbeddedWidget.h"
#include <QDebug>
#include <QPushButton>

CVImageLoaderEmbeddedWidget::CVImageLoaderEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::CVImageLoaderEmbeddedWidget)
{
    ui->setupUi(this);
    
    connect(ui->mpForwardButton, &QPushButton::clicked, this, &CVImageLoaderEmbeddedWidget::forward_button_clicked);
    connect(ui->mpBackwardButton, &QPushButton::clicked, this, &CVImageLoaderEmbeddedWidget::backward_button_clicked);
    connect(ui->mpPlayPauseButton, &QPushButton::clicked, this, &CVImageLoaderEmbeddedWidget::play_pause_button_clicked);
    connect(ui->mpOpenButton, &QPushButton::clicked, this, &CVImageLoaderEmbeddedWidget::open_button_clicked);
    connect(ui->mpFilenameButton, &QPushButton::clicked, this, &CVImageLoaderEmbeddedWidget::filename_button_clicked);
}

CVImageLoaderEmbeddedWidget::~CVImageLoaderEmbeddedWidget()
{
    delete ui;
}

void
CVImageLoaderEmbeddedWidget::
set_filename( QString filename )
{
    QString text = filename;//.left(15);
    ui->mpFilenameButton->setText(text);
}

void
CVImageLoaderEmbeddedWidget::
backward_button_clicked()
{
    Q_EMIT button_clicked_signal( 0 );
}


void
CVImageLoaderEmbeddedWidget::
open_button_clicked()
{
    Q_EMIT button_clicked_signal( 1 );
}

void
CVImageLoaderEmbeddedWidget::
play_pause_button_clicked()
{
    if( ui->mpPlayPauseButton->isChecked() )
        Q_EMIT button_clicked_signal( 2 );
    else
        Q_EMIT button_clicked_signal( 3 );
}

void
CVImageLoaderEmbeddedWidget::
forward_button_clicked()
{
    Q_EMIT button_clicked_signal( 4 );
}

void
CVImageLoaderEmbeddedWidget::
filename_button_clicked()
{
    Q_EMIT button_clicked_signal( 5 );
}

void
CVImageLoaderEmbeddedWidget::
set_active( bool active )
{
    ui->mpPlayPauseButton->setChecked( false );

    if( active )
    { 
        ui->mpPlayPauseButton->setEnabled( true );
        ui->mpForwardButton->setEnabled( true );
    }
    else
    {
        ui->mpPlayPauseButton->setEnabled( false );
        ui->mpForwardButton->setEnabled( false );
    }
}

void
CVImageLoaderEmbeddedWidget::
set_flip_pause( bool pause )
{
    ui->mpPlayPauseButton->setChecked( pause );
}

void
CVImageLoaderEmbeddedWidget::
revert_play_pause_state()
{
    // Toggle the button state back to what it was before the click
    ui->mpPlayPauseButton->setChecked( !ui->mpPlayPauseButton->isChecked() );
}

void
CVImageLoaderEmbeddedWidget::
resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    Q_EMIT widget_resized_signal();
}
