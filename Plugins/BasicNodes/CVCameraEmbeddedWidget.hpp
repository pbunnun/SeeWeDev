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

#ifndef CVCAMERAEMBEDDEDWIDGET_H
#define CVCAMERAEMBEDDEDWIDGET_H
#include "CVDevLibrary.hpp"

#include <QVariant>
#include <QWidget>
#include <QLabel>

typedef struct CVCameraParameters{
    int miCameraID{0};
    bool mbCameraStatus{false};
} CVCameraParameters;

namespace Ui {
class CVCameraEmbeddedWidget;
}

class CVCameraEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVCameraEmbeddedWidget( QWidget *parent = nullptr );
    ~CVCameraEmbeddedWidget();

    void
    set_params( CVCameraParameters params );

    CVCameraParameters
    get_params() const { return mParams; }

    void
    set_ready_state( bool );

    void
    set_active( bool );

Q_SIGNALS:
    void
    button_clicked_signal( int button );

public Q_SLOTS:
    void
    camera_status_changed( bool );

    void
    on_mpStartButton_clicked();

    void
    on_mpStopButton_clicked();

    void
    on_mpCameraIDComboBox_currentIndexChanged( int );

private:
    CVCameraParameters mParams;
    QLabel * mpTransparentLabel;
    Ui::CVCameraEmbeddedWidget *ui;
};

#endif // CVCAMERAEMBEDDEDWIDGET_H
