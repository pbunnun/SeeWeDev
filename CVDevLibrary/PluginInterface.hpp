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
 * @file PluginInterface.hpp
 * @brief Plugin system interface for dynamic node registration in CVDev.
 *
 * This file defines the plugin architecture for CVDev, enabling runtime loading
 * of node libraries from shared libraries (.so, .dylib, .dll). Plugins register
 * custom node types with the NodeDelegateModelRegistry for use in dataflow graphs.
 *
 * **Key Features:**
 * - **Dynamic Loading:** Load plugins at runtime from directories
 * - **Node Registration:** Register custom node types with QtNodes
 * - **Type Converters:** Add automatic type conversion nodes
 * - **Duplicate Detection:** Prevent conflicts from duplicate node names
 * - **Qt Plugin System:** Uses Q_DECLARE_INTERFACE for type safety
 *
 * **Plugin Architecture:**
 * @code
 * CVDev Application
 *   ├── Load plugins from directories
 *   │     ├── BasicNodes.so
 *   │     ├── DNNNodes.so
 *   │     └── CustomNodes.so
 *   │
 *   └── Each plugin implements PluginInterface
 *         └── registerDataModel() registers nodes
 *               ├── ImageLoader
 *               ├── GaussianBlur
 *               └── FaceDetector
 * @endcode
 *
 * **Common Use Cases:**
 * - Load all plugins from application plugins directory
 * - Add custom node types without recompiling main application
 * - Modular feature sets (basic, advanced, customer-specific)
 * - Third-party node development
 *
 * **Plugin Loading Flow:**
 * @code
 * // 1. Initialize registry
 * auto registry = std::make_shared<NodeDelegateModelRegistry>();
 * 
 * // 2. Add built-in type converters
 * add_type_converters(registry);
 * 
 * // 3. Load plugins from directory
 * QList<QPluginLoader*> loaders;
 * load_plugins(registry, loaders);
 *   → Scans plugins/ directory
 *   → Loads each .so/.dylib/.dll file
 *   → Calls registerDataModel() for each plugin
 *   → Nodes available in palette
 * @endcode
 *
 * **Creating a Plugin:**
 * @code
 * // myplugin.hpp
 * class MyPlugin : public QObject, public PluginInterface {
 *     Q_OBJECT
 *     Q_PLUGIN_METADATA(IID PluginInterface_iid)
 *     Q_INTERFACES(PluginInterface)
 * 
 * public:
 *     QStringList registerDataModel(
 *         std::shared_ptr<NodeDelegateModelRegistry> registry) override {
 *         QStringList duplicates;
 *         registerModel<MyCustomNode>(registry, duplicates);
 *         registerModel<AnotherNode>(registry, duplicates);
 *         return duplicates;
 *     }
 * };
 * @endcode
 *
 * **Migration Notes (v2 → v3):**
 * - DataModelRegistry → NodeDelegateModelRegistry
 * - Updated registry API for model registration
 * - registeredModelCreators() returns different structure
 *
 * @see NodeDelegateModelRegistry for node type registration
 * @see QPluginLoader for Qt plugin loading mechanism
 * @see Q_DECLARE_INTERFACE for Qt interface declaration
 */

#pragma once

#include <QtPlugin>
#include <QPluginLoader>
// TODO NodeEditor v3 Migration: Updated include for NodeDelegateModelRegistry
// Old v2: #include <DataModelRegistry>
// New v3: #include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/NodeDelegateModelRegistry>
#include "CVDevLibrary.hpp"
#include <QDir>

// TODO NodeEditor v3 Migration: Updated to use NodeDelegateModelRegistry
// using QtNodes::DataModelRegistry;  // v2
using QtNodes::NodeDelegateModelRegistry;  // v3

/**
 * @brief Registers automatic type converter nodes with the registry.
 *
 * Adds built-in type conversion nodes that enable automatic data type
 * conversions in the dataflow graph (e.g., int → double, cv::Mat → QImage).
 *
 * @param model_regs Shared pointer to the node registry
 *
 * **Type Converters Enable:**
 * @code
 * // Automatic conversion between compatible types
 * IntegerData → DoubleData       (int to double)
 * CVImageData → QImageData       (cv::Mat to QImage)
 * FloatData → IntegerData        (float to int with rounding)
 * @endcode
 *
 * **Usage:**
 * @code
 * auto registry = std::make_shared<NodeDelegateModelRegistry>();
 * add_type_converters(registry);  // Add converters first
 * load_plugins(registry, loaders); // Then load plugins
 * @endcode
 *
 * @note Should be called before loading plugins to ensure converters are available
 * @see NodeDelegateModelRegistry::registerTypeConverter() for converter registration
 */
