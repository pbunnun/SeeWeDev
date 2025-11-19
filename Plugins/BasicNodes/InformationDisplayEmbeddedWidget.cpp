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

#include "InformationDisplayEmbeddedWidget.hpp"
#include "ui_InformationDisplayEmbeddedWidget.h"
#include <QDebug>
#include <QFileDialog>
#include <QPushButton>

InformationDisplayEmbeddedWidget::InformationDisplayEmbeddedWidget( QWidget *parent )
    : QWidget( parent ),
      ui( new Ui::InformationDisplayEmbeddedWidget )
{
    ui->setupUi( this );
    ui->mpPlainTextEdit->setMaximumBlockCount( 100 );
    ui->mpPlainTextEdit->setReadOnly( true );
    
    // Explicit connections (instead of automatic on_mpXXX naming)
    connect(ui->mpClearButton, &QPushButton::clicked, this, &InformationDisplayEmbeddedWidget::clear_button_clicked);
    connect(ui->mpExportButton, &QPushButton::clicked, this, &InformationDisplayEmbeddedWidget::export_button_clicked);
    
    // Install event filter on child widgets to capture clicks
    ui->mpPlainTextEdit->installEventFilter(this);
    ui->mpClearButton->installEventFilter(this);
    ui->mpExportButton->installEventFilter(this);
}

InformationDisplayEmbeddedWidget::~InformationDisplayEmbeddedWidget()
{
    delete ui;
}

void
InformationDisplayEmbeddedWidget::
setMaxLineCount( int maxLines )
{
    ui->mpPlainTextEdit->setMaximumBlockCount( maxLines );
}

void
InformationDisplayEmbeddedWidget::
clear_button_clicked()
{
    ui->mpPlainTextEdit->clear();
}

void
InformationDisplayEmbeddedWidget::
export_button_clicked()
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

bool
InformationDisplayEmbeddedWidget::
eventFilter(QObject* obj, QEvent* event)
{
    // Request node selection when any child widget gets focus
    if (obj == ui->mpPlainTextEdit || obj == ui->mpClearButton || obj == ui->mpExportButton)
    {
        if( event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress )
        {
            Q_EMIT widgetClicked();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void 
InformationDisplayEmbeddedWidget::
mousePressEvent(QMouseEvent* event)
{
    // Request node selection when widget is clicked
    Q_EMIT widgetClicked();
    QWidget::mousePressEvent(event);
}
