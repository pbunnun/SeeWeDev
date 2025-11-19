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
 * @file CVImageROIModel.hpp
 * @brief Interactive Region of Interest (ROI) selection and cropping node.
 *
 * This file defines a node for selecting rectangular regions of interest from images
 * with interactive visualization and Apply/Reset controls. The node supports both
 * manual ROI definition via properties and interactive drawing via external ROI tools.
 *
 * **Key Features:**
 * - Interactive ROI rectangle selection with visual feedback
 * - Apply/Reset workflow for confirming or discarding selections
 * - Dual output: Annotated full image + Cropped ROI region
 * - Lock mode to prevent ROI changes during processing
 * - Optional ROI rectangle visualization overlay
 *
 * **Typical ROI Selection Workflow:**
 * ```
 * 1. Connect image input
 * 2. User draws rectangle on display (via interactive graphics view)
 * 3. Node shows preview with ROI rectangle overlay
 * 4. Click Apply → Output switches to cropped region
 * 5. Process cropped region in pipeline
 * 6. Click Reset → Return to full image
 * ```
 *
 * **Common Applications:**
 * - Focus processing on specific image regions (e.g., license plate, face)
 * - Reduce computational load by processing only relevant areas
 * - Multi-ROI workflows (crop → process → recombine)
 * - Interactive annotation and labeling
 * - Exclude irrelevant background from analysis
 *
 * @see CVImageROI for the original implementation
 * @see CVImageROIEmbeddedWidget for Apply/Reset controls
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVImageROIEmbeddedWidget.hpp"
#include <opencv2/highgui.hpp>
#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct CVImageROIParameters
 * @brief ROI rectangle coordinates and visualization settings.
 *
 * This structure defines the rectangular ROI region and how it's displayed.
 *
 * **ROI Definition:**
 * - **mCVPointRect1**: Top-left corner (x1, y1)
 * - **mCVPointRect2**: Bottom-right corner (x2, y2)
 * - ROI rectangle is [x1:x2, y1:y2] (inclusive)
 *
 * **Visualization:**
 * - **mbDisplayLines**: Show/hide ROI rectangle overlay
 * - **mucLineColor**: RGB color for rectangle outline [0-255]
 * - **miLineThickness**: Rectangle border width (pixels)
 *
 * **Lock Mode:**
 * - **mbLockOutputROI**: When true, prevents ROI changes
 *   - Useful for batch processing with fixed ROI
 *   - Disables Apply/Reset buttons
 *   - Ensures consistent region across multiple frames
 *
 * @note Coordinates must satisfy: x2 > x1 and y2 > y1 for valid ROI
 */

typedef struct CVImageROIParameters
{
    cv::Point mCVPointRect1;        ///< Top-left corner of ROI rectangle
    cv::Point mCVPointRect2;        ///< Bottom-right corner of ROI rectangle
    int mucLineColor[3];            ///< RGB color for ROI rectangle overlay [0-255]
    int miLineThickness;            ///< Rectangle border thickness (pixels)
    bool mbDisplayLines;            ///< Whether to draw ROI rectangle on output
    bool mbLockOutputROI;           ///< Lock ROI to prevent changes (batch processing mode)
    CVImageROIParameters()
        : mCVPointRect1(cv::Point(0,0)),
          mCVPointRect2(cv::Point(0,0)),
          mucLineColor{0},
          miLineThickness(2),
          mbDisplayLines(true),
          mbLockOutputROI(false)
    {
    }
} CVImageROIParameters;

/**
 * @struct CVImageROIProperties
 * @brief State flags for ROI workflow control.
 *
 * These boolean flags track the ROI selection state and user actions.
 *
 * **State Flags:**
 * - **mbReset**: Set to true when user clicks Reset button
 *   - Clears ROI selection
 *   - Reverts output to full image
 *   - Resets rectangle to (0,0)-(0,0)
 *
 * - **mbApply**: Set to true when user clicks Apply button
 *   - Confirms current ROI selection
 *   - Switches output to cropped region
 *   - Locks ROI for processing
 *
 * - **mbNewMat**: Indicates whether a new image requires ROI initialization
 *   - true: Fresh image, no ROI applied yet
 *   - false: ROI already applied to current image
 *
 * **Workflow State Machine:**
 * ```
 * Initial: mbNewMat=true, mbApply=false, mbReset=false
 *   → User draws ROI
 *   → User clicks Apply: mbApply=true, mbNewMat=false
 *   → Processing with cropped ROI
 *   → User clicks Reset: mbReset=true
 *   → Back to Initial state
 * ```
 */