void CVDEVSHAREDLIB_EXPORT add_type_converters( std::shared_ptr< NodeDelegateModelRegistry > model_regs );

/**
 * @brief Loads all plugins from a specific directory.
 *
 * Scans the specified directory for plugin files (.so on Linux, .dylib on macOS,
 * .dll on Windows) and loads each one into the application.
 *
 * @param model_regs Shared pointer to the node registry for node registration
 * @param plugins_list Output list to store QPluginLoader instances
 * @param pluginsDir Directory to scan for plugin files
 *
 * **Example:**
 * @code
 * auto registry = std::make_shared<NodeDelegateModelRegistry>();
 * QList<QPluginLoader*> loaders;
 * QDir customDir("/opt/cvdev/custom_plugins");
 * 
 * load_plugins_from_dir(registry, loaders, customDir);
 * // All plugins in /opt/cvdev/custom_plugins now loaded
 * @endcode
 *
 * **Directory Scan:**
 * @code
 * // Looks for files matching:
 * // - Linux:   *.so
 * // - macOS:   *.dylib
 * // - Windows: *.dll
 * 
 * // Skips non-plugin files
 * // Logs errors for failed loads
 * @endcode
 *
 * @note Loaders are added to plugins_list for cleanup on application exit
 * @see QPluginLoader for plugin loading mechanism
 * @see load_plugins() for default directory loading
 */
void CVDEVSHAREDLIB_EXPORT load_plugins_from_dir( std::shared_ptr< NodeDelegateModelRegistry > model_regs, QList< QPluginLoader *> & plugins_list, QDir pluginsDir );

/**
 * @brief Loads all plugins from the default plugins directory.
 *
 * Convenience function that loads plugins from the application's standard
 * plugins directory (typically ./plugins or /usr/lib/cvdev/plugins).
 *
 * @param model_regs Shared pointer to the node registry for node registration
 * @param plugins_list Output list to store QPluginLoader instances
 *
 * **Example:**
 * @code
 * // In main() or application initialization
 * auto registry = std::make_shared<NodeDelegateModelRegistry>();
 * QList<QPluginLoader*> loaders;
 * 
 * add_type_converters(registry);  // Add converters first
 * load_plugins(registry, loaders); // Load all plugins
 * 
 * // Registry now contains all node types from plugins
 * auto* scene = new DataFlowGraphicsScene(registry);
 * @endcode
 *
 * **Default Plugin Directories:**
 * @code
 * // Linux:   /usr/lib/cvdev/plugins/
 * // macOS:   CVDev.app/Contents/PlugIns/
 * // Windows: C:\Program Files\CVDev\plugins\
 * @endcode
 *
 * @note Uses QCoreApplication::applicationDirPath() to locate plugins
 * @see load_plugins_from_dir() for custom directory loading
 * @see QStandardPaths for platform-specific paths
 */
void CVDEVSHAREDLIB_EXPORT load_plugins( std::shared_ptr< NodeDelegateModelRegistry > model_regs, QList< QPluginLoader *> & plugins_list );

/**
 * @brief Loads a single plugin from a specific file path.
 *
 * Loads an individual plugin library file and registers its nodes with
 * the registry. Useful for dynamic plugin loading or testing.
 *
 * @param model_regs Shared pointer to the node registry for node registration
 * @param plugins_list Output list to store the QPluginLoader instance
 * @param filename Absolute or relative path to the plugin file
 *
 * **Example:**
 * @code
 * auto registry = std::make_shared<NodeDelegateModelRegistry>();
 * QList<QPluginLoader*> loaders;
 * 
 * // Load specific plugin
 * load_plugin(registry, loaders, "/usr/lib/cvdev/plugins/CustomNodes.so");
 * 
 * // Plugin's nodes now available in registry
 * @endcode
 *
 * **Error Handling:**
 * @code
 * load_plugin(registry, loaders, "BadPlugin.so");
 * // Logs error if:
 * // - File not found
 * // - Not a valid Qt plugin
 * // - Wrong interface version
 * // - Registration fails
 * @endcode
 *
 * @note Validates plugin interface before loading
 * @note Adds loader to plugins_list even if loading fails (for cleanup)
 * @see QPluginLoader::load() for loading details
 * @see QPluginLoader::errorString() for error messages
 */
