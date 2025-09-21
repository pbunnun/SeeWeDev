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

#include "MathConditionEmbeddedWidget.hpp"
#include "ui_MathConditionEmbeddedWidget.h"
#include <QDebug>

MathConditionEmbeddedWidget::MathConditionEmbeddedWidget( QWidget *parent )
    : QWidget( parent ),
      ui( new Ui::MathConditionEmbeddedWidget )
{
    ui->setupUi( this );
    ui->mpConditionNumber->setValidator( new QDoubleValidator(this) );
}

MathConditionEmbeddedWidget::~MathConditionEmbeddedWidget()
{
    delete ui;
}

void
MathConditionEmbeddedWidget::
on_mpConditionNumber_textChanged( const QString & text )
{
    Q_EMIT condition_changed_signal( ui->mpConditionComboBox->currentIndex(), text );
}

void
MathConditionEmbeddedWidget::
on_mpConditionComboBox_currentIndexChanged( int idx )
{
    Q_EMIT condition_changed_signal( idx, ui->mpConditionNumber->text() );
}

QStringList
MathConditionEmbeddedWidget::
get_condition_string_list()
{
    QStringList string_list;
    for( int count = 0; count < ui->mpConditionComboBox->count(); ++count )
        string_list.append( ui->mpConditionComboBox->itemText( count ) );
    return string_list;
}

QString
MathConditionEmbeddedWidget::
get_condition_number( )
{
    return ui->mpConditionNumber->text();
}

void
MathConditionEmbeddedWidget::
set_condition_number( QString number )
{
    ui->mpConditionNumber->setText( number );
}

int
MathConditionEmbeddedWidget::
get_condition_text_index( )
{
    return ui->mpConditionComboBox->currentIndex();
}

void
MathConditionEmbeddedWidget::
set_condition_text_index( int idx )
{
    ui->mpConditionComboBox->setCurrentIndex( idx );
}
