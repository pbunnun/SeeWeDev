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
        Q_EMIT button_clicked_signal( 1 );
    }
    else
    {
        ui->mpSlider->blockSignals(false);
        Q_EMIT button_clicked_signal( 2 );
    }
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
}

void
CVVDOLoaderEmbeddedWidget::
on_mpSlider_valueChanged( int value )
{
    Q_EMIT slider_value_signal( value );
}

void
CVVDOLoaderEmbeddedWidget::
set_slider_value(int value )
{
    ui->mpSlider->setValue( value );
}
