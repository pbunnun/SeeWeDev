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
 * @file CVMorphologicalTransformationModel.hpp
 * @brief Node model for morphological image operations
 * 
 * This file defines a node that applies morphological transformations to images.
 * Morphological operations are fundamental techniques in image processing for
 * analyzing and processing binary images and grayscale images based on shape.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include <opencv2/imgproc.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct MorphologicalTransformationParameters
 * @brief Parameter structure for morphological operations
 * 
 * Configures morphological transformations with operation type, structuring element,
 * and iteration count. Morphological operations use a structuring element (kernel)
 * to probe and modify image structures.
 * 
 * Common morphological operations:
 * - **Erosion**: Shrinks bright regions, removes small objects
 * - **Dilation**: Expands bright regions, fills small holes
 * - **Opening**: Erosion followed by dilation (removes noise, separates objects)
 * - **Closing**: Dilation followed by erosion (fills holes, connects nearby objects)
 * - **Gradient**: Difference between dilation and erosion (edge detection)
 * - **Top Hat**: Difference between source and opening (bright features)
 * - **Black Hat**: Difference between closing and source (dark features)
 */
typedef struct MorphologicalTransformationParameters{
    /** 
     * @brief Morphological operation type
     * @see cv::MorphTypes (MORPH_ERODE, MORPH_DILATE, MORPH_OPEN, MORPH_CLOSE, etc.)
     */
    int miMorphMethod;
    
    /** 
     * @brief Shape of the structuring element
     * @see cv::MorphShapes (MORPH_RECT, MORPH_ELLIPSE, MORPH_CROSS)
     */
    int miKernelShape;
    
    /** 
     * @brief Size of the structuring element kernel
     * @note Larger kernels = stronger effect on larger structures
     */
    cv::Size mCVSizeKernel;
    
    /** 
     * @brief Anchor position within the kernel (-1,-1 = center)
     * @note Usually left at (0,0) for auto-centering
     */
    cv::Point mCVPointAnchor;
    
    /** 
     * @brief Number of times to apply the operation
     * @note More iterations = stronger effect but slower processing
     */
    int miIteration;
    
    /** 
     * @brief Border extrapolation method for edge pixels
     * @see cv::BorderTypes
     */
    int miBorderType;
    
    /**
     * @brief Default constructor with opening operation
     */
    MorphologicalTransformationParameters()
        : miMorphMethod(cv::MORPH_OPEN),
          miKernelShape(cv::MORPH_RECT),
          mCVSizeKernel(cv::Size(3,3)),
          mCVPointAnchor(cv::Point(0,0)),
          miIteration(1),
          miBorderType(cv::BORDER_DEFAULT)
    {
    }
} MorphologicalTransformationParameters;

/**
 * @class CVMorphologicalTransformationModel
 * @brief Node model for morphological image transformations
 * 
 * This model applies morphological operations using OpenCV's cv::morphologyEx().
 * Morphological transformations are essential tools in image processing for:
 * - **Noise removal**: Opening removes small bright noise, closing removes dark noise
 * - **Shape analysis**: Skeleton extraction, boundary detection, connected components
 * - **Feature extraction**: Detecting edges, ridges, and other structural features
 * - **Preprocessing**: Cleaning binary masks before contour detection
 * - **Post-processing**: Filling holes in segmentation results
 * 
 * Operation details:
 * 
 * **Basic operations:**
 * - Erosion: Minimum filter - shrinks bright regions
 * - Dilation: Maximum filter - expands bright regions
 * 
 * **Compound operations:**
 * - Opening = Erosion → Dilation (breaks narrow isthmuses, removes small objects)
 * - Closing = Dilation → Erosion (fills narrow gaps, small holes)
 * 
 * **Advanced operations:**
 * - Morphological Gradient = Dilation - Erosion (outline of objects)
 * - Top Hat = Source - Opening (bright spots smaller than kernel)
 * - Black Hat = Closing - Source (dark spots smaller than kernel)
 * 
 * Common use cases:
 * - Cleaning up binary masks from thresholding
 * - Removing text/watermarks from images
 * - Separating touching objects (opening)
 * - Connecting broken characters (closing)
 * - Edge detection (gradient)
 * - Feature enhancement (top hat, black hat)
 * 
 * Input:
 * - Port 0: CVImageData - Source image (binary or grayscale)
 * 
 * Output:
 * - Port 0: CVImageData - Morphologically transformed image
 * 
 * Design Note: The structuring element shape affects the operation's behavior:
 * - Rectangular: Preserves horizontal/vertical structures
 * - Elliptical: Isotropic (rotationally symmetric)
 * - Cross: Emphasizes perpendicular structures
 * 
 * @note Works on both binary and grayscale images
 * @see cv::morphologyEx for the underlying OpenCV operation
 * @see cv::MorphTypes for available operations
 * @see cv::MorphShapes for structuring element shapes
 */
class CVMorphologicalTransformationModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a new morphological transformation node
     * 
     * Initializes with default opening operation using 3×3 rectangular kernel.
     */
    CVMorphologicalTransformationModel();

    /**
     * @brief Destructor
     */
    virtual
    ~CVMorphologicalTransformationModel() override {}

    /**
     * @brief Serializes the node state to JSON
     * 
     * Saves the current morphological operation parameters.
     * 
     * @return QJsonObject containing operation type, kernel shape/size, iterations
     */
    QJsonObject
    save() const override;

    /**
     * @brief Restores the node state from JSON
     * 
     * Loads previously saved morphological parameters.
     * 
     * @param p QJsonObject containing the saved node configuration
     */
    void
    load(const QJsonObject &p) override;

    /**
     * @brief Returns the number of ports for the given port type
     * 
     * This node has:
     * - 1 input port (source image)
     * - 1 output port (transformed image)
     * 
     * @param portType The type of port (In or Out)
     * @return Number of ports of the specified type
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port
     * 
     * All ports use CVImageData type.
     * 
     * @param portType The type of port (In or Out)
     * @param portIndex The index of the port
     * @return NodeDataType describing CVImageData
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Provides the morphologically transformed output
     * 
     * @param port The output port index (only 0 is valid)
     * @return Shared pointer to the transformed CVImageData
     * @note Returns nullptr if no input has been processed
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Receives and processes input image data
     * 
     * When image data arrives, this method:
     * 1. Validates the input data
     * 2. Creates structuring element from shape and size
     * 3. Applies cv::morphologyEx() with current parameters
     * 4. Stores the result for output
     * 5. Notifies connected nodes
     * 
     * @param nodeData The input CVImageData
     * @param portIndex The input port index (must be 0)
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief No embedded widget for this node
     * 
     * @return nullptr - Parameters are set via property browser
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets model properties from the property browser
     * 
     * Handles property changes for:
     * - "operation": Morphological operation type (enumeration)
     * - "kernel_shape": Structuring element shape (RECT, ELLIPSE, CROSS)
     * - "kernel_width": Width of structuring element (int, >= 1)
     * - "kernel_height": Height of structuring element (int, >= 1)
     * - "iterations": Number of times to apply operation (int, >= 1)
     * - "border_type": Edge pixel handling method (enumeration)
     * 
     * When properties change, the node automatically reprocesses current input.
     * 
     * @param property The name of the property being set
     * @param value The new value for the property
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Provides a thumbnail preview pixmap
     * 
     * @return QPixmap for node list/palette preview
     */
    QPixmap
    minPixmap() const override { return _minPixmap; }

    /** @brief Category name for node organization */
    static const QString _category;

    /** @brief Display name for the node type */
    static const QString _model_name;

private:
    /**
     * @brief Internal helper to perform morphological transformation
     * 
     * Executes the morphological operation:
     * 1. Creates structuring element using cv::getStructuringElement()
     * 2. Calls cv::morphologyEx() with the operation and kernel
     * 3. Applies the specified number of iterations
     * 
     * Why iterations matter:
     * - Single iteration: Standard morphological operation
     * - Multiple iterations: Stronger effect, reaches larger structures
     * - Trade-off: More iterations = slower but more thorough
     * 
     * @param in The input CVImageData to transform
     * @param out The output CVImageData to populate with result
     * @param params The morphological operation parameters
     * @note Uses cv::morphologyEx() internally
     * @see cv::morphologyEx
     * @see cv::getStructuringElement
     */
    void processData(const std::shared_ptr<CVImageData>& in, std::shared_ptr<CVImageData>& out, const MorphologicalTransformationParameters& params);

    /** @brief Current morphological operation parameters */
    MorphologicalTransformationParameters mParams;
    
    /** @brief Cached transformed output image */
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    
    /** @brief Cached input image data */
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    
    /** @brief Preview pixmap for node palette */
    QPixmap _minPixmap;
};


