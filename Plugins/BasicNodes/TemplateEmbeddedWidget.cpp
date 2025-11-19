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

#include "TemplateEmbeddedWidget.hpp"
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include "ui_TemplateEmbeddedWidget.h"
#include <QDebug>

TemplateEmbeddedWidget::TemplateEmbeddedWidget( QWidget *parent )
    : QWidget( parent ),
      ui( new Ui::TemplateEmbeddedWidget )
{
    ui->setupUi( this );
    setMinimumSize(158, 122); // Prevent resize smaller than default
    connect(ui->mpStartButton, &QPushButton::clicked, this, &TemplateEmbeddedWidget::start_button_clicked);
    connect(ui->mpStopButton, &QPushButton::clicked, this, &TemplateEmbeddedWidget::stop_button_clicked);
    connect(ui->mpSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TemplateEmbeddedWidget::spin_box_value_changed);
    connect(ui->mpComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TemplateEmbeddedWidget::combo_box_current_index_changed);
    connect(ui->mpSendButton, &QPushButton::clicked, this, &TemplateEmbeddedWidget::send_button_clicked);

}

TemplateEmbeddedWidget::~TemplateEmbeddedWidget()
{
    delete ui;
}

void
TemplateEmbeddedWidget::
start_button_clicked()
{
    ui->mpStopButton->setEnabled( true );
    ui->mpStartButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 0 );
}

void
TemplateEmbeddedWidget::
stop_button_clicked()
{
    ui->mpStartButton->setEnabled( true );
    ui->mpStopButton->setEnabled( false );
    Q_EMIT button_clicked_signal( 1 );
}

void
TemplateEmbeddedWidget::
spin_box_value_changed( int value )
{
    qDebug() << "SpinBox : changed value to " << value;
    Q_EMIT button_clicked_signal( 2 );
}

void
TemplateEmbeddedWidget::
combo_box_current_index_changed( int idx )
{
    qDebug() << "ComboBox : current index is " << idx;
    Q_EMIT button_clicked_signal( 3 );
}

void
TemplateEmbeddedWidget::
send_button_clicked()
{
    Q_EMIT button_clicked_signal( 4 );
}

QStringList
TemplateEmbeddedWidget::
get_combobox_string_list()
{
    QStringList string_list;
    for( int count = 0; count < ui->mpComboBox->count(); ++count )
        string_list.append( ui->mpComboBox->itemText( count ) );
    return string_list;
}

const QSpinBox *
TemplateEmbeddedWidget::
get_spinbox()
{
    return ui->mpSpinBox;
}

void
TemplateEmbeddedWidget::
set_combobox_value( QString value )
{
    ui->mpComboBox->setCurrentText( value );
}

void
TemplateEmbeddedWidget::
set_spinbox_value( int value )
{
    ui->mpSpinBox->setValue( value );
}

void
TemplateEmbeddedWidget::
set_active_button( bool startButtonActive )
{
    //If a button is active, it will not be able to click!.
    ui->mpStartButton->setEnabled( !startButtonActive );
    ui->mpStopButton->setEnabled( startButtonActive );
}

QString
TemplateEmbeddedWidget::
get_combobox_text( )
{
    return ui->mpComboBox->currentText();
}

void
TemplateEmbeddedWidget::
set_display_text( QString value )
{
    ui->mpDisplayText->setText( value );
}

void
TemplateEmbeddedWidget::
resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    Q_EMIT widget_resized_signal();
}
