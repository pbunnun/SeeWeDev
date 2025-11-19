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
 * @file CVDrawContourModel.hpp
 * @brief Contour visualization node for drawing detected contours on images.
 *
 * This node provides contour rendering capabilities by drawing contour boundaries
 * directly onto images. It accepts both an input image and contour data from
 * FindContourModel, enabling visual inspection and analysis of detected shapes.
 *
 * The node allows customization of drawing style through color (RGB), line thickness,
 * and line type parameters. This is essential for debugging contour detection pipelines
 * and creating annotated output for presentations or reports.
 *
 * @see FindContourModel for contour detection
 * @see FindAndCVDrawContourModel for combined detection and visualization
 * @see cv::drawContours OpenCV documentation
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <opencv2/imgproc.hpp>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "ContourPointsData.hpp"


using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVDrawContourParameters
 * @brief Configuration parameters for contour rendering.
 *
 * This structure encapsulates all visual parameters for drawing contours:
 * - **Color (RGB)**: Defines the rendering color in BGR format (OpenCV convention)
 *   * mucBValue: Blue channel (0-255)
 *   * mucGValue: Green channel (0-255, default 255 for bright green)
 *   * mucRValue: Red channel (0-255)
 * - **Line Thickness**: Controls contour boundary width in pixels (default: 2)
 *   * Positive values create lines of specified thickness
 *   * Negative values (e.g., -1) fill the contour interior
 * - **Line Type**: Specifies the line drawing algorithm
 *   * 0: 8-connected line (default, smoother)
 *   * 1: 4-connected line (faster but more jagged)
 *   * 16: Anti-aliased line (highest quality, slowest)
 *
 * **Design Rationale**:
 * Default green color (0, 255, 0) provides good contrast against typical grayscale
 * or natural images. The default thickness of 2 pixels balances visibility with
 * precision for most applications.
 *
 * **Usage Examples**:
 * - Object highlighting: Use filled contours (thickness = -1) with semi-transparent overlay
 * - Edge visualization: Use thin lines (thickness = 1) for precise boundary display
 * - Presentation graphics: Use anti-aliased lines (type = 16) for publication quality
 */
typedef struct CVDrawContourParameters{
    int mucBValue;       ///< Blue channel value (0-255) for contour color
    int mucGValue;       ///< Green channel value (0-255) for contour color
    int mucRValue;       ///< Red channel value (0-255) for contour color
    int miLineThickness; ///< Line thickness in pixels; -1 fills the contour
    int miLineType;      ///< Line drawing algorithm: 0=8-connected, 1=4-connected, 16=anti-aliased
    CVDrawContourParameters()
        : mucBValue(0),
          mucGValue(255),
          mucRValue(0),
          miLineThickness(2),
          miLineType(0)
    {
    }
} CVDrawContourParameters;

/**
 * @class CVDrawContourModel
 * @brief Node for visualizing contours by drawing them onto images.
 *
 * This visualization node renders contour boundaries detected by FindContourModel onto
 * input images, creating annotated output for analysis, debugging, or presentation purposes.
 * It provides flexible styling options for color, thickness, and line quality.
 *
 * **Functionality**:
 * - Accepts two inputs: image data and contour points data
 * - Draws all contours from the contour data onto the image
 * - Customizable color via RGB channel values (BGR format internally)
 * - Adjustable line thickness (positive for outline, negative for fill)
 * - Selectable line type (8-connected, 4-connected, or anti-aliased)
 *
 * **Input Ports**:
 * - Port 0: CVImageData - Base image to draw contours on (typically original or preprocessed)
 * - Port 1: ContourPointsData - Contour data from FindContourModel or similar
 *
 * **Output Port**:
 * - Port 0: CVImageData - Annotated image with drawn contours
 *
 * **Drawing Algorithm**:
 * 1. Clone input image to preserve original
 * 2. Extract contours vector from ContourPointsData
 * 3. Call cv::drawContours with all contours (index=-1)
 * 4. Apply specified color (BGR), thickness, and line type
 * 5. Output annotated image
 *
 * **Common Use Cases**:
 * - **Visual Debugging**: Verify contour detection accuracy before further processing
 * - **Quality Inspection**: Highlight detected defects or features for human review
 * - **Data Annotation**: Create labeled datasets for machine learning
 * - **Report Generation**: Produce annotated images for documentation or presentations
 * - **Real-time Monitoring**: Display detected objects/regions in surveillance or manufacturing
 *
 * **Typical Pipeline**:
 * ImageSource → Preprocessing → FindContour → **CVDrawContour** → Display/Save
 * - Alternative: Use FindAndCVDrawContourModel for combined detection+visualization
 *
 * **Performance Notes**:
 * - Anti-aliased lines (type=16) provide best quality but are ~3-4x slower
 * - Drawing is fast (~1ms for typical contours) but scales with contour count
 * - For real-time applications, prefer 8-connected lines (type=0)
 *
 * **Design Decision**:
 * Separate drawing node allows reuse of contour data for multiple visualizations
 * (e.g., different colors for different analysis views) without re-detecting contours.
 *
 * @see FindContourModel for contour detection
 * @see ContourPointsData for contour data structure
 * @see cv::drawContours for OpenCV drawing implementation
 */
class CVDrawContourModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVDrawContourModel with default green contours.
     */
    CVDrawContourModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVDrawContourModel() override {}

    /**
     * @brief Serializes model parameters to JSON.
     * @return QJsonObject containing drawing parameters (color, thickness, line type)
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model parameters from JSON.
     * @param p JSON object with saved parameters
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports for the specified type.
     * @param portType Input or Output
     * @return 2 for Input (image + contours), 1 for Output (annotated image)
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for the specified port.
     * @param portType Input or Output
     * @param portIndex Port index
     * @return CVImageData for ports 0 (input/output), ContourPointsData for input port 1
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the output data (annotated image).
     * @param port Output port index (0)
     * @return Shared pointer to CVImageData with drawn contours
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and triggers contour drawing.
     * @param data Input image (port 0) or contour points (port 1)
     * @param portIndex Input port index
     *
     * Processes data when both inputs are available, drawing all contours onto the image.
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    /**
     * @brief No embedded widget for this node.
     * @return nullptr
     */
    QWidget *
    embeddedWidget() override {return nullptr;}

    /**
     * @brief Updates drawing parameters from the property browser.
     * @param property Property name (e.g., "b_value", "g_value", "r_value", "line_thickness", "line_type")
     * @param value New property value
     *
     * Automatically triggers re-drawing when parameters change.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Returns the minimized pixmap icon for the node.
     * @return QPixmap representing the node in minimized state
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;    ///< Node category: "Image Processing"
    static const QString _model_name;  ///< Unique model name: "Draw Contour"

private:
    CVDrawContourParameters mParams;                              ///< Current drawing parameters (color, thickness, line type)
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };   ///< Input image data
    std::shared_ptr<CVImageData> mpCVImageOutData { nullptr };  ///< Output image data with drawn contours
    std::shared_ptr<ContourPointsData> mpContourPointsData {nullptr}; ///< Input contour points data
    QPixmap _minPixmap;                                         ///< Minimized node icon

    /**
     * @brief Processes data by drawing contours onto the image.
     * @param in Input image
     * @param outImage Output image with drawn contours
     * @param ctrPnts Contour points to draw
     * @param params Drawing parameters (color, thickness, line type)
     *
     * **Algorithm**:
     * 1. Clone input image: `outImage = in.clone()`
     * 2. Extract contours: `contours = ctrPnts.getContours()`
     * 3. Create color scalar: `color = cv::Scalar(B, G, R)`
     * 4. Draw all contours: `cv::drawContours(outImage, contours, -1, color, thickness, lineType)`
     * 5. Return annotated image
     *
     * The index parameter -1 in cv::drawContours means "draw all contours".
     * For selective drawing, use FindAndCVDrawContourModel with filtering options.
     */
    void processData(const std::shared_ptr<CVImageData>& in, std::shared_ptr<CVImageData>& outImage,
                     std::shared_ptr<ContourPointsData> &ctrPnts, const CVDrawContourParameters& params);
};

