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
 * @file CVSobelAndScharrModel.hpp
 * @brief Model for Sobel and Scharr gradient-based edge detection.
 *
 * This file defines the CVSobelAndScharrModel class for computing image gradients using
 * Sobel or Scharr operators. It supports separate X and Y derivative computation with
 * optional combined magnitude output, configurable kernel sizes, scaling, and border
 * handling. The model is fundamental for edge detection, feature extraction, and
 * image analysis tasks.
 *
 * @note Bug warning: Empty Sobel output connected to Gaussian Blur Node may cause issues.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include <opencv2/imgproc.hpp>
#include "CVSobelAndScharrEmbeddedWidget.hpp"


using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVSobelAndScharrParameters
 * @brief Configuration parameters for Sobel/Scharr gradient filters.
 *
 * Stores all settings for gradient computation including derivative orders,
 * kernel size, scaling factors, and border handling.
 */
typedef struct CVSobelAndScharrParameters{
    int miOrderX;       ///< Order of X derivative (0 = no X derivative, 1 = first derivative, 2 = second)
    int miOrderY;       ///< Order of Y derivative (0 = no Y derivative, 1 = first derivative, 2 = second)
    int miKernelSize;   ///< Kernel size (1, 3, 5, 7, ... or -1 for Scharr)
    double mdScale;     ///< Scale factor multiplied to computed derivatives
    double mdDelta;     ///< Optional delta added to results
    int miBorderType;   ///< Border extrapolation method (cv::BorderTypes)
    
    /**
     * @brief Default constructor.
     *
     * Initializes with standard first-order gradients (∂/∂x and ∂/∂y),
     * 3×3 kernel, no scaling, and default border handling.
     */
    CVSobelAndScharrParameters()
        : miOrderX(1),
          miOrderY(1),
          miKernelSize(3),
          mdScale(1),
          mdDelta(0),
          miBorderType(cv::BORDER_DEFAULT)
    {
    }
} CVSobelAndScharrParameters;

/**
 * @class CVSobelAndScharrModel
 * @brief Node model for computing image gradients using Sobel or Scharr operators.
 *
 * This model computes first and second-order image derivatives using Sobel or Scharr
 * filters, which are essential for edge detection, gradient magnitude/orientation
 * computation, and feature extraction. It outputs separate X and Y gradients plus an
 * optional combined magnitude image.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Input image (grayscale recommended, color supported)
 *
 * **Output Ports:**
 * 1. **CVImageData** - X gradient (∂I/∂x)
 * 2. **CVImageData** - Y gradient (∂I/∂y)
 * 3. **CVImageData** - Combined gradient magnitude: √(∂I/∂x)² + (∂I/∂y)²
 *
 * **Gradient Computation:**
 *
 * The Sobel operator convolves the image with separable kernels:
 * - **X-direction (horizontal edges):**
 *   \f[
 *   G_x = \begin{bmatrix} -1 & 0 & +1 \\ -2 & 0 & +2 \\ -1 & 0 & +1 \end{bmatrix} * I
 *   \f]
 * - **Y-direction (vertical edges):**
 *   \f[
 *   G_y = \begin{bmatrix} -1 & -2 & -1 \\ 0 & 0 & 0 \\ +1 & +2 & +1 \end{bmatrix} * I
 *   \f]
 *
 * **Magnitude Computation:**
 * \f[
 * |G| = \sqrt{G_x^2 + G_y^2}
 * \f]
 *
 * **Scharr vs Sobel:**
 * - **Sobel:** Flexible kernel sizes (3, 5, 7, ...), good general-purpose gradient
 * - **Scharr:** Optimized 3×3 kernel with coefficients [-3, 0, 3; -10, 0, 10; -3, 0, 3]
 *   - Better rotational symmetry and gradient accuracy
 *   - Only supports 3×3 (miKernelSize = -1 or 3)
 *
 * **Derivative Orders:**
 * - **First-order (miOrderX=1, miOrderY=1):** Standard edge detection
 *   - Detects intensity changes
 * - **Second-order (miOrderX=2 or miOrderY=2):** Laplacian-like behavior
 *   - Detects zero-crossings and fine details
 * - **Mixed orders:** e.g., (1, 0) for horizontal edges only
 *
 * **Kernel Sizes:**
 * - 1: Very small, minimal smoothing
 * - 3: Standard, good balance (default)
 * - 5, 7, ...: Larger kernels, more smoothing, less noise sensitivity
 * - -1: Special value for Scharr filter (3×3 optimized)
 *
 * **Scale and Delta:**
 * - **mdScale:** Multiplies gradient values (useful for visualization or normalization)
 *   - Example: scale=0.5 to reduce gradient magnitude
 * - **mdDelta:** Adds offset to results (shifts intensity range)
 *   - Example: delta=128 to center values around mid-gray
 *
 * **Border Handling:**
 * Border extrapolation methods for edge pixels:
 * - **cv::BORDER_DEFAULT (REFLECT_101):** Reflect with adjustment for edge pixels
 * - **cv::BORDER_CONSTANT:** Fill with constant value (black)
 * - **cv::BORDER_REPLICATE:** Repeat edge pixels
 * - **cv::BORDER_REFLECT:** Reflect without adjustment
 * - **cv::BORDER_WRAP:** Wrap around to opposite edge
 *
 * **Properties (Configurable):**
 * - **order_x:** X derivative order (0, 1, 2)
 * - **order_y:** Y derivative order (0, 1, 2)
 * - **kernel_size:** Aperture size (1, 3, 5, 7, ... or -1 for Scharr)
 * - **scale:** Gradient scale multiplier
 * - **delta:** Added offset value
 * - **border_type:** Border extrapolation method
 * - **use_scharr:** Boolean to enable Scharr mode (via embedded widget)
 *
 * **Use Cases:**
 * - Edge detection (magnitude output)
 * - Gradient orientation computation (atan2(Gy, Gx))
 * - Feature extraction for SIFT, SURF, HOG
 * - Preprocessing for Canny edge detector
 * - Texture analysis
 * - Optical flow computation
 * - Image sharpening (add gradient to original)
 * - Embossing effects
 *
 * **Example Workflows:**
 * @code
 * // Standard edge detection
 * [Camera] -> [Grayscale] -> [SobelScharr] -> [Display Magnitude]
 *                               OrderX=1, OrderY=1, Kernel=3
 * 
 * // Horizontal edges only
 * [Image] -> [SobelScharr] -> [Display X-gradient]
 *              OrderX=1, OrderY=0
 * 
 * // High-accuracy gradients with Scharr
 * [Image] -> [SobelScharr: Use Scharr=checked] -> [GradientMagnitude]
 * 
 * // Second derivative for feature detection
 * [Image] -> [SobelScharr: OrderX=2, OrderY=0] -> [ZeroCrossing]
 * @endcode
 *
 * **Output Interpretation:**
 * - Port 0 (X-gradient): Responds to vertical edges (left-right intensity changes)
 * - Port 1 (Y-gradient): Responds to horizontal edges (top-bottom intensity changes)
 * - Port 2 (Magnitude): Combined edge strength regardless of orientation
 *
 * **Performance Notes:**
 * - Larger kernels slower but smoother
 * - Scharr slightly slower than Sobel 3×3 but more accurate
 * - Second derivatives more noise-sensitive; consider pre-smoothing
 *
 * **Typical Parameter Settings:**
 * - General edge detection: OrderX=1, OrderY=1, Kernel=3
 * - High accuracy: Use Scharr, OrderX=1, OrderY=1
 * - Noise reduction: Kernel=5 or 7
 * - Emboss effect: OrderX=1, OrderY=1, Scale=1, Delta=128
 *
 * @see CVSobelAndScharrEmbeddedWidget
 * @see cv::Sobel
 * @see cv::Scharr
 * @see CannyEdgeModel
 */
class CVSobelAndScharrModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVSobelAndScharrModel.
     *
     * Initializes with default parameters (first-order X and Y derivatives, 3×3 kernel)
     * and creates the embedded widget for Scharr mode selection.
     */
    CVSobelAndScharrModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVSobelAndScharrModel() override {}

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing all parameters and Scharr checkbox state.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved parameters.
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 1 for input (source image), 3 for output (X, Y, magnitude gradients).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return CVImageData for all ports.
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the computed gradient data.
     * @param port Port index (0=X-gradient, 1=Y-gradient, 2=magnitude).
     * @return Shared pointer to gradient CVImageData.
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input image data and triggers gradient computation.
     * @param nodeData Input CVImageData.
     * @param Port index (0).
     *
     * When a new image is received, computes Sobel/Scharr gradients according
     * to current parameters and updates all output ports.
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief Returns the embedded Scharr checkbox widget.
     * @return Pointer to CVSobelAndScharrEmbeddedWidget.
     */
    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    /**
     * @brief Sets a model property.
     * @param Property name ("order_x", "order_y", "kernel_size", "scale", "delta", "border_type").
     * @param QVariant value.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Returns the minimum node icon.
     * @return QPixmap icon.
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private Q_SLOTS:

    /**
     * @brief Slot for Scharr checkbox state changes.
     * @param State value (Qt::Checked or Qt::Unchecked as int).
     *
     * When Scharr mode is enabled, forces kernel size to 3×3 (or -1) and
     * recomputes gradients using cv::Scharr() instead of cv::Sobel().
     */
    void em_checkbox_checked(int);

private:
    CVSobelAndScharrParameters mParams;                       ///< Gradient computation parameters
    std::shared_ptr<CVImageData> mapCVImageData[3] {{ nullptr }}; ///< Output gradients [X, Y, magnitude]
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr }; ///< Input image
    CVSobelAndScharrEmbeddedWidget* mpEmbeddedWidget;         ///< Scharr mode checkbox widget
    QPixmap _minPixmap;                                     ///< Node icon

    /**
     * @brief Processes input image and computes gradients.
     * @param in Input CVImageData (source image).
     * @param out Array of 3 output CVImageData pointers (X, Y, magnitude).
     * @param params Gradient computation parameters.
     *
     * Executes cv::Sobel() or cv::Scharr() based on checkbox state, computes
     * X and Y gradients, calculates magnitude, and populates output ports.
     *
     * **Implementation Notes:**
     * - Converts color images to grayscale internally
     * - Uses CV_16S or CV_32F output depth for signed gradients
     * - Computes magnitude using cv::magnitude() or manual sqrt(Gx² + Gy²)
     * - Applies scale and delta to all outputs
     */
    void processData(const std::shared_ptr<CVImageData> &in, std::shared_ptr<CVImageData> (&out)[3],
                     const CVSobelAndScharrParameters &params);
};


