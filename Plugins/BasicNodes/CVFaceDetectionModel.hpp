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
 * @file CVFaceDetectionModel.hpp
 * @brief Provides face detection using Haar Cascade or DNN-based classifiers.
 *
 * This file implements a node for detecting human faces in images using OpenCV's face
 * detection algorithms. The node supports both traditional Haar Cascade classifiers
 * (fast, CPU-based) and modern DNN-based detectors (more accurate, can use GPU).
 *
 * Face detection is a fundamental computer vision task with applications ranging from
 * photography (auto-focus, smile detection) to security (surveillance, access control)
 * and human-computer interaction (gaze tracking, emotion recognition).
 *
 * The algorithm identifies rectangular regions in the image that likely contain faces,
 * returning bounding boxes that can be used for:
 * - Face counting (number of people in scene)
 * - Face cropping (extract individual faces for recognition)
 * - Face tracking (follow faces across video frames)
 * - Feature point initialization (for landmark detection)
 * - ROI selection (focus processing on face regions)
 *
 * Detection Methods:
 * 1. Haar Cascade (Classic):
 *    - Uses hand-crafted features (Haar-like features)
 *    - Fast, runs efficiently on CPU
 *    - Requires frontal or near-frontal faces
 *    - Pre-trained models: frontalface, profileface, eye, smile, etc.
 *
 * 2. DNN-based (Modern):
 *    - Uses deep neural networks (e.g., SSD, YOLO, ResNet)
 *    - More accurate, robust to pose and lighting
 *    - Can utilize GPU acceleration
 *    - Detects faces at various angles
 *
 * The node provides an embedded widget for:
 * - Loading cascade XML files or DNN models
 * - Adjusting detection parameters (scale, min neighbors, min size)
 * - Toggling visualization options (bounding boxes, confidence scores)
 *
 * Typical Applications:
 * - Photo tagging and organization
 * - Attendance systems (count people in frame)
 * - Surveillance and security monitoring
 * - Video conferencing (auto-framing on speakers)
 * - Face recognition pipeline initialization
 * - Demographic analysis (age/gender estimation preprocessing)
 *
 * @see CVFaceDetectionModel, CVFaceDetectionEmbeddedWidget, cv::CascadeClassifier, cv::dnn
 */

#ifndef FACEDETECTIONMODEL_H
#define FACEDETECTIONMODEL_H

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include "PBNodeDelegateModel.hpp"
#include "CVFaceDetectionEmbeddedWidget.hpp"

