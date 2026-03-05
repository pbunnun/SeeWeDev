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

#include "CVRTSPCameraEmbeddedWidget.hpp"
#include <QLineEdit>
#include <QPushButton>
#include "ui_CVRTSPCameraEmbeddedWidget.h"
#include <QDebug>

CVRTSPCameraEmbeddedWidget::CVRTSPCameraEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CVRTSPCameraEmbeddedWidget)
{
    ui->setupUi(this);
    connect(ui->mpStartButton, &QPushButton::clicked, this, &CVRTSPCameraEmbeddedWidget::start_button_clicked);
    connect(ui->mpStopButton, &QPushButton::clicked, this, &CVRTSPCameraEmbeddedWidget::stop_button_clicked);
    connect(ui->mpRTSPUrlLineEdit, &QLineEdit::editingFinished, this, &CVRTSPCameraEmbeddedWidget::rtsp_url_editing_finished);

    ui->mpRTSPUrlLineEdit->setStyleSheet("QLineEdit { background-color : yellow; }");

    // Start with empty URL
    ui->mpRTSPUrlLineEdit->clear();

    mpTransparentLabel = new QLabel( this );
    mpTransparentLabel->setGeometry(geometry());
    mpTransparentLabel->setAutoFillBackground( false );
}

CVRTSPCameraEmbeddedWidget::~CVRTSPCameraEmbeddedWidget()
{
    delete ui;
    delete mpTransparentLabel;
}

void
CVRTSPCameraEmbeddedWidget::
camera_status_changed( bool status )
{
    mCVRTSPCameraProperty.mbCameraStatus = status;
    if( mCVRTSPCameraProperty.mbCameraStatus )
        ui->mpRTSPUrlLineEdit->setStyleSheet( "QLineEdit { background-color : green; }" );
    else
        ui->mpRTSPUrlLineEdit->setStyleSheet( "QLineEdit { background-color : yellow; }" );
}

void
CVRTSPCameraEmbeddedWidget::
set_ready_state( bool bReady )
{
    ui->mpStartButton->setEnabled( bReady );
    ui->mpStopButton->setEnabled( !bReady );
}

void
CVRTSPCameraEmbeddedWidget::
set_camera_property(CVRTSPCameraProperty property)
{
    mCVRTSPCameraProperty = property;
    ui->mpRTSPUrlLineEdit->setText( mCVRTSPCameraProperty.msRTSPUrl );
}

void
CVRTSPCameraEmbeddedWidget::
start_button_clicked()
{
    ui->mpStopButton->setEnabled( true );
    ui->mpStartButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 0 );
}

void
CVRTSPCameraEmbeddedWidget::
stop_button_clicked()
{
    ui->mpStartButton->setEnabled( true );
    ui->mpStopButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 1 );
}

void
CVRTSPCameraEmbeddedWidget::
rtsp_url_editing_finished()
{
    QString newUrl = ui->mpRTSPUrlLineEdit->text();
    if( newUrl != mCVRTSPCameraProperty.msRTSPUrl )
    {
        mCVRTSPCameraProperty.msRTSPUrl = newUrl;
        ui->mpStartButton->setEnabled( true );
        ui->mpStopButton->setEnabled( false );
        Q_EMIT button_clicked_signal( 2 );
    }
}

void
CVRTSPCameraEmbeddedWidget::
set_active( bool active )
{
    if( active )
        mpTransparentLabel->hide();
    else
        mpTransparentLabel->show();
}
