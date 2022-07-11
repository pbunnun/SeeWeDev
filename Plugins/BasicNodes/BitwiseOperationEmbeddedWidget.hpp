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

#ifndef BITWISEOPERATIONEMBEDDEDWIDGET_HPP
#define BITWISEOPERATIONEMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class BitwiseOperationEmbeddedWidget;
}

class BitwiseOperationEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:

    explicit BitwiseOperationEmbeddedWidget(QWidget *parent = nullptr);
    ~BitwiseOperationEmbeddedWidget();

    void set_maskStatus_label(bool active);

private:
    Ui::BitwiseOperationEmbeddedWidget *ui;
};

#endif // BITWISEOPERATIONEMBEDDEDWIDGET_HPP
