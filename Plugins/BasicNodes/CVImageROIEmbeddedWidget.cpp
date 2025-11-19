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

#include "CVImageROIEmbeddedWidget.hpp"
#include <QPushButton>
#include "ui_CVImageROIEmbeddedWidget.h"

CVImageROIEmbeddedWidget::CVImageROIEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CVImageROIEmbeddedWidget)
{
    ui->setupUi(this);
    connect(ui->mpApplyButton, &QPushButton::clicked, this, &CVImageROIEmbeddedWidget::apply_button_clicked);
    connect(ui->mpResetButton, &QPushButton::clicked, this, &CVImageROIEmbeddedWidget::reset_button_clicked);

    ui->mpApplyButton->setEnabled(false);
    ui->mpResetButton->setEnabled(false);
}

CVImageROIEmbeddedWidget::~CVImageROIEmbeddedWidget()
{
    delete ui;
}

void CVImageROIEmbeddedWidget::apply_button_clicked()
{
    Q_EMIT button_clicked_signal(1);
}

void CVImageROIEmbeddedWidget::reset_button_clicked()
{
    Q_EMIT button_clicked_signal(0);
}

void CVImageROIEmbeddedWidget::enable_apply_button(const bool enable)
{
    ui->mpApplyButton->setEnabled(enable);
}

void CVImageROIEmbeddedWidget::enable_reset_button(const bool enable)
{
    ui->mpResetButton->setEnabled(enable);
}
