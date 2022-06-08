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
