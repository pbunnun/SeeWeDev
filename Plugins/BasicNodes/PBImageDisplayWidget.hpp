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
 * @file PBImageDisplayWidget.hpp
 * @brief Qt widget for efficient OpenCV image display with automatic scaling.
 *
 * This widget provides optimized rendering of cv::Mat images in Qt applications,
 * handling format conversion (BGR→RGB), scaling, and paint events. It uses
 * QOpenGLWidget (Qt < 6.7) or QLabel (Qt ≥ 6.7) as the base for hardware-accelerated
 * rendering when available.
 *
 * **Key Features:**
 * - Automatic cv::Mat to QPixmap/QImage conversion
 * - Adaptive scaling to fit widget size
 * - Supports grayscale and color images
 * - Hardware acceleration (OpenGL) when available
 * - Efficient repainting on resize events
 *
 * **Typical Use:**
 * Used internally by CVImageDisplay node for image visualization.
 *
 * @see CVImageDisplay for the node that uses this widget
 */

#pragma once

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0) )
    #define ImageDisplayWidget QOpenGLWidget
    #include <QOpenGLWidget>
#else
    #define ImageDisplayWidget QLabel
    #include <QLabel>
#endif

#include <QPainter>
#include <opencv2/core/core.hpp>

/**
 * @class PBImageDisplayWidget
 * @brief Optimized Qt widget for displaying OpenCV images.
 *
 * PBImageDisplayWidget handles the conversion and rendering of cv::Mat images
 * in Qt GUI applications. It automatically scales images to fit the widget size
 * while maintaining aspect ratio and provides efficient repainting.
 *
 * **Base Class Selection:**
 * - Qt < 6.7: Uses QOpenGLWidget for hardware-accelerated rendering
 * - Qt ≥ 6.7: Uses QLabel (OpenGL widget deprecated in newer Qt)
 *
 * **Image Format Support:**
 * - Grayscale (CV_8UC1): Displayed as grayscale QImage
 * - BGR Color (CV_8UC3): Converted to RGB for Qt display
 * - BGR Alpha (CV_8UC4): Converted to RGBA for Qt display
 *
 * **Usage Example:**
 * ```cpp
 * PBImageDisplayWidget *widget = new PBImageDisplayWidget(parent);
 * cv::Mat image = cv::imread("photo.jpg");
 * widget->Display(image);  // Renders image to widget
 * ```
 *
 * **Scaling Behavior:**
 * - Automatically scales to fit widget dimensions
 * - Maintains original aspect ratio
 * - Recalculates scale on resize events
 * - Supports both upscaling and downscaling
 *
 * **Performance:**
 * - Caches converted QImage internally (avoids repeated conversion)
 * - Only repaints on explicit Display() calls or resize events
 * - Uses Qt's optimized image rendering pipeline
 * - Hardware acceleration when QOpenGLWidget is used
 *
 * @note Images are converted from BGR (OpenCV) to RGB (Qt) for correct color display
 * @see CVImageDisplay for the node implementation using this widget
 */
class PBImageDisplayWidget : public ImageDisplayWidget
{
    Q_OBJECT
public:
    PBImageDisplayWidget( QWidget *parent = nullptr );
    
    /**
     * @brief Displays a cv::Mat image in the widget.
     * @param image OpenCV image to display (grayscale or BGR color)
     * @note Triggers automatic conversion and repaint
     */
    void Display( const cv::Mat &image );

protected:
    /**
     * @brief Handles paint events to render the image.
     * @param event Paint event (unused, can be nullptr)
     */
    void paintEvent( QPaintEvent * ) override;
    
    /**
     * @brief Handles resize events and recalculates scaling.
     * @param event Resize event containing new dimensions
     */
    void resizeEvent( QResizeEvent * ) override;

private:
    cv::Mat mCVImage;       ///< Stored OpenCV image for rendering

    QPainter mPainter;      ///< Qt painter for drawing operations

    quint8 muImageFormat{0};    ///< Image format identifier (grayscale=1, BGR=3, BGRA=4)
    qreal mrScale_x{1.};        ///< Horizontal scaling factor
    qreal mrScale_y{1.};        ///< Vertical scaling factor

    int miImageWidth{0};    ///< Original image width (pixels)
    int miImageHeight{0};   ///< Original image height (pixels)
};