#include "CVImageData.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class CVFaceDetectionModel
 * @brief Node for detecting human faces in images using cascade classifiers or DNN models.
 *
 * This model implements face detection capabilities using OpenCV's cv::CascadeClassifier
 * or cv::dnn module. It processes input images to identify rectangular regions containing
 * faces, outputting a visualization image with bounding boxes drawn around detected faces.
 *
 * Detection Pipeline:
 *
 * 1. Preprocessing:
 *    - Convert input to grayscale (Haar cascades require grayscale)
 *    - Optional histogram equalization for improved contrast
 *    - Image pyramids for multi-scale detection
 *
 * 2. Face Detection:
 *    Using Haar Cascade (cv::CascadeClassifier::detectMultiScale):
 *    ```cpp
 *    cascade.detectMultiScale(
 *        gray_image,           // Input grayscale image
 *        faces,                // Output vector of Rect (bounding boxes)
 *        scaleFactor,          // Image pyramid scale (e.g., 1.1 = 10% reduction per level)
 *        minNeighbors,         // Minimum detections to confirm (higher = fewer false positives)
 *        flags,                // Optional flags (e.g., CASCADE_SCALE_IMAGE)
 *        minSize,              // Minimum face size (pixels)
 *        maxSize               // Maximum face size (pixels)
 *    );
 *    ```
 *
 *    Key Parameters:
 *    - scaleFactor: Controls pyramid step (1.1 = fine search, 1.3 = coarse/fast)
 *    - minNeighbors: Votes required for detection (3-6 typical, higher reduces false positives)
 *    - minSize/maxSize: Expected face size range (speeds up detection)
 *
 * 3. Post-processing:
 *    - Non-maximum suppression (remove overlapping detections)
 *    - Draw bounding boxes on output image
 *    - Optional confidence scoring (for DNN models)
 *
 * Algorithm Overview (Haar Cascade):
 *
 * Haar cascades use a machine learning approach based on Haar-like features:
 *
 * Feature Extraction:
 * - Rectangular patterns capturing intensity differences
 * - Examples: horizontal edges (eyebrows), vertical edges (nose bridge)
 * - Computed rapidly using integral images
 *
 * Cascade Structure:
 * - Series of increasingly complex classifiers
 * - Early stages reject obvious non-faces quickly (~95% rejection)
 * - Later stages perform detailed analysis on candidates
 * - Achieves speed through early rejection (most windows dismissed in milliseconds)
 *
 * Multi-Scale Detection:
 * - Image pyramid: Process at multiple scales (e.g., 100%, 90%, 81%, ...)
 * - OR: Scale detector rather than image (controlled by flags)
 * - Detects faces from far away (small) to close up (large)
 *
 * Common Use Cases:
 *
 * 1. Face Counting:
 *    ```
 *    Camera → FaceDetection → CountRects → InformationDisplay
 *    ```
 *    Output: "5 faces detected"
 *
 * 2. Face Cropping for Recognition:
 *    ```
 *    ImageLoader → FaceDetection → ROI Extractor → FaceRecognitionModel
 *    ```
 *    Extract individual face regions for identification
 *
 * 3. Auto-Focus in Photography:
 *    ```
 *    Camera → FaceDetection → CalculateFocusPoint → CameraControl
 *    ```
 *    Center camera focus on detected faces
 *
 * 4. Privacy Protection:
 *    ```
 *    Video → FaceDetection → BlurRegions → Output
 *    ```
 *    Blur detected faces for privacy compliance
 *
 * 5. Attendance System:
 *    ```
 *    Camera → FaceDetection → Log(count, timestamp) → Database
 *    ```
 *    Count people entering/exiting areas
 *
 * Widget Functionality (CVFaceDetectionEmbeddedWidget):
 * - Classifier Selection:
 *   * Load Haar cascade XML files (e.g., haarcascade_frontalface_default.xml)
 *   * Load DNN model files (e.g., res10_300x300_ssd_iter_140000.caffemodel)
 * - Parameter Tuning:
 *   * Adjust scaleFactor slider (1.05 to 2.0)
 *   * Set minNeighbors (1 to 10)
 *   * Configure min/max face size
 * - Visualization Options:
 *   * Toggle bounding box display
 *   * Choose box color and thickness
 *   * Display confidence scores (for DNN)
 *
 * Performance Characteristics:
 * - Haar Cascade (CPU):
 *   * 640×480 image: 10-50ms (depends on face count and parameters)
 *   * Real-time capable: 20-100 FPS
 *   * Scales linearly with image resolution
 * - DNN Model (GPU):
 *   * 640×480 image: 5-20ms (with CUDA acceleration)
 *   * More consistent performance regardless of face count
 *   * Benefits significantly from GPU acceleration
 *
 * Optimization Tips:
 * 1. Set tight minSize/maxSize bounds (e.g., 30×30 to 200×200)
 * 2. Use larger scaleFactor for faster detection (e.g., 1.3 instead of 1.1)
 * 3. Increase minNeighbors to reduce false positives
 * 4. Use CASCADE_SCALE_IMAGE flag for better performance on large images
 * 5. Process every Nth frame in video (e.g., every 3rd frame)
 * 6. Resize large images before detection (e.g., max width 640)
 * 7. Use DNN models with GPU for best accuracy/speed tradeoff
 *
 * Limitations:
 * - Haar Cascades:
 *   * Struggles with non-frontal faces (profile, tilted)
 *   * Sensitive to lighting conditions
 *   * May miss faces with accessories (sunglasses, hats)
 *   * False positives on face-like patterns
 * - General:
 *   * Cannot identify individuals (detection only, not recognition)
 *   * Performance degrades with very small or very large faces
 *   * Occlusions (hands, objects) reduce detection accuracy
 *
 * Pre-trained Models:
 * OpenCV provides several Haar cascade files:
 * - haarcascade_frontalface_default.xml (most common)
 * - haarcascade_frontalface_alt.xml (alternative, slightly different)
 * - haarcascade_profileface.xml (side-view faces)
 * - haarcascade_eye.xml (eye detection)
 * - haarcascade_smile.xml (smile detection)
 *
 * DNN models (more accurate):
 * - res10_300x300_ssd_iter_140000.caffemodel (Caffe SSD)
 * - opencv_face_detector_uint8.pb (TensorFlow)
 * - yunet (lightweight, very fast)
 *
 * Best Practices:
 * 1. Choose appropriate model for your use case:
 *    - Frontal faces, speed critical: Haar frontalface
 *    - High accuracy needed: DNN models
 *    - Profile faces: Use profile cascade or DNN
 * 2. Tune parameters on representative test images
 * 3. Apply preprocessing: GaussianBlur, histogram equalization
 * 4. Post-process results: filter by size, aspect ratio, position
 * 5. Track faces across frames for video (reduces jitter)
 * 6. Combine with skin color segmentation for better accuracy
 *
 * Design Rationale:
 * - Embedded Widget: Allows real-time parameter tuning without recompiling
 * - Static processData: Enables efficient batch processing
 * - Model Loading: Supports user-provided custom-trained cascades
 * - Visualization: Immediate feedback on detection results
 *
 * @see CVFaceDetectionEmbeddedWidget, cv::CascadeClassifier, cv::dnn::Net
 */
