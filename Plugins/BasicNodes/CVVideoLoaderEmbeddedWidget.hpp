//Copyright © 2025, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

/**
 * @file CVVideoLoaderEmbeddedWidget.hpp
 * @brief Interactive widget for video file playback control.
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QVariant>
#include <QWidget>
#include <QLabel>

namespace Ui {
class CVVideoLoaderEmbeddedWidget;
}

class CVVideoLoaderEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVVideoLoaderEmbeddedWidget( QWidget *parent = nullptr );
    ~CVVideoLoaderEmbeddedWidget();

    void set_filename( QString );
    void set_flip_pause( bool );
    void set_maximum_slider( int );
    void set_slider_value( int );
    void set_toggle_play( bool );
    void pause_video();

Q_SIGNALS:
    void button_clicked_signal( int button );
    void slider_value_signal( int value );
    void widget_resized_signal();

public Q_SLOTS:
    void forward_button_clicked();
    void backward_button_clicked();
    void play_pause_button_clicked();
    void filename_button_clicked();
    void slider_value_changed( int );
    void frame_number_spinbox_value_changed( int );

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::CVVideoLoaderEmbeddedWidget *ui;
};
