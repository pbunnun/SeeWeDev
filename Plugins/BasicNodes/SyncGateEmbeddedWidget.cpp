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

#include "SyncGateEmbeddedWidget.hpp"
#include "ui_SyncGateEmbeddedWidget.h"

SyncGateEmbeddedWidget::SyncGateEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SyncGateEmbeddedWidget)
{
    ui->setupUi(this);
}

SyncGateEmbeddedWidget::~SyncGateEmbeddedWidget()
{
    delete ui;
}

bool SyncGateEmbeddedWidget::get_in0_Checkbox() const
{
    return ui->mpIn0Checkbox->isChecked();
}

bool SyncGateEmbeddedWidget::get_in1_Checkbox() const
{
    return ui->mpIn1Checkbox->isChecked();
}

bool SyncGateEmbeddedWidget::get_out0_Checkbox() const
{
    return ui->mpOut0Checkbox->isChecked();
}

bool SyncGateEmbeddedWidget::get_out1_Checkbox() const
{
    return ui->mpOut1Checkbox->isChecked();
}

void SyncGateEmbeddedWidget::set_in0_Checkbox(const bool state)
{
    ui->mpIn0Checkbox->setCheckState(state? Qt::Checked : Qt::Unchecked);
}

void SyncGateEmbeddedWidget::set_in1_Checkbox(const bool state)
{
    ui->mpIn1Checkbox->setCheckState(state? Qt::Checked : Qt::Unchecked);
}

void SyncGateEmbeddedWidget::set_out0_Checkbox(const bool state)
{
    ui->mpOut0Checkbox->setCheckState(state? Qt::Checked : Qt::Unchecked);
}

void SyncGateEmbeddedWidget::set_out1_Checkbox(const bool state)
{
    ui->mpOut1Checkbox->setCheckState(state? Qt::Checked : Qt::Unchecked);
}

void SyncGateEmbeddedWidget::on_mpIn0Checkbox_stateChanged(int state)
{
    Q_EMIT checkbox_checked_signal(0,state);
}

void SyncGateEmbeddedWidget::on_mpIn1Checkbox_stateChanged(int state)
{
    Q_EMIT checkbox_checked_signal(1,state);
}

void SyncGateEmbeddedWidget::on_mpOut0Checkbox_stateChanged(int state)
{
    Q_EMIT checkbox_checked_signal(2,state);
}

void SyncGateEmbeddedWidget::on_mpOut1Checkbox_stateChanged(int state)
{
    Q_EMIT checkbox_checked_signal(3,state);
}
