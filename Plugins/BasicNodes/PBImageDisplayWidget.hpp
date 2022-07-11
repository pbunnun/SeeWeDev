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

#ifndef PBIMAGEDISPLAYWIDGET_HPP
#define PBIMAGEDISPLAYWIDGET_HPP

#include "CVDevLibrary.hpp"

#include <QOpenGLWidget>
#include <QPainter>
#include <opencv2/core/core.hpp>

class PBImageDisplayWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    PBImageDisplayWidget( QWidget *parent = nullptr );
    void Display( const cv::Mat &image );

protected:
    void paintEvent( QPaintEvent * ) override;

private:
    cv::Mat mCVImage;

    QPainter mPainter;

    quint8 muImageFormat{0};
    qreal mrScale_x{1.};
    qreal mrScale_y{1.};
};

#endif