class CVFaceDetectionModel : public PBNodeDelegateModel {
    Q_OBJECT

    public:
        CVFaceDetectionModel();

        virtual
        ~CVFaceDetectionModel() override {}

        unsigned int
        nPorts(PortType portType) const override;

        NodeDataType
        dataType(PortType portType, PortIndex portIndex) const override;

        std::shared_ptr<NodeData>
        outData(PortIndex port) override;

        void
        setInData(std::shared_ptr<NodeData> nodeData, PortIndex) override;

        /**
         * @brief Returns the embedded configuration widget.
         * @return Pointer to CVFaceDetectionEmbeddedWidget for model loading and parameter tuning
         */
        QWidget *
        embeddedWidget() override { return mpEmbeddedWidget; }
    
        void
        setModelProperty( QString &, const QVariant & ) override;
    
        QJsonObject
        save() const override;

        QPixmap
        minPixmap() const override {
            return _minPixmap;
        }

        static const QString _category;

        static const QString _model_name;
    
    private Q_SLOTS:
        /**
         * @brief Handles button clicks from the embedded widget.
         *
         * This slot is connected to the embedded widget's button signals for actions like:
         * - Load cascade/model file
         * - Adjust detection parameters
         * - Toggle visualization options
         * - Reset to default settings
         *
         * @param button Integer identifier for the clicked button
         */
        void
        em_button_clicked( int button );

    private:
        CVFaceDetectionEmbeddedWidget * mpEmbeddedWidget;  ///< Embedded widget for UI controls
        
        /**
         * @brief Output image with detected faces highlighted.
         *
         * Contains the input image with bounding boxes drawn around all detected faces.
         * Boxes are typically rendered in a bright color (e.g., green, red) with configurable
         * thickness. May also include text labels showing confidence scores.
         */
        std::shared_ptr<CVImageData> mpCVImageData {
            nullptr
        };
        
        QPixmap _minPixmap;  ///< Node icon for graph display
        
        /**
         * @brief Core face detection processing function.
         *
         * This static method performs the actual face detection using the loaded classifier:
         *
         * Algorithm Implementation:
         * 1. Input Validation:
         *    ```cpp
         *    if (p == nullptr || p->data().empty()) return cv::Mat();
         *    ```
         *
         * 2. Grayscale Conversion:
         *    ```cpp
         *    cv::Mat gray;
         *    if (image.channels() == 3)
         *        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
         *    else
         *        gray = image.clone();
         *    ```
         *
         * 3. Optional Preprocessing:
         *    ```cpp
         *    cv::equalizeHist(gray, gray);  // Enhance contrast
         *    ```
         *
         * 4. Multi-Scale Detection:
         *    ```cpp
         *    std::vector<cv::Rect> faces;
         *    cascade.detectMultiScale(
         *        gray,                    // Grayscale input
         *        faces,                   // Output face rectangles
         *        1.1,                     // Scale factor (10% reduction per pyramid level)
         *        3,                       // Min neighbors (3-6 typical)
         *        0,                       // Flags
         *        cv::Size(30, 30)         // Minimum face size
         *    );
         *    ```
         *
         * 5. Visualization:
         *    ```cpp
         *    cv::Mat output = image.clone();
         *    for (const auto& face : faces) {
         *        cv::rectangle(output, face, cv::Scalar(0, 255, 0), 2);
         *        // Optional: Draw confidence score or face number
         *    }
         *    ```
         *
         * Return Value:
         * - Success: cv::Mat with detected faces marked by bounding boxes
         * - No faces: Original image returned (no boxes drawn)
         * - Error: Empty cv::Mat() if input is invalid
         *
         * Example Detection:
         * ```
         * Input: Family photo (3 people)
         * Output: Same photo with 3 green rectangles around faces
         * Processing time: ~25ms (640×480 image, Haar cascade)
         * ```
         *
         * Thread Safety:
         * - Static method: No shared state modified
         * - Cascade classifier should be loaded before calling
         * - Safe for parallel execution if each thread has own classifier instance
         *
         * @param p Shared pointer to input CVImageData containing image to process
         * @return cv::Mat with detected faces marked by bounding boxes
         *
         * @note Requires grayscale or color 8-bit input image
         * @note Returns empty Mat if classifier not loaded or input invalid
         * @note Detection parameters come from embedded widget settings
         *
         * @see cv::CascadeClassifier::detectMultiScale, CVFaceDetectionEmbeddedWidget
         */
        static cv::Mat processData(const std::shared_ptr<CVImageData> &p);
};

#endif
