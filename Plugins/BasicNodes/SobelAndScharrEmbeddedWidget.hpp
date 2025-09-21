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

#ifndef SOBELANDSCHARREMBEDDEDWIDGET_HPP
#define SOBELANDSCHARREMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class SobelAndScharrEmbeddedWidget;
}

class SobelAndScharrEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SobelAndScharrEmbeddedWidget(QWidget *parent = nullptr);
    ~SobelAndScharrEmbeddedWidget();

    void change_enable_checkbox(const bool enable);
    void change_check_checkbox(const Qt::CheckState state);
    bool checkbox_is_enabled();
    bool checkbox_is_checked();

Q_SIGNALS:
    void checkbox_checked_signal( int state );

private Q_SLOTS:
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    void on_mpCheckBox_stateChanged(int state);
#else
    void on_mpCheckBox_checkStateChanged(Qt::CheckState state);
#endif
private:
    Ui::SobelAndScharrEmbeddedWidget *ui;
};

#endif // SOBELANDSCHARREMBEDDEDWIDGET_HPP