void CVDEVSHAREDLIB_EXPORT load_plugin( std::shared_ptr< NodeDelegateModelRegistry > model_regs, QList< QPluginLoader *> & plugins_list, QString filename );

/**
 * @class PluginInterface
 * @brief Abstract interface for CVDev plugin implementations.
 *
 * Defines the contract that all CVDev plugins must implement. Plugins register
 * their custom node types with the NodeDelegateModelRegistry through the
 * registerDataModel() method.
 *
 * **Core Functionality:**
 * - **Node Registration:** Register multiple node types from a plugin
 * - **Duplicate Detection:** Report duplicate model names
 * - **Type Safety:** Qt interface declaration ensures correct plugin type
 *
 * **Plugin Implementation Pattern:**
 * @code
 * // MyPlugin.hpp
 * class MyPlugin : public QObject, public PluginInterface {
 *     Q_OBJECT
 *     Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "myplugin.json")
 *     Q_INTERFACES(PluginInterface)
 * 
 * public:
 *     QStringList registerDataModel(
 *         std::shared_ptr<NodeDelegateModelRegistry> registry) override {
 *         
 *         QStringList duplicates;
 *         
 *         // Register each node type
 *         registerModel<ImageLoaderNode>(registry, duplicates);
 *         registerModel<ImageSaverNode>(registry, duplicates);
 *         registerModel<BlurFilterNode>(registry, duplicates);
 *         
 *         return duplicates;  // List of any duplicate names found
 *     }
 * };
 * @endcode
 *
 * **Plugin Metadata (myplugin.json):**
 * @code
 * {
 *     "Name": "My Custom Nodes",
 *     "Version": "1.0.0",
 *     "Author": "Your Name",
 *     "Description": "Custom image processing nodes"
 * }
 * @endcode
 *
 * **Qt Plugin Macros:**
 * - Q_OBJECT: Enables Qt meta-object system
 * - Q_PLUGIN_METADATA: Specifies plugin metadata and IID
 * - Q_INTERFACES: Declares implemented interface
 *
 * @see registerDataModel() for node registration implementation
 * @see registerModel() template for individual node registration
 * @see Q_DECLARE_INTERFACE for Qt interface system
 */
class PluginInterface
{
public:
    /**
     * @brief Virtual destructor for interface.
     *
     * Ensures proper cleanup when plugins are unloaded.
     */
    virtual ~PluginInterface() = default;

    /**
     * @brief Registers all node types provided by this plugin.
     *
     * Called during plugin loading to register custom node types with the
     * NodeDelegateModelRegistry. Each plugin should register all its node
     * types in this method using the registerModel() template.
     *
     * @param model_regs Shared pointer to the application's node registry
     * @return QStringList List of duplicate model names (if any)
     *
     * **Implementation Example:**
     * @code
     * QStringList MyPlugin::registerDataModel(
     *     std::shared_ptr<NodeDelegateModelRegistry> registry) {
     *     
     *     QStringList duplicates;
     *     
     *     // Register each node type from this plugin
     *     registerModel<ImageLoaderModel>(registry, duplicates);
     *     registerModel<ImageSaverModel>(registry, duplicates);
     *     registerModel<GaussianBlurModel>(registry, duplicates);
     *     registerModel<CannyEdgeModel>(registry, duplicates);
     *     
     *     // Log any duplicates
     *     if (!duplicates.isEmpty()) {
     *         qWarning() << "Duplicate models detected:" << duplicates;
     *     }
     *     
     *     return duplicates;
     * }
     * @endcode
     *
     * **Duplicate Handling:**
     * @code
     * // If another plugin already registered "ImageLoader"
     * // duplicates list will contain "ImageLoader"
     * // First registration wins, duplicate is skipped
     * @endcode
     *
     * **Plugin Categories:**
     * Nodes should define static members for categorization:
     * @code
     * class MyNode : public PBNodeDelegateModel {
     * public:
     *     static const QString _category;  // e.g., "Image Processing"
     *     static const QString _model_name; // e.g., "Gaussian Blur"
     *     // ...
     * };
     * @endcode
     *
     * @note Called once during application initialization for each plugin
     * @note Return value used for logging/debugging duplicate detection
     * @see registerModel() for individual node registration
     */
    virtual
    QStringList registerDataModel( std::shared_ptr< NodeDelegateModelRegistry > model_regs ) = 0;

