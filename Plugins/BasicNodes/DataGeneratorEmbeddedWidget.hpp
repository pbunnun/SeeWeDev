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

#ifndef DATAGENERATOREMBEDDEDWIDGET_HPP
#define DATAGENERATOREMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class DataGeneratorEmbeddedWidget;
}

class DataGeneratorEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataGeneratorEmbeddedWidget(QWidget *parent = nullptr);
    ~DataGeneratorEmbeddedWidget();

    QStringList get_combobox_string_list() const;

    int get_combobox_index() const;

    QString get_text_input() const;

    void set_combobox_index(const int index);

    void set_text_input(const QString& input);

Q_SIGNALS :

    void widget_clicked_signal();

private Q_SLOTS :

    void on_mpComboBox_currentIndexChanged( int );

    void on_mpPlainTextEdit_textChanged();

private:
    Ui::DataGeneratorEmbeddedWidget *ui;

    static const QStringList comboxboxStringList;
};

#endif // DATAGENERATOREMBEDDEDWIDGET_HPP
