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
 * @file CVColorMapModel.hpp
 * @brief Pseudo-coloring node for mapping grayscale intensities to color palettes.
 *
 * This node applies false-color mapping (pseudo-coloring) to grayscale images by
 * converting intensity values to color representations using predefined color maps.
 * This technique enhances visual perception of intensity variations and is widely
 * used in scientific visualization, thermal imaging, and data analysis.
 *
 * The node uses OpenCV's applyCVColorMap function with various built-in palettes
 * (JET, HOT, RAINBOW, etc.), transforming single-channel intensity data into
 * visually interpretable RGB representations.
 *
 * **Key Applications**:
 * - Thermal/infrared image visualization
 * - Depth map visualization
 * - Heatmap generation
 * - Medical imaging (X-ray, MRI intensity mapping)
 * - Scientific data visualization
 *
 * @see cv::applyCVColorMap for color mapping implementation
 * @see CVColorMaps enum for available palettes
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "SyncData.hpp"

#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVColorMapParameters
 * @brief Configuration for color map selection.
 *
 * This structure specifies which color palette to apply when converting grayscale
 * intensities to pseudo-colors.
 *
 * **Available Color Maps** (cv::ColormapTypes):
 * - **COLORMAP_AUTUMN** (0): Red-orange-yellow progression, warm tones
 * - **COLORMAP_BONE** (1): Grayscale with blue tint, suitable for medical imaging
 * - **COLORMAP_JET** (2): Blue→cyan→yellow→red (default), classic rainbow spectrum
 * - **COLORMAP_WINTER** (3): Blue to green progression, cool tones
 * - **COLORMAP_RAINBOW** (4): Spectral colors from violet to red
 * - **COLORMAP_OCEAN** (5): Deep blue to cyan, suitable for depth maps
 * - **COLORMAP_SUMMER** (6): Green to yellow progression
 * - **COLORMAP_SPRING** (7): Magenta to yellow progression
 * - **COLORMAP_COOL** (8): Cyan to magenta progression
 * - **COLORMAP_HSV** (9): Full hue spectrum, high saturation
 * - **COLORMAP_PINK** (10): Pastel tones, reduced saturation
 * - **COLORMAP_HOT** (11): Black→red→yellow→white, thermal imaging standard
 * - **COLORMAP_PARULA** (12): Perceptually uniform, modern default in MATLAB
 * - **COLORMAP_MAGMA** (13): Perceptually uniform, black→purple→red→yellow
 * - **COLORMAP_INFERNO** (14): Perceptually uniform, black→red→yellow
 * - **COLORMAP_PLASMA** (15): Perceptually uniform, purple→pink→yellow
 * - **COLORMAP_VIRIDIS** (16): Perceptually uniform, purple→green→yellow
 * - **COLORMAP_CIVIDIS** (17): Color-blind friendly, blue→yellow
 * - **COLORMAP_TWILIGHT** (18): Cyclic, suitable for angular data
 * - **COLORMAP_TWILIGHT_SHIFTED** (19): Cyclic with shifted range
 * - **COLORMAP_TURBO** (20): Improved JET, better perceptual uniformity
 * - **COLORMAP_DEEPGREEN** (21): Dark to bright green progression
 *
 * **Choosing a Color Map**:
 * - **Scientific Visualization**: Use perceptually uniform maps (VIRIDIS, MAGMA, PLASMA)
 *   to avoid false gradients and ensure color intensity matches data intensity
 * - **Thermal Imaging**: HOT or JET for traditional thermal camera appearance
 * - **Depth Maps**: OCEAN or JET for intuitive near-far representation
 * - **Medical Imaging**: BONE for X-ray-like appearance, or VIRIDIS for quantitative analysis
 * - **Presentations**: TURBO (improved JET) for vibrant, easily distinguishable colors
 * - **Color-Blind Users**: CIVIDIS for accessibility
 *
 * **Default: COLORMAP_JET**
 * The classic rainbow palette providing good visual separation across intensity ranges,
 * though not perceptually uniform. For new projects, consider VIRIDIS or TURBO.
 */
typedef struct CVColorMapParameters{
    int miCVColorMap;  ///< Color map type (cv::ColormapTypes enum value, default: COLORMAP_JET)
    CVColorMapParameters()
        : miCVColorMap(cv::COLORMAP_JET)
    {
    }
} CVColorMapParameters;

