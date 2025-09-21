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

#include "CVImageLoaderEmbeddedWidget.hpp"
#include "ui_CVImageLoaderEmbeddedWidget.h"
#include <QDebug>

CVImageLoaderEmbeddedWidget::CVImageLoaderEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::CVImageLoaderEmbeddedWidget)
{
    ui->setupUi(this);
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
on_mpBackwardButton_clicked()
{
    Q_EMIT button_clicked_signal( 0 );
}


void
CVImageLoaderEmbeddedWidget::
on_mpOpenButton_clicked()
{
    Q_EMIT button_clicked_signal( 1 );
}

void
CVImageLoaderEmbeddedWidget::
on_mpPlayPauseButton_clicked()
{
    if( ui->mpPlayPauseButton->isChecked() )
        Q_EMIT button_clicked_signal( 2 );
    else
        Q_EMIT button_clicked_signal( 3 );
}

void
CVImageLoaderEmbeddedWidget::
on_mpForwardButton_clicked()
{
    Q_EMIT button_clicked_signal( 4 );
}

void
CVImageLoaderEmbeddedWidget::
on_mpFilenameButton_clicked()
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
