//Copyright © 2020 - 2026, NECTEC, all rights reserved

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
 * @file CVTemplateMatchingModel.hpp
 * @brief Model for template matching and object localization in images.
 *
 * This file defines the CVTemplateMatchingModel class for finding occurrences of a
 * template image within a larger source image using various matching methods. It
 * outputs both the similarity map and the source image with the best match location
 * highlighted by a rectangle, enabling object detection and localization tasks.
 */

#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <opencv2/imgproc.hpp>

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "InformationData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @struct TemplateMatchingParameters
 * @brief Configuration parameters for template matching operations.
 *
 * Stores matching method selection and visualization settings for the bounding
 * rectangle drawn around detected matches.
 */
typedef struct TemplateMatchingParameters{
    int miMatchingMethod;     ///< Matching method (cv::TemplateMatchModes)
    int miMaxMatches;         ///< Maximum matching outputs for Information port
    
    /**
     * @brief Default constructor.
     *
     * Initializes with TM_SQDIFF method, 3 max matches.
     */
    TemplateMatchingParameters()
        : miMatchingMethod(cv::TM_SQDIFF),
          miMaxMatches(3)
    {
    }
} TemplateMatchingParameters;

/**
 * @class CVTemplateMatchingModel
 * @brief Node model for template matching and object localization.
 *
 * This model implements template matching using OpenCV's cv::matchTemplate() function,
 * which slides a template image over a source image and computes similarity at each
 * position. It outputs a cropped image corresponding to the region with the best match,
 * and a text report listing candidate matching areas sorted by their matching score.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Source image (where template is searched)
 * 2. **CVImageData** - Template image (what to find)
 *
 * **Output Ports:**
 * 1. **CVImageData** - Cropped image of the best match region (maximum matching score)
 * 2. **InformationData** - A text list of candidate bounding box rectangles sorted by score
 *
 * **Matching Methods (cv::TemplateMatchModes):**
 *
 * 1. **TM_SQDIFF (Squared Difference):**
 *    - Formula: \f$ R(x,y) = \sum_{x',y'} [T(x',y') - I(x+x',y+y')]^2 \f$
 *    - Lower values = better match
 *    - Sensitive to brightness differences
 *
 * 2. **TM_SQDIFF_NORMED (Normalized Squared Difference):**
 *    - Normalized version of TM_SQDIFF
 *    - Range: [0, 1], 0 = perfect match
 *    - Less sensitive to overall brightness
 *
 * 3. **TM_CCORR (Cross-Correlation):**
 *    - Formula: \f$ R(x,y) = \sum_{x',y'} [T(x',y') \cdot I(x+x',y+y')] \f$
 *    - Higher values = better match
 *    - Sensitive to brightness
 *
 * 4. **TM_CCORR_NORMED (Normalized Cross-Correlation):**
 *    - Normalized version of TM_CCORR
 *    - Range: [-1, 1], 1 = perfect match
 *    - Brightness invariant
 *
 * 5. **TM_CCOEFF (Correlation Coefficient):**
 *    - Subtracts mean before correlation
 *    - Higher values = better match
 *    - Partially brightness invariant
 *
 * 6. **TM_CCOEFF_NORMED (Normalized Correlation Coefficient):**
 *    - Normalized version of TM_CCOEFF
 *    - Range: [-1, 1], 1 = perfect match
 *    - **Recommended:** Best brightness/contrast invariance
 *
 * **Match Detection:**
 * - For TM_SQDIFF methods: Minimum value = best match
 * - For other methods: Maximum value = best match
 * - Uses cv::minMaxLoc() iteratively with peak suppression to extract multiple candidate regions.
 *
 * **Properties (Configurable):**
 * - **matching_method:** Matching algorithm (cv::TemplateMatchModes)
 * - **max_matches:** Maximum matching candidates to output/display in the Information report (default: 3)
 *
 * **Use Cases:**
 * - Logo detection in images
 * - GUI element localization (finding buttons, icons)
 * - Simple object tracking (when object doesn't change)
 * - Quality control (finding defects, verifying assembly)
 * - Optical character recognition (finding character templates)
 * - Game automation (finding UI elements)
 * - Document processing (finding stamps, signatures)
 *
 * **Limitations:**
 * - No rotation invariance (template must have same orientation)
 * - No scale invariance (template must have same size)
 * - Computationally expensive for large images/templates
 * - Lighting conditions must be similar
 *
 * **Best Practices:**
 * 1. Use TM_CCOEFF_NORMED for general cases (best robustness)
 * 2. Ensure template is smaller than source image
 * 3. Pre-process both images identically (grayscale, blur, etc.)
 * 4. Consider downscaling for faster processing
 * 5. Set appropriate max matches limit to filter candidate report size
 *
 * **Example Workflow:**
 * @code
 * [Scene Image] ----\
 *                    [TemplateMatching] -> [Cropped Image]
 * [Logo Template] --/                   \-> [Sorted Matches Info]
 * @endcode
 *
 * **Performance Notes:**
 * - Complexity: O(W×H×w×h) where (W,H) = source size, (w,h) = template size
 * - Faster with smaller templates
 * - Normalized methods slightly slower but more robust
 *
 * @see cv::matchTemplate
 * @see cv::minMaxLoc
 * @see FindContourModel (for shape-based detection)
 */
class CVTemplateMatchingModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVTemplateMatchingModel.
     *
     * Initializes with TM_SQDIFF method and default max matches value (3).
     */
    CVTemplateMatchingModel();

    /**
     * @brief Destructor.
     */
    virtual
    ~CVTemplateMatchingModel() override {}

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing matching parameters.
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
     * @return 2 for input (source + template), 2 for output (cropped image + sorted matches).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return CVImageData for inputs/cropped output, InformationData for sorted matches output.
     */
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief Returns the output data.
     * @param port Port index (0=cropped image, 1=sorted matches list).
     * @return Shared pointer to output NodeData (CVImageData or InformationData).
     */
    std::shared_ptr<NodeData>
    outData(PortIndex port) override;

    /**
     * @brief Sets input data and triggers template matching.
     * @param nodeData Input CVImageData (source or template).
     *
     * When both inputs are connected, performs template matching using
     * cv::matchTemplate() and updates both output ports.
     */
    void
    setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

    /**
     * @brief Returns nullptr (no embedded widget).
     * @return nullptr.
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    /**
     * @brief Sets a model property.
     * @param Property name ("matching_method", "max_matches").
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

    QString
    portToolTip(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private:
    TemplateMatchingParameters mParams;                       ///< Matching configuration
    std::shared_ptr<CVImageData> mapCVImageInData[2] {{ nullptr }}; ///< Input images [source, template]
    std::shared_ptr<CVImageData> mpCroppedImageData { nullptr };    ///< Output: Cropped image of max matching score
    std::shared_ptr<InformationData> mpInfoData { nullptr };        ///< Output: Bounding box rect list sorted by score
    QPixmap _minPixmap;                                       ///< Node iconBlock

    /**
     * @brief Processes input images and performs template matching.
     * @param in Array of 2 input CVImageData pointers [source, template].
     * @param outCrop Output cropped image of maximum matching score.
     * @param outInfo Output information with matching bounding box rectangles.
     * @param params Matching parameters.
     */
    void processData( const std::shared_ptr< CVImageData> (&in)[2],
                      std::shared_ptr< CVImageData > & outCrop,
                      std::shared_ptr< InformationData > & outInfo,
                      const TemplateMatchingParameters & params );
};