typedef struct CVImageROIProperties
{
    bool mbReset;       ///< True when Reset button clicked (clear ROI)
    bool mbApply;       ///< True when Apply button clicked (confirm ROI)
    bool mbNewMat;      ///< True for new image requiring ROI initialization
    CVImageROIProperties()
        : mbReset(false),
          mbApply(false),
          mbNewMat(true)
    {
    }
} CVImageROIProperties;

/**
 * @class CVImageROIModel
 * @brief Interactive ROI selection node with Apply/Reset workflow.
 *
 * CVImageROIModel enables users to select rectangular regions of interest from images
 * with visual feedback and confirmation workflow. It provides dual outputs: an annotated
 * full image showing the ROI rectangle, and a cropped image containing only the ROI region.
 *
 * **Port Configuration:**
 * - **Inputs:**
 *   - Port 0: CVImageData - Source image (full frame)
 *   - Port 1: CVImageData (optional) - External ROI rectangle coordinates (for programmatic control)
 * - **Outputs:**
 *   - Port 0: CVImageData - Annotated full image with ROI rectangle overlay
 *   - Port 1: CVImageData - Cropped ROI region (or full image before Apply)
 *
 * **Embedded Widget:**
 * - Apply Button: Confirm ROI and switch to cropped output
 * - Reset Button: Clear ROI and revert to full image
 * - Buttons enabled/disabled based on ROI state
 *
 * **ROI Selection Methods:**
 *
 * 1. **Interactive Drawing** (typical):
 *    - User draws rectangle on image display
 *    - Rectangle coordinates sent via Port 1 input
 *    - Preview shows rectangle overlay
 *    - Click Apply to confirm
 *
 * 2. **Manual Property Entry**:
 *    - Set Point1 (x1, y1) and Point2 (x2, y2) in properties panel
 *    - Rectangle appears immediately
 *    - Click Apply to confirm
 *
 * 3. **Programmatic Control**:
 *    - Connect ROI coordinates from upstream node
 *    - Automated ROI selection for batch processing
 *
 * **Workflow States:**
 *
 * ```
 * State 1: Initial (No ROI)
 *   - Output Port 0: Full image (no rectangle)
 *   - Output Port 1: Full image (uncropped)
 *   - Apply/Reset buttons: Disabled
 *
 * State 2: ROI Drawn (Not Applied)
 *   - Output Port 0: Full image WITH rectangle overlay
 *   - Output Port 1: Full image (not cropped yet)
 *   - Apply button: Enabled
 *   - Reset button: Enabled
 *
 * State 3: ROI Applied
 *   - Output Port 0: Full image WITH rectangle overlay
 *   - Output Port 1: CROPPED ROI region
 *   - Apply button: Disabled (already applied)
 *   - Reset button: Enabled
 *
 * State 4: After Reset
 *   - Return to State 1
 * ```
 *
 * **Common Use Cases:**
 *
 * 1. **License Plate Recognition:**
 *    ```
 *    Camera → CVImageROI (select plate region) → Apply → OCR → Display
 *    ```
 *
 * 2. **Face Feature Extraction:**
 *    ```
 *    Image → FaceDetect → BoundingBox → CVImageROI → FeatureExtract
 *    ```
 *
 * 3. **Multi-Region Processing:**
 *    ```
 *    Image ┬→ CVImageROI1 (region A) → Process A
 *          ├→ CVImageROI2 (region B) → Process B
 *          └→ CVImageROI3 (region C) → Process C
 *    ```
 *
 * 4. **Background Exclusion:**
 *    ```
 *    Image → CVImageROI (exclude borders) → Threshold → FindContour
 *    ```
 *
 * 5. **Fixed Region Monitoring:**
 *    ```
 *    Camera → CVImageROI (parking space) → [Lock ROI] → MotionDetect → Alert
 *    ```
 *
 * **Performance Characteristics:**
 * - ROI cropping: O(ROI_width × ROI_height) - very fast
 * - Rectangle overlay: O(4 × thickness) - negligible
 * - Typical latency: < 1ms for cropping operation
 * - Memory: Creates new cv::Mat for cropped region (shares data if possible)
 *
 * **Lock Mode:**
 * When `LockOutputROI` is enabled:
 * - ROI coordinates cannot be changed
 * - Apply/Reset buttons disabled
 * - Ensures consistent ROI across video frames
 * - Useful for batch processing or real-time monitoring
 *
 * **Design Rationale:**
 * - Dual outputs allow simultaneous visualization and processing
 * - Apply/Reset workflow prevents accidental ROI changes
 * - Lock mode supports automated processing pipelines
 * - External ROI input enables programmatic control
 * - Rectangle overlay provides immediate visual feedback
 *
 * @note ROI coordinates are clipped to image bounds to prevent errors
 * @note Cropped output shares image data when possible (cv::Mat::operator())
 * @see CVImageROIEmbeddedWidget for UI controls
 * @see cv::Rect, cv::Mat::operator() for ROI extraction
 */
class CVImageROIModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    CVImageROIModel();

    virtual
    ~CVImageROIModel() override {}

    QJsonObject
    save() const override;

    void
    load(const QJsonObject &p) override;

    unsigned int
    nPorts(PortType portType) const override;

    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;

    QWidget *
    embeddedWidget() override { return mpEmbeddedWidget; }

    void
    setModelProperty( QString &, const QVariant & ) override;

    QPixmap
    minPixmap() const override { return _minPixmap; }

    static const QString _category;

    static const QString _model_name;

private Q_SLOTS :

    /**
     * @brief Handles Apply/Reset button clicks from embedded widget.
     * @param button 0=Apply (confirm ROI), 1=Reset (clear ROI)
     */
    void em_button_clicked( int button );


private:

    /**
     * @brief Processes ROI selection, cropping, and visualization.
     *
     * **Processing Logic:**
     * ```cpp
     * if (props.mbReset) {
     *     // Reset workflow
     *     params.mCVPointRect1 = (0, 0);
     *     params.mCVPointRect2 = (0, 0);
     *     props.mbApply = false;
     *     props.mbNewMat = true;
     *     out[0] = in[0];  // Full image, no rectangle
     *     out[1] = in[0];  // Full image, no crop
     * }
     * else if (external_roi_input_connected && in[1]) {
     *     // Use external ROI coordinates
     *     extract_roi_from_input(in[1]);
     *     params.mCVPointRect1 = extracted_top_left;
     *     params.mCVPointRect2 = extracted_bottom_right;
     * }
     * 
     * if (props.mbApply) {
     *     // Apply workflow: crop to ROI
     *     cv::Rect roi(params.mCVPointRect1, params.mCVPointRect2);
     *     cv::Mat cropped = in[0]->data()(roi);  // Extract ROI region
     *     out[1] = std::make_shared<CVImageData>(cropped);
     * } else {
     *     // Not applied: output full image
     *     out[1] = in[0];
     * }
     * 
     * // Always draw rectangle on Port 0 output (if DisplayLines enabled)
     * if (params.mbDisplayLines && valid_roi) {
     *     cv::Mat annotated = in[0]->data().clone();
     *     cv::rectangle(annotated, params.mCVPointRect1, params.mCVPointRect2,
     *                   cv::Scalar(color[0], color[1], color[2]), thickness);
     *     out[0] = std::make_shared<CVImageData>(annotated);
     * } else {
     *     out[0] = in[0];
     * }
     * ```
     *
     * @param in Input images [0]=source, [1]=optional external ROI
     * @param out Output images [0]=annotated full, [1]=cropped or full
     * @param params ROI rectangle and visualization parameters
     * @param props ROI workflow state flags
     */
    void processData(const std::shared_ptr< CVImageData > (&in)[2], std::shared_ptr<CVImageData> (&out)[2],
                     const CVImageROIParameters & params, CVImageROIProperties &props );

    /**
     * @brief Updates ROI parameters from external input or properties.
     * @param in External ROI data (containing rectangle coordinates)
     * @param params Output: Updated ROI parameters
     */
    void overwrite(const std::shared_ptr<CVImageData>& in, CVImageROIParameters& params);

    static const std::string color[3];  ///< Color channel names for property system

    CVImageROIParameters mParams;         ///< Current ROI parameters
    CVImageROIProperties mProps;          ///< Current ROI state flags

    CVImageROIEmbeddedWidget* mpEmbeddedWidget;   ///< Apply/Reset button widget

    std::shared_ptr<CVImageData> mapCVImageInData[2] {{nullptr}};   ///< Input images
    std::shared_ptr<CVImageData> mapCVImageData[2] {{nullptr}};     ///< Output images

    QPixmap _minPixmap;  ///< Node icon
};

