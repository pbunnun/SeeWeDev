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

#include "SobelAndScharrEmbeddedWidget.hpp"
#include "ui_SobelAndScharrEmbeddedWidget.h"

SobelAndScharrEmbeddedWidget::SobelAndScharrEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SobelAndScharrEmbeddedWidget)
{
    ui->setupUi(this);
}

SobelAndScharrEmbeddedWidget::~SobelAndScharrEmbeddedWidget()
{
    delete ui;
}
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
void SobelAndScharrEmbeddedWidget::on_mpCheckBox_stateChanged(int state)
{
    Q_EMIT checkbox_checked_signal( state );
}
#else
void SobelAndScharrEmbeddedWidget::on_mpCheckBox_checkStateChanged(Qt::CheckState state)
{
    Q_EMIT checkbox_checked_signal( state );
}
#endif
void SobelAndScharrEmbeddedWidget::change_enable_checkbox(const bool enable)
{
    ui->mpCheckBox->setEnabled(enable);
}

void SobelAndScharrEmbeddedWidget::change_check_checkbox(const Qt::CheckState state)
{
    ui->mpCheckBox->setCheckState(state);
}

bool SobelAndScharrEmbeddedWidget::checkbox_is_enabled()
{
    return ui->mpCheckBox->isEnabled();
}

bool SobelAndScharrEmbeddedWidget::checkbox_is_checked()
{
    return ui->mpCheckBox->isChecked();
}
