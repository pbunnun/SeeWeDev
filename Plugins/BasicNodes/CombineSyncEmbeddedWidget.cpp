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

#include "CombineSyncEmbeddedWidget.hpp"
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include "ui_CombineSyncEmbeddedWidget.h"

CombineSyncEmbeddedWidget::CombineSyncEmbeddedWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CombineSyncEmbeddedWidget)
{
    ui->setupUi(this);

    // Connect signals from UI widgets
    connect(ui->mpComboBox, &QComboBox::currentTextChanged, 
            this, &CombineSyncEmbeddedWidget::operation_changed_signal);
    connect(ui->mpSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &CombineSyncEmbeddedWidget::input_size_changed_signal);
    connect(ui->mpResetButton, &QPushButton::clicked, 
            this, &CombineSyncEmbeddedWidget::reset_clicked_signal);
}

CombineSyncEmbeddedWidget::~CombineSyncEmbeddedWidget()
{
    delete ui;
}

void
CombineSyncEmbeddedWidget::
set_operation(int index)
{
    if (index >= 0 && index < ui->mpComboBox->count())
    {
        ui->mpComboBox->blockSignals(true);
        ui->mpComboBox->setCurrentIndex(index);
        ui->mpComboBox->blockSignals(false);
    }
}

void
CombineSyncEmbeddedWidget::
set_input_size(int size)
{
    ui->mpSpinBox->blockSignals(true);
    ui->mpSpinBox->setValue(size);
    ui->mpSpinBox->blockSignals(false);
}

int
CombineSyncEmbeddedWidget::
get_operation() const
{
    return ui->mpComboBox->currentIndex();
}

int
CombineSyncEmbeddedWidget::
get_input_size() const
{
    return ui->mpSpinBox->value();
}
