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

#include "InformationDisplayEmbeddedWidget.hpp"
#include "ui_InformationDisplayEmbeddedWidget.h"
#include <QDebug>
#include <QFileDialog>

InformationDisplayEmbeddedWidget::InformationDisplayEmbeddedWidget( QWidget *parent )
    : QWidget( parent ),
      ui( new Ui::InformationDisplayEmbeddedWidget )
{
    ui->setupUi( this );
    ui->mpPlainTextEdit->setMaximumBlockCount( 1000 );
    ui->mpPlainTextEdit->setReadOnly( true );
}

InformationDisplayEmbeddedWidget::~InformationDisplayEmbeddedWidget()
{
    delete ui;
}

void
InformationDisplayEmbeddedWidget::
on_mpClearButton_clicked()
{
    ui->mpPlainTextEdit->clear();
}

void
InformationDisplayEmbeddedWidget::
on_mpExportButton_clicked()
{
    QString text = ui->mpPlainTextEdit->toPlainText();
    QString filename = QFileDialog::getSaveFileName(qobject_cast<QWidget *>(this),
                                                    tr("Export to a text file"),
                                                    QDir::homePath(),
                                                    tr("Text (*.txt)"));
    if( !filename.isEmpty() )
    {
        QFile file(filename);
        if( file.open(QIODevice::WriteOnly) )
        {
            QTextStream out(&file);
            out << text;
            file.close();
        }
    }
}

void
InformationDisplayEmbeddedWidget::
appendPlainText(QString text)
{
    ui->mpPlainTextEdit->appendPlainText(text);
}
