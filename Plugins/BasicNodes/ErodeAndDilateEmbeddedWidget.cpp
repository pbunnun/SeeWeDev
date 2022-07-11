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

#include "ErodeAndDilateEmbeddedWidget.hpp"
#include "ui_ErodeAndDilateEmbeddedWidget.h"

ErodeAndDilateEmbeddedWidget::ErodeAndDilateEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ErodeAndDilateEmbeddedWidget)
{
    ui->setupUi(this);
    ui->mpErodeRadioButton->setChecked(true);
}

ErodeAndDilateEmbeddedWidget::~ErodeAndDilateEmbeddedWidget()
{
    delete ui;
}

void ErodeAndDilateEmbeddedWidget::on_mpErodeRadioButton_clicked()
{
    currentState = 0;
    Q_EMIT radioButton_clicked_signal();
}

void ErodeAndDilateEmbeddedWidget::on_mpDilateRadioButton_clicked()
{
    currentState = 1;
    Q_EMIT radioButton_clicked_signal();
}

int ErodeAndDilateEmbeddedWidget::getCurrentState() const
{
    return currentState;
}

void ErodeAndDilateEmbeddedWidget::setCurrentState(const int state)
{
    currentState = state;
    switch(currentState)
    {
    case 0:
        ui->mpErodeRadioButton->setChecked(true);
        break;

    case 1:
        ui->mpDilateRadioButton->setChecked(true);
        break;
    }
}
