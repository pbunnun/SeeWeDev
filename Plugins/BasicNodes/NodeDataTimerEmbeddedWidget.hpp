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

#ifndef NODEDATATIMEREMBEDDEDWIDGET_HPP
#define NODEDATATIMEREMBEDDEDWIDGET_HPP

#include <QDebug>
#include <QWidget>
#include <QTimer>

namespace Ui {
class NodeDataTimerEmbeddedWidget;
}

class NodeDataTimerEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NodeDataTimerEmbeddedWidget(QWidget *parent = nullptr);
    ~NodeDataTimerEmbeddedWidget();

    int get_second_spinbox() const;
    int get_millisecond_spinbox() const;
    int get_pf_combobox() const;
    float get_remaining_time() const;
    bool get_start_button() const;
    bool get_stop_button() const;

    void set_second_spinbox(const int duration);
    void set_millisecond_spinbox(const int duration);
    void set_pf_combobox(const int index);
    void set_remaining_time(const float time);
    void set_start_button(const bool enable);
    void set_stop_button(const bool enable);

    void set_widget_bundle(const bool enable);
    void set_pf_labels(const int index);
    void set_remaining_label(const float duration);

    void run() const;
    void terminate() const;

Q_SIGNALS :

    void timeout_signal() const;

private Q_SLOTS :

    void on_mpSecondSpinbox_valueChanged(int duration);
    void on_mpMillisecondSpinbox_valueChanged(int duration);
    void on_mpPFComboBox_currentIndexChanged(int index);
    void on_mpStartButton_clicked();
    void on_mpStopButton_clicked();
    void on_mpResetButton_clicked();

    void on_singleShot();
    void on_timeout();

private:
    Ui::NodeDataTimerEmbeddedWidget *ui;
    int miInterval = 1;
    int miPeriod = 500;
    bool mbIsRunning = false;

    QTimer* mpQTimer;
};

#endif // NODEDATATIMEREMBEDDEDWIDGET_HPP
