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

#ifndef MATHCONDITIONEMBEDDEDWIDGET_H
#define MATHCONDITIONEMBEDDEDWIDGET_H

#include <QWidget>
#include <QSpinBox>

namespace Ui {
class MathConditionEmbeddedWidget;
}

class MathConditionEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MathConditionEmbeddedWidget( QWidget *parent = nullptr );
    ~MathConditionEmbeddedWidget();

    QStringList
    get_condition_string_list();

    void
    set_condition_text_index( int idx );

    int
    get_condition_text_index();

    void
    set_condition_number( QString number );

    QString
    get_condition_number( );

public Q_SLOTS:
    void
    on_mpConditionComboBox_currentIndexChanged( int idx );

    void
    on_mpConditionNumber_textChanged( const QString & text );

Q_SIGNALS:
    void
    condition_changed_signal( int cond_idx, QString number );

private:
    Ui::MathConditionEmbeddedWidget *ui;
};

#endif // MATHCONDITIONEMBEDDEDWIDGET_H
