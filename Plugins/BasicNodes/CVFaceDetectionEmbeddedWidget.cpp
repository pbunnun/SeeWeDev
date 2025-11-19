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

#include "CVFaceDetectionEmbeddedWidget.hpp"
#include <QComboBox>
#include "ui_CVFaceDetectionEmbeddedWidget.h"
#include <QDebug>

CVFaceDetectionEmbeddedWidget::CVFaceDetectionEmbeddedWidget( QWidget *parent )
    : QWidget( parent ),
      ui( new Ui::CVFaceDetectionEmbeddedWidget )
{
    ui->setupUi( this );
    connect(ui->mpComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CVFaceDetectionEmbeddedWidget::combo_box_current_index_changed);

}

CVFaceDetectionEmbeddedWidget::~CVFaceDetectionEmbeddedWidget() {
    delete ui;
}

void CVFaceDetectionEmbeddedWidget::combo_box_current_index_changed( int idx ) {

    qDebug() << "ComboBox : current index is " << idx;
    Q_EMIT button_clicked_signal( 3 );
}

QStringList CVFaceDetectionEmbeddedWidget::get_combobox_string_list() {
    QStringList string_list;
    for( int count = 0; count < ui->mpComboBox->count(); ++count )
        string_list.append( ui->mpComboBox->itemText( count ) );
    return string_list;
}

void CVFaceDetectionEmbeddedWidget::set_combobox_value( QString value ) {
    ui->mpComboBox->setCurrentText( value );
}

QString CVFaceDetectionEmbeddedWidget::get_combobox_text() {
    return ui->mpComboBox->currentText();
}
