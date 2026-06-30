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

/**
 * @file CVImageROIWidget.hpp
 * @brief Embedded image display widget with interactive ROI drag-selection.
 *
 * Extends PBImageDisplayWidget to allow users to drag a rectangle over the
 * displayed image to define a Region of Interest. The selected rectangle is
 * emitted as a signal in image (pixel) coordinates.
 *
 * **Interaction:**
 * - Drag left mouse button to draw a rectangle on the image.
 * - On release, emits roiSelected(QRect) in image-pixel coordinates.
 * - The active ROI rectangle is always drawn as an overlay.
 *
 * **Coordinate systems:**
 * - Widget coordinates: pixel position inside the QWidget.
 * - Image coordinates: pixel position in the original cv::Mat image.
 *   Conversion uses the ratio of widget size to image size.
 */

#pragma once

#include "PBImageDisplayWidget.hpp"
#include <QPoint>
#include <QRect>
#include <opencv2/core/core.hpp>

/**
 * @class CVImageROIWidget
 * @brief Image display widget that lets users drag-select an ROI rectangle.
 *
 * On top of PBImageDisplayWidget's image rendering, this widget:
 * - Intercepts mouse press / move / release to rubber-band an ROI rectangle.
 * - Paints a semi-transparent overlay showing the current ROI.
 * - Emits roiSelected(QRect) with the ROI in image-pixel coordinates when
 *   the user finishes dragging.
 * - Accepts external ROI updates via setDisplayROI() so the overlay stays in
 *   sync with property-panel edits.
 *
 * @see PBImageDisplayWidget
 * @see CVImageROINewModel
 */
class CVImageROIWidget : public PBImageDisplayWidget
{
    Q_OBJECT

public:
    explicit CVImageROIWidget( QWidget *parent = nullptr );

    /**
     * @brief Display an image and remember its pixel dimensions for coordinate mapping.
     * @param image OpenCV image to show (passed through to base class).
     */
    void Display( const cv::Mat &image );

    /**
     * @brief Update the displayed ROI rectangle (e.g. after a property-panel edit).
     * @param roi Rectangle in image-pixel coordinates.
     */
    void setDisplayROI( const cv::Rect &roi );

Q_SIGNALS:
    /**
     * @brief Emitted when the user finishes dragging a new ROI rectangle.
     * @param roi Selected rectangle in image-pixel coordinates.
     *            Both width and height are guaranteed to be positive.
     */
    void roiSelected( QRect roi );

    /**
     * @brief Emitted when the widget is clicked to request node selection.
     */
    void widgetClicked();

protected:
    void paintEvent( QPaintEvent *ev ) override;
    void mousePressEvent( QMouseEvent *ev ) override;
    void mouseMoveEvent( QMouseEvent *ev ) override;
    void mouseReleaseEvent( QMouseEvent *ev ) override;

private:
    /** Convert a point in widget coordinates to image-pixel coordinates. */
    QPoint widgetToImage( const QPoint &widgetPt ) const;

    /** Convert a rect in image-pixel coordinates to widget coordinates. */
    QRectF imageToWidget( const QRect &imageRect ) const;

    int     mImageWidth  { 0 };     ///< Width of the currently displayed image (pixels)
    int     mImageHeight { 0 };     ///< Height of the currently displayed image (pixels)

    QRect   mROIImageRect;          ///< Current ROI in image-pixel coordinates
    bool    mIsDragging  { false }; ///< True while left-button is held down
    QPoint  mDragStartWidget;       ///< Widget-coord origin of the current drag
    QPoint  mDragCurrentWidget;     ///< Widget-coord current drag endpoint
};
