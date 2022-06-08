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
    ui->mpFilenameButton->setText(filename);
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
CVImageLoaderEmbeddedWidget::
set_flip_pause( bool pause )
{
    ui->mpPlayPauseButton->setChecked( pause );
}
