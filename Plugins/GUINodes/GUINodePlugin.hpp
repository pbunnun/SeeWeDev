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
 * @file GUINodePlugin.hpp
 * @brief Plugin interface for GUI control and display nodes.
 *
 * This file defines the plugin that registers GUI-related node models with
 * the CVDev application. GUI nodes provide interactive controls and visual
 * displays for dataflow graphs, enabling user interaction and data visualization.
 *
 * **Registered GUI Models:**
 * - LCD Number Display (digital numeric display)
 * - Push Button (manual trigger control)
 *
 * **Use Cases:**
 * - Manual control and triggering
 * - Numeric value display
 * - User interaction in dataflows
 * - Testing and debugging
 * - Dashboard creation
 *
 * @see PluginInterface
 * @see LCDNumberModel
 * @see PushButtonModel
 */

#pragma once

#include <PluginInterface.hpp>

#include <QObject>

/**
 * @class GUINodePlugin
 * @brief Plugin for registering GUI control and display node models.
 *
 * This plugin implements the Qt plugin interface to register interactive GUI
 * nodes with the CVDev application. These nodes provide user interface elements
 * embedded directly in the dataflow graph.
 *
 * **Registered Models:**
 * - **LCDNumberModel:** Digital LCD-style numeric display
 * - **PushButtonModel:** Clickable button for manual triggering
 *
 * **GUI Node Benefits:**
 * - Interactive control within dataflow graphs
 * - Real-time value visualization
 * - Manual triggering and testing
 * - No external windows required
 * - Embedded in node graph
 *
 * **Common Patterns:**
 * - Use PushButton for manual start/stop control
 * - Use LCDNumber to monitor numeric values
 * - Combine for interactive dashboards
 * - Testing and debugging workflows
 *
 * @see PluginInterface
 * @see LCDNumberModel
 * @see PushButtonModel
 */
class GUINodePlugin : public QObject,
                        public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "CVDev.PluginInterface" FILE "basicnodes.json" )
    Q_INTERFACES( PluginInterface )

public:
    /**
     * @brief Registers all GUI node models.
     * @param model_regs Shared pointer to the application's node registry
     * @return List of registered model names.
     *
     * This method is called during plugin initialization to register
     * GUI control and display nodes.
     */
    QStringList registerDataModel( std::shared_ptr< NodeDelegateModelRegistry > model_regs ) override;
};

