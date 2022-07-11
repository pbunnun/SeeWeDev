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

#include "FaceDetectionEmbeddedWidget.hpp"
#include "ui_FaceDetectionEmbeddedWidget.h"
#include <QDebug>

FaceDetectionEmbeddedWidget::FaceDetectionEmbeddedWidget( QWidget *parent )
    : QWidget( parent ),
      ui( new Ui::FaceDetectionEmbeddedWidget )
{
    ui->setupUi( this );
}

FaceDetectionEmbeddedWidget::~FaceDetectionEmbeddedWidget() {
    delete ui;
}

void FaceDetectionEmbeddedWidget::on_mpComboBox_currentIndexChanged( int idx ) {

    qDebug() << "ComboBox : current index is " << idx;
    Q_EMIT button_clicked_signal( 3 );
}

QStringList FaceDetectionEmbeddedWidget::get_combobox_string_list() {
    QStringList string_list;
    for( int count = 0; count < ui->mpComboBox->count(); ++count )
        string_list.append( ui->mpComboBox->itemText( count ) );
    return string_list;
}

void FaceDetectionEmbeddedWidget::set_combobox_value( QString value ) {
    ui->mpComboBox->setCurrentText( value );
}

QString FaceDetectionEmbeddedWidget::get_combobox_text() {
    return ui->mpComboBox->currentText();
}
