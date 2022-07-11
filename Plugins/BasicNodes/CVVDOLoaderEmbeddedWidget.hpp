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

#ifndef CVVDOLOADEREMBEDDEDWIDGET_H
#define CVVDOLOADEREMBEDDEDWIDGET_H
#include "CVDevLibrary.hpp"

#include <QVariant>
#include <QWidget>
#include <QLabel>

namespace Ui {
class CVVDOLoaderEmbeddedWidget;
}

class CVVDOLoaderEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVVDOLoaderEmbeddedWidget( QWidget *parent = nullptr );
    ~CVVDOLoaderEmbeddedWidget();

    void
    set_filename( QString );

    void
    set_active( bool );

    void
    set_flip_pause( bool );

    void
    set_maximum_slider( int );

    void
    set_slider_value( int );

Q_SIGNALS:
    void
    button_clicked_signal( int button );

    void
    slider_value_signal( int value );

public Q_SLOTS:

    void
    on_mpForwardButton_clicked();

    void
    on_mpBackwardButton_clicked();

    void
    on_mpPlayPauseButton_clicked();

    void
    on_mpFilenameButton_clicked();

    void
    on_mpSlider_valueChanged( int );

private:
    Ui::CVVDOLoaderEmbeddedWidget *ui;
};

#endif // CVIMAGELOADEREMBEDDEDWIDGET_H
