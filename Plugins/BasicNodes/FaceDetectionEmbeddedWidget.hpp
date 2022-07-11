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

#ifndef FACEDETECTIONEMBEDDEDWIDGET_H
#define FACEDETECTIONEMBEDDEDWIDGET_H

#include <QWidget>

namespace Ui {
    class FaceDetectionEmbeddedWidget;
}

class FaceDetectionEmbeddedWidget : public QWidget {
    Q_OBJECT

    public:
        explicit FaceDetectionEmbeddedWidget( QWidget *parent = nullptr );
        ~FaceDetectionEmbeddedWidget();

        QStringList get_combobox_string_list();

        void set_combobox_value( QString value );

        QString get_combobox_text();

    Q_SIGNALS:
        void button_clicked_signal( int button );

    public Q_SLOTS:
        void on_mpComboBox_currentIndexChanged( int );

    private:
        Ui::FaceDetectionEmbeddedWidget *ui;
};

#endif // FACEDETECTIONEMBEDDEDWIDGET_H
