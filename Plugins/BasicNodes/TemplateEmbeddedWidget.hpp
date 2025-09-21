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

#ifndef TEMPLATEEMBEDDEDWIDGET_H
#define TEMPLATEEMBEDDEDWIDGET_H

#include <QWidget>
#include <QSpinBox>

namespace Ui {
class TemplateEmbeddedWidget;
}

class TemplateEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TemplateEmbeddedWidget( QWidget *parent = nullptr );
    ~TemplateEmbeddedWidget();

    QStringList
    get_combobox_string_list();

    const QSpinBox *
    get_spinbox();

    void
    set_combobox_value( QString value );

    void
    set_display_text( QString value );

    void
    set_spinbox_value( int value );

    void
    set_active_button( bool startButtonActive );

    QString
    get_combobox_text();

Q_SIGNALS:
    void
    button_clicked_signal( int button );

public Q_SLOTS:
    void
    on_mpStartButton_clicked();

    void
    on_mpStopButton_clicked();

    void
    on_mpSpinBox_valueChanged( int );

    void
    on_mpComboBox_currentIndexChanged( int );

    void
    on_mpSendButton_clicked();

private:
    Ui::TemplateEmbeddedWidget *ui;
};

#endif // TEMPLATEEMBEDDEDWIDGET_H
