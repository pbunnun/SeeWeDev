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
 * @file BasicNodePlugin.hpp
 * @brief Plugin interface for registering basic OpenCV image processing nodes
 * 
 * This file defines the main plugin class that registers all basic image processing
 * node models with the CVDev framework. The BasicNodes plugin provides fundamental
 * computer vision operations including image I/O, filtering, transformations, and
 * basic image analysis.
 */

#pragma once

#include <PluginInterface.hpp>

#include <QObject>

/**
 * @class BasicNodePlugin
 * @brief Main plugin class for BasicNodes collection
 * 
 * This plugin provides a comprehensive set of basic computer vision and image processing
 * nodes for the CVDev visual programming environment. It implements the PluginInterface
 * to register model delegates with the node editor framework.
 * 
 * The plugin registers various categories of nodes:
 * - Image I/O: Loading, saving, and displaying images
 * - Filtering: Gaussian blur, Sobel, morphological operations
 * - Transformations: Resize, rotate, color space conversions
 * - Analysis: Histograms, contours, connected components
 * - Utility: Timers, synchronization gates, data generators
 * 
 * @note This plugin uses Qt's plugin system via Q_PLUGIN_METADATA
 * @see PluginInterface for the base interface contract
 */
class BasicNodePlugin : public QObject,
                        public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "CVDev.PluginInterface" FILE "basicnodes.json" )
    Q_INTERFACES( PluginInterface )

public:
    /**
     * @brief Registers all basic node models with the framework
     * 
     * This method is called by the CVDev framework during plugin initialization.
     * It registers all available node model classes with the provided registry,
     * making them available in the node editor palette.
     * 
     * @param model_regs Shared pointer to the node delegate model registry
     * @return QStringList List of successfully registered model names
     * 
     * @note Models are registered using their class names as unique identifiers
     * @see NodeDelegateModelRegistry for registration mechanism
     */
    QStringList registerDataModel( std::shared_ptr< NodeDelegateModelRegistry > model_regs );
};

