#include <QtGui>
#include <QtOpenGL/QtOpenGL>
#include "PBImageDisplayWidget.hpp"
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <QDebug>

PBImageDisplayWidget::
PBImageDisplayWidget(QWidget *parent)
    : QOpenGLWidget( parent )
{
    QSize minSize = QSize( 80, 60 );
    setMinimumSize( minSize );
    resize(minSize);
    setAutoFillBackground( false );
}

void
PBImageDisplayWidget::
Display( const cv::Mat &image )
{
    mCVImage = image.clone();
    muImageFormat = static_cast< quint8 >( mCVImage.channels() );
    repaint();
}

void
PBImageDisplayWidget::
paintEvent( QPaintEvent * )
{
    mPainter.begin( this );
    if( muImageFormat == 1 )
    {
        QImage image( static_cast< uchar* >( mCVImage.data ), mCVImage.cols, mCVImage.rows, static_cast< qint32 >( mCVImage.step ), QImage::Format_Grayscale8 );
        mrScale_x = static_cast< qreal >( this->width() )/static_cast< qreal >( mCVImage.cols );
        mrScale_y = static_cast< qreal >( this->height() )/static_cast< qreal >( mCVImage.rows );
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
        mrScale_x = static_cast< qreal >( this->width() )/static_cast< qreal >( mCVImage.cols );
        mrScale_y = static_cast< qreal >( this->height() )/static_cast< qreal >( mCVImage.rows );
        mPainter.scale( mrScale_x, mrScale_y );
        mPainter.drawImage( 0, 0, image, 0, 0, image.width(), image.height(), Qt::ColorOnly );
    }
    mPainter.end();
}