/**
 * @class CVColorMapModel
 * @brief Applies pseudo-coloring to grayscale images using predefined color palettes.
 *
 * This visualization node transforms grayscale intensity values into color-coded
 * representations, making subtle intensity differences more visually apparent to
 * human perception. It's essential for scientific visualization, thermal imaging,
 * and any application where quantitative data needs intuitive visual interpretation.
 *
 * **Functionality**:
 * - Accepts grayscale (single-channel) images as input
 * - Applies selected color map using cv::applyCVColorMap
 * - Outputs 3-channel BGR color image
 * - Optional sync data input for pipeline synchronization
 *
 * **Input Ports**:
 * - Port 0: CVImageData - Grayscale image (8-bit single channel)
 * - Port 1: SyncData - Optional synchronization signal
 *
 * **Output Port**:
 * - Port 0: CVImageData - Pseudo-colored 3-channel BGR image
 *
 * **Processing Algorithm**:
 * 1. Validate input is 8-bit single-channel (CV_8UC1)
 * 2. Apply color map: `cv::applyCVColorMap(inputGray, outputColor, colorMapType)`
 * 3. Output mapped BGR image
 *
 * **Color Mapping Function**:
 * For each pixel with intensity value I (0-255):
 * \f[
 * RGB(I) = CVColorMap[I]
 * \f]
 * where CVColorMap is a lookup table mapping intensity → (B, G, R) triplet.
 *
 * **Common Use Cases**:
 * - **Thermal Imaging**: Visualize temperature distributions with HOT or JET maps
 * - **Depth Visualization**: Convert depth maps to intuitive near-far colors (OCEAN, JET)
 * - **Heatmaps**: Display intensity/frequency distributions (VIRIDIS, TURBO)
 * - **Medical Imaging**: Enhance visibility of subtle structures (BONE for X-ray)
 * - **Scientific Data**: Publication-quality perceptually uniform visualizations (VIRIDIS, MAGMA)
 * - **Quality Inspection**: Highlight variations in surface measurements
 *
 * **Typical Pipelines**:
 * - Depth camera → **CVColorMap** → Display (depth visualization)
 * - Grayscale → Threshold → DistanceTransform → **CVColorMap** → Display (distance heatmap)
 * - Thermal sensor → Normalize → **CVColorMap** → Display (thermal visualization)
 * - DNN activation → **CVColorMap** → Display (neural network visualization)
 *
 * **Perceptual Uniformity**:
 * Not all color maps are created equal:
 * - **Perceptually Uniform** (VIRIDIS, MAGMA, PLASMA, INFERNO, CIVIDIS):
 *   Equal steps in data value produce equal perceptual color changes. Recommended
 *   for scientific visualization to avoid false gradients.
 * - **Rainbow/Spectral** (JET, RAINBOW, TURBO):
 *   Vibrant and familiar but can create false edges where data is smooth. Use
 *   for presentations or when tradition dictates (thermal imaging).
 * - **Sequential** (HOT, BONE, OCEAN):
 *   Single hue progression, good for showing monotonic data trends.
 *
 * **Performance Notes**:
 * - Color mapping is very fast (~0.5ms for 640x480 images) as it's a simple LUT operation
 * - No computational overhead compared to manual color mapping
 * - Real-time capable even for high-resolution images
 *
 * **Design Decision**:
 * Default JET color map is chosen for familiarity and vibrant appearance, despite
 * not being perceptually uniform. Users doing quantitative analysis should select
 * VIRIDIS or TURBO for more accurate visual interpretation.
 *
 * @see cv::applyCVColorMap for mapping implementation
 * @see cv::ColormapTypes for complete color map list
 * @see https://matplotlib.org/stable/tutorials/colors/colormaps.html for color map theory
 */
class CVColorMapModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVColorMapModel with JET color map as default.
     */
    CVColorMapModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVColorMapModel() override {}

    /**
     * @brief Serializes model parameters to JSON.
     * @return QJsonObject containing color map selection
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model parameters from JSON.
     * @param p JSON object with saved parameters
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the specified type.
     * @param portType Input or Output
     * @return 2 for Input (image + sync), 1 for Output (colored image)
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for the specified port.
     * @param portType Input or Output
     * @param portIndex Port index
     * @return CVImageData for ports 0, SyncData for input port 1
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the output data (pseudo-colored image).
     * @param port Output port index (0)
     * @return Shared pointer to CVImageData with color-mapped image
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and triggers color mapping.
     * @param nodeData Input image (port 0) or sync signal (port 1)
     * @param portIndex Input port index
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief No embedded widget for this node.
     * @return nullptr
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Updates color map selection from the property browser.
     * @param property Property name (e.g., "color_map")
     * @param value New color map type (cv::ColormapTypes enum value)
     *
     * Automatically triggers re-mapping when color map changes.
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
    static const QString _model_name;  ///< Unique model name: "Color Map"

private:
    CVColorMapParameters mParams;                                 ///< Current color map selection
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };   ///< Input grayscale image
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };     ///< Output pseudo-colored image
    std::shared_ptr<SyncData> mpSyncData {nullptr};             ///< Optional synchronization signal
    QPixmap _minPixmap;                                         ///< Minimized node icon

    /**
     * @brief Processes data by applying the selected color map.
     * @param in Input grayscale image (must be CV_8UC1)
     * @param out Output pseudo-colored image (CV_8UC3, BGR format)
     * @param params Color map parameters (color map type selection)
     *
     * **Algorithm**:
     * ```cpp
     * // Validate input
     * cv::Mat inputGray = in->getData();
     * assert(inputGray.type() == CV_8UC1);  // Must be 8-bit grayscale
     * 
     * // Apply color map
     * cv::Mat outputColor;
     * cv::applyCVColorMap(inputGray, outputColor, params.miCVColorMap);
     * 
     * // Store result
     * out->setData(outputColor);
     * ```
     *
     * The cv::applyCVColorMap function uses a 256-entry lookup table for each
     * color map, making the operation extremely fast and suitable for real-time
     * visualization.
     *
     * **Input Requirements**:
     * - Image must be 8-bit single-channel (CV_8UC1)
     * - Intensity range 0-255 (full dynamic range recommended)
     * - If input has different range, normalize first using cv::normalize
     *
     * **Output Format**:
     * - 3-channel BGR image (CV_8UC3)
     * - Same dimensions as input
     * - Full color depth utilizing entire palette
     */
    void processData( const std::shared_ptr<CVImageData> &in, std::shared_ptr< CVImageData > & out,
                      const CVColorMapParameters & params );

    QPixmap _minpixmap;
};