    /**
     * @brief Template helper to register individual node model types.
     *
     * Registers a single node model type with the registry, checking for
     * duplicates. If a model with the same name already exists, the duplicate
     * is logged but not registered.
     *
     * @tparam ModelType The node model class to register (must derive from NodeDelegateModel)
     * @param model_regs Shared pointer to the node registry
     * @param duplicate_model_names Output list to append duplicate names
     *
     * **Template Requirements:**
     * ModelType must have:
     * - static const QString _model_name: Unique identifier
     * - static const QString _category: Node palette category
     * - Default constructor
     *
     * **Example Node Declaration:**
     * @code
     * class GaussianBlurModel : public PBNodeDelegateModel {
     * public:
     *     static const QString _category;   // "Image Filters"
     *     static const QString _model_name; // "Gaussian Blur"
     *     
     *     GaussianBlurModel();
     *     // ... node implementation ...
     * };
     * @endcode
     *
     * **Usage in Plugin:**
     * @code
     * QStringList MyPlugin::registerDataModel(
     *     std::shared_ptr<NodeDelegateModelRegistry> registry) {
     *     
     *     QStringList dups;
     *     registerModel<GaussianBlurModel>(registry, dups);
     *     registerModel<MedianBlurModel>(registry, dups);
     *     registerModel<BilateralFilterModel>(registry, dups);
     *     return dups;
     * }
     * @endcode
     *
     * **Duplicate Detection:**
     * @code
     * // First call - successful registration
     * registerModel<ImageLoader>(registry, dups);  // Registered OK
     * 
     * // Second call - duplicate detected
     * registerModel<ImageLoader>(registry, dups);  // Skipped, "ImageLoader" added to dups
     * @endcode
     *
     * **Registry Structure:**
     * After registration, nodes appear in palette under their category:
     * @code
     * Image Filters/
     *   ├── Gaussian Blur
     *   ├── Median Blur
     *   └── Bilateral Filter
     * @endcode
     *
     * @note Migration from v2: registeredModelCreators() API differs in v3
     * @note First registration wins - subsequent registrations of same name skipped
     *
     * @see NodeDelegateModelRegistry::registerModel() for registry API
     * @see PBNodeDelegateModel for base node model class
     */
    template <typename ModelType>
    void
    registerModel(std::shared_ptr< NodeDelegateModelRegistry > model_regs, QStringList & duplicate_model_names )
    {
        // TODO NodeEditor v3 Migration: The v3 registry API is different
        // v2: registeredModelCreators().count(name)
        // v3: registeredModelCreators() returns different structure
        // For now, keeping similar logic but may need adjustment
        if( model_regs->registeredModelCreators().count( ModelType::_model_name ) )
            duplicate_model_names.append( ModelType::_model_name );
        else
            model_regs->registerModel< ModelType >( ModelType::_category );
    }
};

QT_BEGIN_NAMESPACE

/**
 * @def PluginInterface_iid
 * @brief Interface identifier for CVDev plugins.
 *
 * Unique string identifier used by Qt's plugin system to verify plugin
 * interface compatibility. Must match in both interface declaration and
 * plugin implementation.
 *
 * **Usage in Plugin:**
 * @code
 * class MyPlugin : public QObject, public PluginInterface {
 *     Q_OBJECT
 *     Q_PLUGIN_METADATA(IID PluginInterface_iid)
 *     Q_INTERFACES(PluginInterface)
 *     // ...
 * };
 * @endcode
 *
 * **Version Control:**
 * The "/1.0" suffix indicates interface version. Increment when making
 * incompatible interface changes.
 *
 * @note Must match exactly between interface and implementation
 * @see Q_DECLARE_INTERFACE for Qt interface system
 */
#define PluginInterface_iid "CVDev.PluginInterface/1.0"

/**
 * @brief Declares PluginInterface to Qt's meta-object system.
 *
 * Registers the interface with Qt's plugin loader, enabling runtime
 * type checking and interface casting.
 *
 * @see Q_DECLARE_INTERFACE documentation for Qt plugin system
 * @see QPluginLoader::instance() for plugin instantiation
 */
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

QT_END_NAMESPACE
