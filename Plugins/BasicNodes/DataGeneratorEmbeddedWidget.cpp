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

#include "DataGeneratorEmbeddedWidget.hpp"
#include <QComboBox>
#include <QPlainTextEdit>
#include "ui_DataGeneratorEmbeddedWidget.h"
#include <QDebug>

const QStringList DataGeneratorEmbeddedWidget::comboxboxStringList = {"int", "float", "double", "bool", "std::string", "cv::Rect", "cv::Point", "cv::Scalar"};

DataGeneratorEmbeddedWidget::DataGeneratorEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataGeneratorEmbeddedWidget)
{
    ui->setupUi(this);
    connect(ui->mpComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DataGeneratorEmbeddedWidget::combo_box_current_index_changed);
    connect(ui->mpPlainTextEdit, &QPlainTextEdit::textChanged, this, &DataGeneratorEmbeddedWidget::plain_text_edit_text_changed);

    ui->mpPlainTextEdit->setMaximumBlockCount(100);
    ui->mpPlainTextEdit->setReadOnly(false);
}

DataGeneratorEmbeddedWidget::~DataGeneratorEmbeddedWidget()
{
    delete ui;
}

void DataGeneratorEmbeddedWidget::combo_box_current_index_changed( int )
{
    Q_EMIT widget_clicked_signal();
}

void DataGeneratorEmbeddedWidget::plain_text_edit_text_changed()
{
    Q_EMIT widget_clicked_signal();
}

QStringList DataGeneratorEmbeddedWidget::get_combobox_string_list() const
{
    return comboxboxStringList;
}

int DataGeneratorEmbeddedWidget::get_combobox_index() const
{
    return ui->mpComboBox->currentIndex();
}

QString DataGeneratorEmbeddedWidget::get_text_input() const
{
    return ui->mpPlainTextEdit->toPlainText();
}

void DataGeneratorEmbeddedWidget::set_combobox_index(const int index)
{
    ui->mpComboBox->setCurrentIndex(index);
}

void DataGeneratorEmbeddedWidget::set_text_input(const QString &input)
{
    ui->mpPlainTextEdit->setPlainText(input);
}

