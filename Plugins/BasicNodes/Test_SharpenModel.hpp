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
 * @file Test_SharpenModel.hpp
 * @brief Test node model for image sharpening operations.
 *
 * This file defines the Test_SharpenModel class, which implements basic image sharpening
 * filters for testing and development purposes. It demonstrates simple image processing
 * operations and serves as a test case for the node framework.
 *
 * **Purpose:** Testing framework and basic sharpening filter example.
 */

#ifndef TEST_SHARPENMODEL_H
#define TEST_SHARPENMODEL_H

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"

#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class Test_SharpenModel
 * @brief Test node model for basic image sharpening operations.
 *
 * This model implements simple image sharpening using kernel-based filtering,
 * designed for testing the node framework and demonstrating basic image enhancement.
 * It serves as both a functional sharpening filter and a test case for development.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Source image to sharpen
 *
 * **Output Ports:**
 * 1. **CVImageData** - Sharpened image
 *
 * **Sharpening Operation:**
 * Image sharpening enhances edges and fine details by amplifying high-frequency
 * components. Common approaches include:
 *
 * 1. **Unsharp Masking:**
 *    - Blur original image
 *    - Subtract blur from original to get detail mask
 *    - Add scaled mask back to original
 *    - Formula: sharpened = original + α × (original - blurred)
 *
 * 2. **Kernel-based Sharpening:**
 *    - Convolve with sharpening kernel
 *    - Example kernel: [0 -1 0; -1 5 -1; 0 -1 0]
 *    - Center weight > 1 amplifies pixel
 *    - Negative weights subtract neighbors
 *
 * 3. **Laplacian Sharpening:**
 *    - Apply Laplacian edge detector
 *    - Add result to original image
 *    - Formula: sharpened = original - ∇²(original)
 *
 * **Typical Sharpening Kernel:**
 * @code
 * [  0  -1   0 ]
 * [ -1   5  -1 ]
 * [  0  -1   0 ]
 * @endcode
 * or stronger:
 * @code
 * [ -1  -1  -1 ]
 * [ -1   9  -1 ]
 * [ -1  -1  -1 ]
 * @endcode
 *
 * **Use Cases:**
 * - Testing node framework functionality
 * - Basic image enhancement
 * - Pre-processing for OCR or text recognition
 * - Enhancing fine details in microscopy
 * - Improving perceived image quality
 *
 * **Limitations:**
 * - May amplify noise
 * - Can create edge artifacts
 * - No parameter control (fixed kernel)
 * - Test implementation (not production-quality)
 *
 * **Best Practices:**
 * - Apply to noise-reduced images
 * - Avoid excessive sharpening (halos)
 * - Consider using Filter2DModel for customizable kernels
 * - Use cv::GaussianBlur first to reduce noise
 *
 * **Alternative Approaches:**
 * For production use, consider:
 * - Filter2DModel with custom kernels
 * - Unsharp masking with adjustable strength
 * - Frequency domain filtering
 * - Bilateral filter for edge-preserving sharpening
 *
 * @see Filter2DModel
 * @see cv::filter2D
 * @see cv::Laplacian
 */
class Test_SharpenModel : public PBNodeDelegateModel
{
    Q_OBJECT

    public :

        /**
         * @brief Constructs a Test_SharpenModel.
         *
         * Initializes the sharpening node with default kernel.
         */
        Test_SharpenModel();
        
        /**
         * @brief Destructor.
         *
         * Cleans up allocated resources.
         */
        virtual ~Test_SharpenModel() override;
        
        /**
         * @brief Returns the number of ports.
         * @param PortType Input or Output.
         * @return 1 for both input and output.
         */
        unsigned int nPorts(PortType PortType) const override;
        
        /**
         * @brief Returns the data type for a specific port.
         * @param portType Input or Output.
         * @param portIndex Port index (0).
         * @return CVImageData for both ports.
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
        
        /**
         * @brief Returns the sharpened image.
         * @param port Port index (0).
         * @return Shared pointer to sharpened CVImageData.
         */
        std::shared_ptr<NodeData> outData(PortIndex port) override;
        
        /**
         * @brief Sets input image and triggers sharpening.
         * @param nodeData Input CVImageData to sharpen.
         * @param Port index (0).
         *
         * Applies sharpening filter when input image is received.
         */
        void setInData(std::shared_ptr<NodeData>nodeData,PortIndex) override;
        
        /**
         * @brief Returns nullptr (no embedded widget).
         * @return nullptr.
         */
        QWidget *embeddedWidget() override {return nullptr;}
        
        /**
         * @brief Returns the minimum node icon.
         * @return QPixmap icon.
         */
        QPixmap minPixmap() const override {return _minPixmap;}

        static const QString _category;   ///< Node category ("Test" or "Filters")
        static const QString _model_name; ///< Node display name

    private :

        std::shared_ptr<CVImageData> mpCVImageData = nullptr;  ///< Output sharpened image
        QPixmap _minPixmap;                                    ///< Node icon

};

#endif // TEST_SHARPENMODEL_H
