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

#include "CVUSBCameraEmbeddedWidget.hpp"
#include <QComboBox>
#include <QPushButton>
#include "ui_CVUSBCameraEmbeddedWidget.h"
#include <QDebug>

CVUSBCameraEmbeddedWidget::CVUSBCameraEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CVUSBCameraEmbeddedWidget)
{
    ui->setupUi(this);
    connect(ui->mpStartButton, &QPushButton::clicked, this, &CVUSBCameraEmbeddedWidget::start_button_clicked);
    connect(ui->mpStopButton, &QPushButton::clicked, this, &CVUSBCameraEmbeddedWidget::stop_button_clicked);
    connect(ui->mpRefreshButton, &QPushButton::clicked, this, &CVUSBCameraEmbeddedWidget::refresh_button_clicked);
    connect(ui->mpCameraIDComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CVUSBCameraEmbeddedWidget::camera_id_combo_box_current_index_changed);


    ui->mpCameraIDComboBox->setStyleSheet("QComboBox { background-color : yellow; }");

    // Start with an empty list; CVUSBCameraModel will populate actual IDs
    ui->mpCameraIDComboBox->clear();

    mpTransparentLabel = new QLabel( this );
    mpTransparentLabel->setGeometry(geometry());
    mpTransparentLabel->setAutoFillBackground( false );
}

CVUSBCameraEmbeddedWidget::~CVUSBCameraEmbeddedWidget()
{
    delete ui;
    delete mpTransparentLabel;
}

void
CVUSBCameraEmbeddedWidget::
camera_status_changed( bool status )
{
    mCVUSBCameraProperty.mbCameraStatus = status;
    if( mCVUSBCameraProperty.mbCameraStatus )
        ui->mpCameraIDComboBox->setStyleSheet( "QComboBox { background-color : green; }" );
    else
        ui->mpCameraIDComboBox->setStyleSheet( "QComboBox { background-color : yellow; }" );
}

void
CVUSBCameraEmbeddedWidget::
set_ready_state( bool bReady )
{
    ui->mpStartButton->setEnabled( bReady );
    ui->mpStopButton->setEnabled( !bReady );
}

void
CVUSBCameraEmbeddedWidget::
set_camera_property(CVUSBCameraProperty property)
{
    mCVUSBCameraProperty = property;
    ui->mpCameraIDComboBox->setCurrentText( QString::number( mCVUSBCameraProperty.miCameraID ) );
}

void
CVUSBCameraEmbeddedWidget::
set_camera_id_options( const QStringList &cameraIds, int currentId )
{
    ui->mpCameraIDComboBox->blockSignals( true );
    ui->mpCameraIDComboBox->clear();
    for( const auto &id : cameraIds )
    {
        ui->mpCameraIDComboBox->addItem( id );
    }

    int index = ui->mpCameraIDComboBox->findText( QString::number( currentId ) );
    if( index == -1 && ui->mpCameraIDComboBox->count() > 0 )
        index = 0;
    if( index >= 0 )
        ui->mpCameraIDComboBox->setCurrentIndex( index );
    ui->mpCameraIDComboBox->blockSignals( false );

    mCVUSBCameraProperty.miCameraID = ui->mpCameraIDComboBox->currentText().toInt();
}

void
CVUSBCameraEmbeddedWidget::
refresh_button_clicked()
{
    Q_EMIT button_clicked_signal( 3 );
}

void
CVUSBCameraEmbeddedWidget::
start_button_clicked()
{
    ui->mpStopButton->setEnabled( true );
    ui->mpStartButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 0 );
}

void
CVUSBCameraEmbeddedWidget::
stop_button_clicked()
{
    ui->mpStartButton->setEnabled( true );
    ui->mpStopButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 1 );
}

void
CVUSBCameraEmbeddedWidget::
camera_id_combo_box_current_index_changed( int )
{
    mCVUSBCameraProperty.miCameraID = ui->mpCameraIDComboBox->currentText().toInt();
    ui->mpStartButton->setEnabled( true );
    ui->mpStopButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 2 );
}

void
CVUSBCameraEmbeddedWidget::
set_active( bool active )
{
    if( active )
        mpTransparentLabel->hide();
    else
        mpTransparentLabel->show();
}
