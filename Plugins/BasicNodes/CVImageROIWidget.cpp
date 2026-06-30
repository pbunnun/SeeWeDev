//Copyright © 2026, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "CVImageROIWidget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <algorithm>

CVImageROIWidget::
CVImageROIWidget( QWidget *parent )
    : PBImageDisplayWidget( parent )
{
    setMouseTracking( false );
    setCursor( Qt::CrossCursor );
}

void
CVImageROIWidget::
Display( const cv::Mat &image )
{
    if( !image.empty() )
    {
        mImageWidth  = image.cols;
        mImageHeight = image.rows;
    }
    PBImageDisplayWidget::Display( image );
}

void
CVImageROIWidget::
setDisplayROI( const cv::Rect &roi )
{
    mROIImageRect = QRect( roi.x, roi.y, roi.width, roi.height );
    repaint();
}

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------

void
CVImageROIWidget::
mousePressEvent( QMouseEvent *ev )
{
    Q_EMIT widgetClicked();
    
    if( ev->button() == Qt::LeftButton && mImageWidth > 0 && mImageHeight > 0 )
    {
        mIsDragging       = true;
        mDragStartWidget  = ev->pos();
        mDragCurrentWidget = ev->pos();
        ev->accept();
    }
    else
    {
        PBImageDisplayWidget::mousePressEvent( ev );
    }
}

void
CVImageROIWidget::
mouseMoveEvent( QMouseEvent *ev )
{
    if( mIsDragging )
    {
        mDragCurrentWidget = ev->pos();
        repaint();
        ev->accept();
    }
    else
    {
        PBImageDisplayWidget::mouseMoveEvent( ev );
    }
}

void
CVImageROIWidget::
mouseReleaseEvent( QMouseEvent *ev )
{
    if( ev->button() == Qt::LeftButton && mIsDragging )
    {
        mIsDragging        = false;
        mDragCurrentWidget = ev->pos();

        // Build the drag rect in widget coordinates (normalised).
        QRect widgetRect = QRect( mDragStartWidget, mDragCurrentWidget ).normalized();

        if( widgetRect.width() > 2 && widgetRect.height() > 2 )
        {
            // Convert to image coordinates.
            QPoint topLeft     = widgetToImage( widgetRect.topLeft() );
            QPoint bottomRight = widgetToImage( widgetRect.bottomRight() );

            // Clamp to image bounds.
            topLeft.setX( std::max( 0, std::min( topLeft.x(), mImageWidth  - 1 ) ) );
            topLeft.setY( std::max( 0, std::min( topLeft.y(), mImageHeight - 1 ) ) );
            bottomRight.setX( std::max( 0, std::min( bottomRight.x(), mImageWidth  - 1 ) ) );
            bottomRight.setY( std::max( 0, std::min( bottomRight.y(), mImageHeight - 1 ) ) );

            QRect imageRect( topLeft, bottomRight );
            if( imageRect.width() > 0 && imageRect.height() > 0 )
            {
                mROIImageRect = imageRect;
                Q_EMIT roiSelected( imageRect );
            }
        }

        repaint();
        ev->accept();
    }
    else
    {
        PBImageDisplayWidget::mouseReleaseEvent( ev );
    }
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void
CVImageROIWidget::
paintEvent( QPaintEvent *ev )
{
    // Draw the image via the base class first.
    PBImageDisplayWidget::paintEvent( ev );

    // Nothing to overlay if no image has been received yet.
    if( mImageWidth <= 0 || mImageHeight <= 0 )
        return;

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing, false );

    // Draw the committed ROI overlay.
    if( mROIImageRect.isValid() && mROIImageRect.width() > 0 && mROIImageRect.height() > 0 )
    {
        QRectF roiWidget = imageToWidget( mROIImageRect );

        // Semi-transparent fill.
        painter.fillRect( roiWidget, QColor( 0, 120, 255, 40 ) );

        // Solid border.
        QPen pen( QColor( 0, 180, 255 ), 2, Qt::SolidLine );
        painter.setPen( pen );
        painter.drawRect( roiWidget );
    }

    // Draw the live drag rubber-band on top.
    if( mIsDragging )
    {
        QRect dragWidgetRect = QRect( mDragStartWidget, mDragCurrentWidget ).normalized();
        painter.fillRect( dragWidgetRect, QColor( 255, 200, 0, 30 ) );

        QPen dragPen( QColor( 255, 180, 0 ), 1, Qt::DashLine );
        painter.setPen( dragPen );
        painter.drawRect( dragWidgetRect );
    }

    painter.end();
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

QPoint
CVImageROIWidget::
widgetToImage( const QPoint &widgetPt ) const
{
    if( width() == 0 || height() == 0 )
        return widgetPt;

    int ix = static_cast< int >( widgetPt.x() * mImageWidth  / static_cast< double >( width() ) );
    int iy = static_cast< int >( widgetPt.y() * mImageHeight / static_cast< double >( height() ) );
    return QPoint( ix, iy );
}

QRectF
CVImageROIWidget::
imageToWidget( const QRect &imageRect ) const
{
    if( mImageWidth == 0 || mImageHeight == 0 )
        return QRectF( imageRect );

    double sx = static_cast< double >( width()  ) / mImageWidth;
    double sy = static_cast< double >( height() ) / mImageHeight;

    return QRectF( imageRect.x()      * sx,
                   imageRect.y()      * sy,
                   imageRect.width()  * sx,
                   imageRect.height() * sy );
}
