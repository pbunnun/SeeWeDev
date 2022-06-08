#include "CVCameraEmbeddedWidget.hpp"
#include "ui_CVCameraEmbeddedWidget.h"
#include <QDebug>

CVCameraEmbeddedWidget::CVCameraEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CVCameraEmbeddedWidget)
{
    ui->setupUi(this);

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
    mParams.mbCameraStatus = status;
    if( mParams.mbCameraStatus )
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
set_params(CVCameraParameters params)
{
    mParams = params;
    ui->mpCameraIDComboBox->setCurrentText( QString::number( mParams.miCameraID ) );
}

void
CVCameraEmbeddedWidget::
on_mpStartButton_clicked()
{
    ui->mpStopButton->setEnabled( true );
    ui->mpStartButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 0 );
}

void
CVCameraEmbeddedWidget::
on_mpStopButton_clicked()
{
    ui->mpStartButton->setEnabled( true );
    ui->mpStopButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 1 );
}

void
CVCameraEmbeddedWidget::
on_mpCameraIDComboBox_currentIndexChanged( int )
{
    mParams.miCameraID = ui->mpCameraIDComboBox->currentText().toInt();
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
