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

#include "CVSobelAndScharrEmbeddedWidget.hpp"
#include <QCheckBox>
#include "ui_CVSobelAndScharrEmbeddedWidget.h"

CVSobelAndScharrEmbeddedWidget::CVSobelAndScharrEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CVSobelAndScharrEmbeddedWidget)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(ui->mpCheckBox, QOverload<int>::of(&QCheckBox::stateChanged), this, &CVSobelAndScharrEmbeddedWidget::check_box_state_changed);
#else
    connect(ui->mpCheckBox, &QCheckBox::checkStateChanged, this, &CVSobelAndScharrEmbeddedWidget::check_box_check_state_changed);
#endif
}

CVSobelAndScharrEmbeddedWidget::~CVSobelAndScharrEmbeddedWidget()
{
    delete ui;
}
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
void CVSobelAndScharrEmbeddedWidget::check_box_state_changed(int state)
{
    Q_EMIT checkbox_checked_signal( state );
}
#else
void CVSobelAndScharrEmbeddedWidget::check_box_check_state_changed(Qt::CheckState state)
{
    Q_EMIT checkbox_checked_signal( state );
}
#endif
void CVSobelAndScharrEmbeddedWidget::change_enable_checkbox(const bool enable)
{
    ui->mpCheckBox->setEnabled(enable);
}

void CVSobelAndScharrEmbeddedWidget::change_check_checkbox(const Qt::CheckState state)
{
    ui->mpCheckBox->setCheckState(state);
}

bool CVSobelAndScharrEmbeddedWidget::checkbox_is_enabled()
{
    return ui->mpCheckBox->isEnabled();
}

bool CVSobelAndScharrEmbeddedWidget::checkbox_is_checked()
{
    return ui->mpCheckBox->isChecked();
}
