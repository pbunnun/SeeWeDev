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

#include <QtOpenGL/QtOpenGL>
#include "PBImageDisplayWidget.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <QDebug>
#include <cmath>

PBImageDisplayWidget::
PBImageDisplayWidget(QWidget *parent)
    : ImageDisplayWidget( parent )
{
    QSize minSize = QSize( 80, 60 );
    setMinimumSize( minSize );
    resize(QSize(640,480));
    setAutoFillBackground( false );
}

void
PBImageDisplayWidget::
Display( const cv::Mat &image )
{
    // Don't process if the widget is not visible or image is empty
    if (!isVisible() || image.empty())
        return;
        
    // Create a deep copy to avoid race conditions with paintEvent
    mCVImage = image.clone();
    muImageFormat = static_cast< quint8 >( mCVImage.channels() );
    if( mCVImage.cols != miImageWidth || mCVImage.rows != miImageHeight )
    {
        miImageWidth = mCVImage.cols;
        miImageHeight = mCVImage.rows;
        if( mCVImage.cols != 0 && mCVImage.rows != 0 )
        {
            mrScale_x = static_cast< qreal >( this->width() )/static_cast< qreal >( mCVImage.cols );
            mrScale_y = static_cast< qreal >( this->height() )/static_cast< qreal >( mCVImage.rows );
        }
    }
    repaint();
}

void
PBImageDisplayWidget::
paintEvent( QPaintEvent * )
{
    // Don't paint if widget is not visible or image data is invalid
    if (!isVisible() || mCVImage.empty() || mCVImage.data == nullptr)
        return;
        
    mPainter.begin( this );
    mPainter.setRenderHint(QPainter::Antialiasing);
    if( muImageFormat == 1 )
    {
        QImage image( static_cast< uchar* >( mCVImage.data ), mCVImage.cols, mCVImage.rows, static_cast< qint32 >( mCVImage.step ), QImage::Format_Grayscale8 );
        mPainter.scale( mrScale_x, mrScale_y );
        mPainter.drawImage( 0, 0, image, 0, 0, image.width(), image.height(), Qt::MonoOnly );
    }
    else if( muImageFormat == 3 )
    {
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
        QImage image = QImage( static_cast<uchar*>(mCVImage.data), mCVImage.cols, mCVImage.rows, static_cast< qint32 >( mCVImage.step ), QImage::Format_RGB888 ).rgbSwapped();
#else
        QImage image = QImage( static_cast<uchar*>(mCVImage.data), mCVImage.cols, mCVImage.rows, static_cast< qint32 >( mCVImage.step ), QImage::Format_BGR888 );
#endif
        mPainter.scale( mrScale_x, mrScale_y );
        mPainter.drawImage( 0, 0, image, 0, 0, image.width(), image.height(), Qt::ColorOnly );
    }
    mPainter.end();
}

void
PBImageDisplayWidget::
resizeEvent( QResizeEvent * ev )
{
    // Enforce aspect ratio if image is loaded
    if (miImageWidth > 0 && miImageHeight > 0)
    {
        double aspectRatio = static_cast<double>(miImageHeight) / static_cast<double>(miImageWidth);
        int targetHeight = static_cast<int>(width() * aspectRatio);
        
        // Only adjust if the difference is significant (more than 2 pixels)
        if (std::abs(targetHeight - height()) > 2)
        {
            // Block signals to avoid recursion
            bool oldState = signalsBlocked();
            blockSignals(true);
            setFixedHeight(targetHeight);
            blockSignals(oldState);
        }
    }
    
    if( mCVImage.cols != 0 && mCVImage.rows != 0 )
    {
        mrScale_x = static_cast< qreal >( this->width() )/static_cast< qreal >( mCVImage.cols );
        mrScale_y = static_cast< qreal >( this->height() )/static_cast< qreal >( mCVImage.rows );
    }
    ImageDisplayWidget::resizeEvent(ev);
}

