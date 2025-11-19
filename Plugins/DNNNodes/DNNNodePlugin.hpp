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
 * @file DNNNodePlugin.hpp
 * @brief Plugin interface for deep neural network (DNN) processing nodes.
 *
 * This file defines the plugin that registers all DNN-related node models
 * with the CVDev application. It provides access to various deep learning
 * models for object detection, classification, text detection/recognition,
 * and face detection.
 *
 * **Registered DNN Models:**
 * - YOLO Object Detection (YOLOv3/v4)
 * - Face Detection (DNN-based)
 * - ONNX Classification
 * - NecML Classification
 * - NomadML Classification
 * - Text Detection (DB algorithm)
 * - Text Recognition (CRNN-based)
 *
 * **Plugin System:**
 * Qt plugin architecture allows dynamic loading of node models at runtime,
 * enabling modular extension of the application without recompilation.
 *
 * @see PluginInterface
 * @see CVYoloDNNModel
 * @see FaceDetectionDNNModel
 * @see TextDetectionDNNModel
 * @see TextRecognitionDNNModel
 */

#pragma once

#include <PluginInterface.hpp>

#include <QObject>

/**
 * @class DNNNodePlugin
 * @brief Plugin for registering deep neural network node models.
 *
 * This plugin implements the Qt plugin interface to register all DNN-based
 * processing nodes with the CVDev application. It enables the application
 * to discover and instantiate DNN models at runtime.
 *
 * **Qt Plugin Mechanism:**
 * - Q_OBJECT: Enables Qt meta-object features (signals/slots, properties)
 * - Q_PLUGIN_METADATA: Provides plugin identification and metadata
 * - Q_INTERFACES: Declares implemented interface
 *
 * **Registered Models:**
 * The plugin registers the following DNN node models:
 * - **CVYoloDNNModel:** YOLO object detection
 * - **FaceDetectionDNNModel:** Face detection
 * - **OnnxClassificationDNNModel:** ONNX-based classification
 * - **NecMLClassificationModel:** NECTEC ML classification
 * - **NomadMLClassificationModel:** NOMAD ML classification
 * - **TextDetectionDNNModel:** Text region detection
 * - **TextRecognitionDNNModel:** OCR text recognition
 *
 * **Plugin Loading:**
 * The application automatically discovers and loads this plugin at startup,
 * making all DNN nodes available in the node palette.
 *
 * @see PluginInterface
 * @see NodeDelegateModelRegistry
 */
class DNNNodePlugin : public QObject,
                        public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "CVDev.PluginInterface" FILE "basicnodes.json" )
    Q_INTERFACES( PluginInterface )

public:
    /**
     * @brief Registers all DNN node models.
     * @param model_regs Model registry to register nodes with.
     * @return List of registered model names.
     *
     * This method is called by the application during plugin initialization.
     * It registers all DNN-related node model factories with the registry.
     *
     * **Registration Process:**
     * @code
     * model_regs->registerModel<CVYoloDNNModel>("YOLO Detection");
     * model_regs->registerModel<FaceDetectionDNNModel>("Face Detection");
     * // ... register other models
     * @endcode
     */
    QStringList registerDataModel( std::shared_ptr< NodeDelegateModelRegistry > model_regs );
};

