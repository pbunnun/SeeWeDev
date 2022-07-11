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

#include "BlendImagesEmbeddedWidget.hpp"
#include "ui_BlendImagesEmbeddedWidget.h"

BlendImagesEmbeddedWidget::BlendImagesEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BlendImagesEmbeddedWidget)
{
    ui->setupUi(this);
    ui->mpAddWeightedRadioButton->setChecked(true);
}

BlendImagesEmbeddedWidget::~BlendImagesEmbeddedWidget()
{
    delete ui;
}

void BlendImagesEmbeddedWidget::on_mpAddRadioButton_clicked()
{
    currentState = 0;
    Q_EMIT radioButton_clicked_signal();
}

void BlendImagesEmbeddedWidget::on_mpAddWeightedRadioButton_clicked()
{
    currentState = 1;
    Q_EMIT radioButton_clicked_signal();
}

int BlendImagesEmbeddedWidget::getCurrentState() const
{
    return currentState;
}

void BlendImagesEmbeddedWidget::setCurrentState(const int state)
{
    currentState = state;
    switch(currentState)
    {
    case 0:
        ui->mpAddRadioButton->setChecked(true);
        break;

    case 1:
        ui->mpAddWeightedRadioButton->setChecked(true);
        break;
    }
}
