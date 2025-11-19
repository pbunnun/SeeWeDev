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

#include "CVCameraEmbeddedWidget.hpp"
#include <QComboBox>
#include <QPushButton>
#include "ui_CVCameraEmbeddedWidget.h"
#include <QDebug>

CVCameraEmbeddedWidget::CVCameraEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CVCameraEmbeddedWidget)
{
    ui->setupUi(this);
    connect(ui->mpStartButton, &QPushButton::clicked, this, &CVCameraEmbeddedWidget::start_button_clicked);
    connect(ui->mpStopButton, &QPushButton::clicked, this, &CVCameraEmbeddedWidget::stop_button_clicked);
    connect(ui->mpCameraIDComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CVCameraEmbeddedWidget::camera_id_combo_box_current_index_changed);


    ui->mpCameraIDComboBox->setStyleSheet("QComboBox { background-color : yellow; }");

    mpTransparentLabel = new QLabel( this );
    mpTransparentLabel->setGeometry(geometry());
    mpTransparentLabel->setAutoFillBackground( false );
}

CVCameraEmbeddedWidget::~CVCameraEmbeddedWidget()
{
    delete ui;
    delete mpTransparentLabel;
}

void
CVCameraEmbeddedWidget::
camera_status_changed( bool status )
{
    mCameraProperty.mbCameraStatus = status;
    if( mCameraProperty.mbCameraStatus )
        ui->mpCameraIDComboBox->setStyleSheet( "QComboBox { background-color : green; }" );
    else
        ui->mpCameraIDComboBox->setStyleSheet( "QComboBox { background-color : yellow; }" );
}

void
CVCameraEmbeddedWidget::
set_ready_state( bool bReady )
{
    ui->mpStartButton->setEnabled( bReady );
    ui->mpStopButton->setEnabled( !bReady );
}

void
CVCameraEmbeddedWidget::
set_camera_property(CVCameraProperty property)
{
    mCameraProperty = property;
    ui->mpCameraIDComboBox->setCurrentText( QString::number( mCameraProperty.miCameraID ) );
}

void
CVCameraEmbeddedWidget::
start_button_clicked()
{
    ui->mpStopButton->setEnabled( true );
    ui->mpStartButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 0 );
}

void
CVCameraEmbeddedWidget::
stop_button_clicked()
{
    ui->mpStartButton->setEnabled( true );
    ui->mpStopButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 1 );
}

void
CVCameraEmbeddedWidget::
camera_id_combo_box_current_index_changed( int )
{
    mCameraProperty.miCameraID = ui->mpCameraIDComboBox->currentText().toInt();
    ui->mpStartButton->setEnabled( true );
    ui->mpStopButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 2 );
}

void
CVCameraEmbeddedWidget::
set_active( bool active )
{
    if( active )
        mpTransparentLabel->hide();
    else
        mpTransparentLabel->show();
}
