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

#include "RGBsetValueEmbeddedWidget.hpp"
#include "ui_RGBsetValueEmbeddedWidget.h"

RGBsetValueEmbeddedWidget::RGBsetValueEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RGBsetValueEmbeddedWidget)
{
    ui->setupUi(this);
}

RGBsetValueEmbeddedWidget::~RGBsetValueEmbeddedWidget()
{
    delete ui;
}

void RGBsetValueEmbeddedWidget::on_mpResetButton_clicked()
{
    Q_EMIT button_clicked_signal( 0 );
}
