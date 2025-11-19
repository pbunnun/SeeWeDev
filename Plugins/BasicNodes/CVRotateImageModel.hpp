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
 * @file CVRotateImageModel.hpp
 * @brief Image rotation node for arbitrary angle transformations.
 *
 * This node provides image rotation capabilities around the image center using
 * affine transformations. It supports arbitrary rotation angles (in degrees) and
 * automatically handles boundary conditions to prevent clipping.
 *
 * The rotation uses OpenCV's getRotationMatrix2D and warpAffine functions,
 * which provide smooth interpolation and proper handling of edge cases.
 *
 * **Key Features**:
 * - Arbitrary rotation angles (0-360 degrees)
 * - Center-point rotation (rotates around image center)
 * - Automatic boundary handling
 * - Smooth interpolation for sub-pixel accuracy
 *
 * @note Rotation may change the output image dimensions if the rotated content
 *       extends beyond original boundaries. This implementation preserves the
 *       original dimensions, potentially cropping rotated corners.
 *
 * @see cv::getRotationMatrix2D for rotation matrix computation
 * @see cv::warpAffine for affine transformation application
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QSpinBox>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVRotateImageModel
 * @brief Node for rotating images by arbitrary angles around the center point.
 *
 * This transformation node rotates input images by a specified angle (in degrees)
 * around the image center using affine transformation. It's commonly used for:
 * - Image alignment and registration
 * - Data augmentation in machine learning pipelines
 * - Correcting skewed or tilted captures
 * - Creating rotational invariance in feature detection
 *
 * **Rotation Algorithm**:
 * 1. Compute image center: `center = (width/2, height/2)`
 * 2. Generate rotation matrix: `M = cv::getRotationMatrix2D(center, angle, scale=1.0)`
 * 3. Apply affine transformation: `cv::warpAffine(input, output, M, size)`
 *
 * The rotation matrix M is a 2x3 affine transformation matrix:
 * \f[
 * M = \begin{bmatrix}
 * \alpha & \beta & (1-\alpha) \cdot center.x - \beta \cdot center.y \\
 * -\beta & \alpha & \beta \cdot center.x + (1-\alpha) \cdot center.y
 * \end{bmatrix}
 * \f]
 * where:
 * - \f$ \alpha = scale \cdot \cos(angle) \f$
 * - \f$ \beta = scale \cdot \sin(angle) \f$
 * - scale = 1.0 (no scaling, pure rotation)
 *
 * **Input Port**:
 * - Port 0: CVImageData - Image to rotate
 *
 * **Output Port**:
 * - Port 0: CVImageData - Rotated image
 *
 * **Parameters**:
 * - **Angle**: Rotation angle in degrees (default: 180.0)
 *   * Positive values rotate counter-clockwise
 *   * Negative values rotate clockwise
 *   * Common values: 90, 180, 270 for orthogonal rotations
 *
 * **Boundary Handling**:
 * This implementation preserves original image dimensions. Content extending beyond
 * boundaries after rotation will be cropped. For non-orthogonal angles (not 90°
 * multiples), corners may be cut off.
 *
 * **Performance Considerations**:
 * - Orthogonal rotations (90°, 180°, 270°) can be optimized using cv::rotate
 * - Arbitrary angles require interpolation: ~2-3ms for 640x480 images
 * - For real-time applications, consider caching rotation matrices if angle is constant
 *
 * **Common Use Cases**:
 * - **Document Processing**: Correct skewed scanned documents
 * - **Data Augmentation**: Generate rotated training samples for ML models
 * - **Camera Alignment**: Fix camera mounting orientation
 * - **Object Recognition**: Test rotational invariance of feature detectors
 * - **Artistic Effects**: Create dynamic visual presentations
 *
 * **Design Decision**:
 * Default angle of 180° is chosen as it's a common operation for flipping
 * upside-down images without requiring separate horizontal/vertical flip nodes.
 *
 * @note For 90° rotations, consider using cv::rotate (ROTATE_90_CLOCKWISE) for
 *       better performance and exact results without interpolation artifacts.
 *
 * @see cv::getRotationMatrix2D for rotation matrix generation
 * @see cv::warpAffine for affine transformation
 * @see cv::rotate for optimized orthogonal rotations
 */
class CVRotateImageModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVRotateImageModel with default 180° rotation.
     */
    CVRotateImageModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVRotateImageModel() override {}

    /**
     * @brief Serializes model parameters to JSON.
     * @return QJsonObject containing rotation angle
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
     * @return 1 for both Input and Output
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for the specified port.
     * @param portType Input or Output
     * @param portIndex Port index (0)
     * @return CVImageData for both input and output
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the output data (rotated image).
     * @param port Output port index (0)
     * @return Shared pointer to CVImageData containing rotated image
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input image data and triggers rotation processing.
     * @param data Input image data
     * @param portIndex Input port index (0)
     */
    void
    setInData(std::shared_ptr<NodeData>, PortIndex) override;

    /**
     * @brief No embedded widget for this node.
     * @return nullptr
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Updates rotation angle from the property browser.
     * @param property Property name (e.g., "angle")
     * @param value New property value (rotation angle in degrees)
     *
     * Automatically triggers image re-rotation when angle changes.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;    ///< Node category: "Image Processing"
    static const QString _model_name;  ///< Unique model name: "Rotate Image"

private:
    /**
     * @brief Processes image rotation using affine transformation.
     * @param in Input image data
     * @param out Output image data (rotated)
     *
     * **Algorithm Steps**:
     * 1. Get input image: `cv::Mat inputImage = in->getData()`
     * 2. Compute center point: `cv::Point2f center(width/2.0, height/2.0)`
     * 3. Generate rotation matrix: `cv::Mat M = cv::getRotationMatrix2D(center, mdAngle, 1.0)`
     * 4. Apply transformation: `cv::warpAffine(inputImage, outputImage, M, inputImage.size())`
     * 5. Store result in out
     *
     * The scale parameter is fixed at 1.0 (no scaling), providing pure rotation.
     * Interpolation method is INTER_LINEAR by default, offering good balance
     * between quality and performance.
     */
    void processData( const std::shared_ptr<CVImageData> & in, std::shared_ptr<CVImageData> & out );
    
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };  ///< Input image data
    std::shared_ptr<CVImageData> mpCVImageOutData { nullptr }; ///< Output rotated image data

    double mdAngle { 180.f }; ///< Rotation angle in degrees (positive = counter-clockwise)
};

