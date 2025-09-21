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

#ifndef SYNCGATEEMBEDDEDWIDGET_HPP
#define SYNCGATEEMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class SyncGateEmbeddedWidget;
}

class SyncGateEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SyncGateEmbeddedWidget(QWidget *parent = nullptr);
    ~SyncGateEmbeddedWidget();

    bool get_in0_Checkbox() const;
    bool get_in1_Checkbox() const;
    bool get_out0_Checkbox() const;
    bool get_out1_Checkbox() const;
    void set_in0_Checkbox(const bool state);
    void set_in1_Checkbox(const bool state);
    void set_out0_Checkbox(const bool state);
    void set_out1_Checkbox(const bool state);

Q_SIGNALS :
    void checkbox_checked_signal(int checkbox, int state);

private Q_SLOTS :
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    void on_mpIn0Checkbox_stateChanged(int arg1);
    void on_mpIn1Checkbox_stateChanged(int arg1);
    void on_mpOut0Checkbox_stateChanged(int arg1);
    void on_mpOut1Checkbox_stateChanged(int arg1);
#else
    void on_mpIn0Checkbox_checkStateChanged(Qt::CheckState arg1);
    void on_mpIn1Checkbox_checkStateChanged(Qt::CheckState arg1);
    void on_mpOut0Checkbox_checkStateChanged(Qt::CheckState arg1);
    void on_mpOut1Checkbox_checkStateChanged(Qt::CheckState arg1);
#endif

private:
    Ui::SyncGateEmbeddedWidget *ui;
};

#endif // SYNCGATEEMBEDDEDWIDGET_HPP
