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

#ifndef FLOODFILLEMBEDDEDWIDGET_HPP
#define FLOODFILLEMBEDDEDWIDGET_HPP

#include <QWidget>
#include <QSpinBox>

namespace Ui {
class FloodFillEmbeddedWidget;
}

class FloodFillEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FloodFillEmbeddedWidget(QWidget *parent = nullptr);
    ~FloodFillEmbeddedWidget();

    void set_maskStatus_label(const bool active);

    void toggle_widgets(const int channels);

    void set_lower_upper(const int (&lower)[4], const int (&upper)[4]);

Q_SIGNALS:

    void spinbox_clicked_signal( int spinbox, int value );

private Q_SLOTS:

    void on_mpLowerBSpinbox_valueChanged( int );

    void on_mpLowerGSpinbox_valueChanged( int );

    void on_mpLowerRSpinbox_valueChanged( int );

    void on_mpLowerGraySpinbox_valueChanged( int );

    void on_mpUpperBSpinbox_valueChanged( int );

    void on_mpUpperGSpinbox_valueChanged( int );

    void on_mpUpperRSpinbox_valueChanged( int );

    void on_mpUpperGraySpinbox_valueChanged( int );

private:
    Ui::FloodFillEmbeddedWidget *ui;
};

#endif // FLOODFILLEMBEDDEDWIDGET_HPP
