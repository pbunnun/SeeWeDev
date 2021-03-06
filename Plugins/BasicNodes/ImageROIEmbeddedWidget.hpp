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

#ifndef IMAGEROIEMBEDDEDWIDGET_HPP
#define IMAGEROIEMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class ImageROIEmbeddedWidget;
}

class ImageROIEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageROIEmbeddedWidget(QWidget *parent = nullptr);
    ~ImageROIEmbeddedWidget();

    void enable_apply_button(const bool enable);

    void enable_reset_button(const bool enable);

Q_SIGNALS :

    void button_clicked_signal( int button );

private Q_SLOTS :

    void on_mpApplyButton_clicked();

    void on_mpResetButton_clicked();

private:
    Ui::ImageROIEmbeddedWidget *ui;
};

#endif // IMAGEROIEMBEDDEDWIDGET_HPP
