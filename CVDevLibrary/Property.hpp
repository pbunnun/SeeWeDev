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
 * @file Property.hpp
 * @brief Property system for node parameter configuration in CVDev.
 *
 * This file defines the property framework used for node configuration,
 * including type-specific property structures and the base Property class
 * hierarchy. Properties appear in the Qt Property Browser for user editing.
 *
 * **Key Components:**
 * - **Property Structures:** EnumPropertyType, DoublePropertyType, IntPropertyType, etc.
 * - **Base Classes:** Property (base), TypedProperty<T> (template)
 * - **Property Vector:** std::vector of property shared pointers
 *
 * **Property Types:**
 * - **Numeric:** Int, Double, Float, Uchar (with min/max ranges)
 * - **Enumeration:** Index-based selection from string list
 * - **File System:** File paths, directory paths
 * - **Geometry:** Size, Rect, Point (integer and float variants)
 *
 * **Common Use Cases:**
 * - Node parameter configuration (thresholds, kernel sizes)
 * - File input/output selection (image paths, video files)
 * - Enumerated options (filter types, color spaces)
 * - Geometric constraints (ROI dimensions, anchor points)
 *
 * **Property Declaration Example:**
 * @code
 * // In node model class
 * class GaussianBlurNode : public PBNodeDelegateModel {
 * public:
 *     const PropertyVector& properties() const override {
 *         static PropertyVector props = {
 *             // Integer property: kernel size (1-31, odd only via validation)
 *             std::make_shared<TypedProperty<IntPropertyType>>(
 *                 "Kernel Size", "kernel_size", 
 *                 QMetaType::Int,
 *                 IntPropertyType{5, 31, 1}  // value, max, min
 *             ),
 *             
 *             // Enum property: border type selection
 *             std::make_shared<TypedProperty<EnumPropertyType>>(
 *                 "Border Type", "border_type",
 *                 QMetaType::User,
 *                 EnumPropertyType{0, {"Default", "Replicate", "Reflect"}}
 *             )
 *         };
 *         return props;
 *     }
 * };
 * @endcode
 *
 * **Property Browser Integration:**
 * @code
 * // Properties automatically displayed in Qt Property Browser:
 * 
 * Gaussian Blur Node
 *   ├─ Kernel Size: [5] (range: 1-31)
 *   └─ Border Type: [Default ▼]
 *        ├─ Default
 *        ├─ Replicate
 *        └─ Reflect
 * @endcode
 *
 * **Property Access Pattern:**
 * @code
 * // Get property value
 * auto props = getProperties();
 * auto kernelProp = std::dynamic_pointer_cast<TypedProperty<IntPropertyType>>(
 *     props[0]
 * );
 * int kernelSize = kernelProp->getData().miValue;
 * 
 * // Use in computation
 * cv::GaussianBlur(input, output, cv::Size(kernelSize, kernelSize), 0);
 * @endcode
 *
 * @see TypedProperty for template-based property implementation
 * @see PBNodeDelegateModel::properties() for node property access
 * @see QtPropertyBrowserLibrary for UI integration
 */

#pragma once

#include <vector>
#include <memory>
#include <QString>
#include <QVariant>

#if				           \
   defined( linux )     || \
   defined( __linux )   || \
   defined( __linux__ )
       #include <memory>
#endif

/**
 * @struct EnumPropertyType
 * @brief Enumeration property for selecting from a list of named options.
 *
 * Provides a dropdown selection interface in the property browser,
 * storing the current selection as an integer index into a string list.
 *
 * **Members:**
 * - miCurrentIndex: Zero-based index of selected option
 * - mslEnumNames: List of option names for display
 *
 * **Example:**
 * @code
 * // Color space selection
 * EnumPropertyType colorSpace;
 * colorSpace.miCurrentIndex = 0;  // Default to first option
 * colorSpace.mslEnumNames = {"BGR", "RGB", "HSV", "LAB", "GRAY"};
 * 
 * // Use in property
 * auto prop = std::make_shared<TypedProperty<EnumPropertyType>>(
 *     "Color Space", "color_space", QMetaType::User, colorSpace
 * );
 * @endcode
 *
 * **Usage in Node:**
 * @code
 * // Get selected option
 * int colorCode = colorSpaceProp->getData().miCurrentIndex;
 * switch(colorCode) {
 *     case 0: cv::cvtColor(input, output, cv::COLOR_BGR2BGR); break;
 *     case 1: cv::cvtColor(input, output, cv::COLOR_BGR2RGB); break;
 *     case 2: cv::cvtColor(input, output, cv::COLOR_BGR2HSV); break;
 *     // ...
 * }
 * @endcode
 */
typedef struct EnumPropertyType {
    int miCurrentIndex{0};       ///< Current selection index (0-based)
    QStringList mslEnumNames;    ///< List of option names
} EnumPropertyType;

/**
 * @struct DoublePropertyType
 * @brief Double-precision floating-point property with range constraints.
 *
 * Stores a double value with minimum and maximum bounds, displayed as
 * a spinbox or slider in the property browser.
 *
 * **Members:**
 * - mdValue: Current value
 * - mdMax: Maximum allowed value
 * - mdMin: Minimum allowed value
 *
 * **Example:**
 * @code
 * // Gaussian sigma parameter (0.0 to 10.0)
 * DoublePropertyType sigma{1.5, 10.0, 0.0};  // value, max, min
 * 
 * auto prop = std::make_shared<TypedProperty<DoublePropertyType>>(
 *     "Sigma", "sigma", QMetaType::Double, sigma
 * );
 * @endcode
 *
 * **Use Cases:**
 * - Gaussian blur sigma (precision matters)
 * - Threshold values (0.0 - 1.0 normalized)
 * - Scale factors (0.1 - 10.0)
 * - Scientific parameters (high precision)
 */
typedef struct DoublePropertyType {
    double mdValue{0};           ///< Current value
    double mdMax{100};           ///< Maximum value
    double mdMin{0};             ///< Minimum value
} DoublePropertyType;

/**
 * @struct IntPropertyType
 * @brief Integer property with range constraints.
 *
 * Stores an integer value with minimum and maximum bounds, displayed
 * as a spinbox in the property browser.
 *
 * **Members:**
 * - miValue: Current value
 * - miMax: Maximum allowed value
 * - miMin: Minimum allowed value
 *
 * **Example:**
 * @code
 * // Kernel size (must be odd, 1-31)
 * IntPropertyType kernelSize{3, 31, 1};  // value, max, min
 * 
 * auto prop = std::make_shared<TypedProperty<IntPropertyType>>(
 *     "Kernel Size", "kernel_size", QMetaType::Int, kernelSize
 * );
 * @endcode
 *
 * **Use Cases:**
 * - Kernel sizes (blur, morphology)
 * - Iteration counts (erosion/dilation)
 * - Thresholds (0-255 for 8-bit images)
 * - Frame numbers, indices
 */
typedef struct IntPropertyType {
    int miValue{0};              ///< Current value
    int miMax{100};              ///< Maximum value
    int miMin{0};                ///< Minimum value
} IntPropertyType;

/**
 * @struct UcharPropertyType
 * @brief Unsigned char property (0-255) stored as int for QVariant compatibility.
 *
 * Represents unsigned char values but uses int storage to avoid QVariant
 * conversion issues. Always cast to uchar before use in algorithms.
 *
 * **Members:**
 * - mucValue: Current value as int (0-255)
 * - mucMax: Maximum value (always 255)
 * - mucMin: Minimum value (always 0)
 *
 * **Example:**
 * @code
 * // Pixel intensity threshold
 * UcharPropertyType threshold{128, 255, 0};
 * 
 * auto prop = std::make_shared<TypedProperty<UcharPropertyType>>(
 *     "Threshold", "threshold", QMetaType::Int, threshold
 * );
 * @endcode
 *
 * **Usage with Type Casting:**
 * @code
 * // Get property
 * UcharPropertyType& threshProp = getThresholdProperty();
 * 
 * // IMPORTANT: Cast to uchar before use
 * uchar thresholdValue = static_cast<uchar>(threshProp.mucValue);
 * 
 * // Use in OpenCV function
 * cv::threshold(input, output, thresholdValue, 255, cv::THRESH_BINARY);
 * @endcode
 *
 * @warning Always cast mucValue to uchar before passing to functions expecting unsigned char
 * @note Internal storage as int due to QVariant::Int requirement
 */
typedef struct UcharPropertyType {
    int mucValue{0};             ///< Value as int (0-255), cast to uchar before use
    int mucMax{255};             ///< Maximum (255 for uchar)
    int mucMin{0};               ///< Minimum (0 for uchar)
} UcharPropertyType;

/**
 * @struct FilePathPropertyType
 * @brief File path property with file dialog support.
 *
 * Stores a file path with filter and mode information for file selection
 * dialogs. Displayed as a line edit with browse button in property browser.
 *
 * **Members:**
 * - msFilename: Full path to the file
 * - msFilter: File type filter for dialog (e.g., "Images (*.png *.jpg)")
 * - msMode: Dialog mode ("open" for read, "save" for write)
 *
 * **Example:**
 * @code
 * // Image file input
 * FilePathPropertyType inputFile;
 * inputFile.msFilename = "/path/to/image.png";
 * inputFile.msFilter = "Images (*.png *.jpg *.bmp);;All Files (*)";
 * inputFile.msMode = "open";
 * 
 * auto prop = std::make_shared<TypedProperty<FilePathPropertyType>>(
 *     "Input Image", "input_path", QMetaType::User, inputFile
 * );
 * @endcode
 *
 * **File Dialog Filters:**
 * @code
 * // Images only
 * "Images (*.png *.jpg *.bmp)"
 * 
 * // Multiple filter categories
 * "Images (*.png *.jpg);;Videos (*.mp4 *.avi);;All Files (*)"
 * 
 * // OpenCV supported formats
 * "OpenCV Images (*.png *.jpg *.bmp *.tif *.tiff)"
 * @endcode
 *
 * **Modes:**
 * - "open": QFileDialog::getOpenFileName (read existing file)
 * - "save": QFileDialog::getSaveFileName (write/create file)
 */
typedef struct FilePathPropertyType {
    QString msFilename;          ///< Full file path
    QString msFilter;            ///< File dialog filter string
    QString msMode{"open"};      ///< Dialog mode: "open" or "save"
} FilePathPropertyType;

/**
 * @struct PathPropertyType
 * @brief Directory path property with folder dialog support.
 *
 * Stores a directory path, displayed with a browse button to open
 * folder selection dialog.
 *
 * **Member:**
 * - msPath: Full directory path
 *
 * **Example:**
 * @code
 * // Output directory for batch processing
 * PathPropertyType outputDir;
 * outputDir.msPath = "/path/to/output/folder";
 * 
 * auto prop = std::make_shared<TypedProperty<PathPropertyType>>(
 *     "Output Directory", "output_dir", QMetaType::User, outputDir
 * );
 * @endcode
 *
 * **Usage:**
 * @code
 * QString outputPath = pathProp->getData().msPath;
 * QDir outputDir(outputPath);
 * 
 * if (!outputDir.exists()) {
 *     outputDir.mkpath(".");  // Create if doesn't exist
 * }
 * 
 * QString outputFile = outputDir.filePath("result.png");
 * @endcode
 */
typedef struct PathPropertyType {
    QString msPath;              ///< Full directory path
} PathPropertyType;

/**
 * @struct SizePropertyType
 * @brief Integer size property (width × height).
 *
 * Stores 2D dimensions as integer width and height, useful for
 * image sizes, kernel sizes, window dimensions.
 *
 * **Members:**
 * - miWidth: Width in pixels
 * - miHeight: Height in pixels
 *
 * **Example:**
 * @code
 * // Resize target dimensions
 * SizePropertyType targetSize{640, 480};  // 640×480 (VGA)
 * 
 * auto prop = std::make_shared<TypedProperty<SizePropertyType>>(
 *     "Output Size", "output_size", QMetaType::User, targetSize
 * );
 * @endcode
 *
 * **Usage with OpenCV:**
 * @code
 * SizePropertyType& sizeProp = getSizeProperty();
 * cv::Size size(sizeProp.miWidth, sizeProp.miHeight);
 * cv::resize(input, output, size);
 * @endcode
 */
typedef struct SizePropertyType {
    int miWidth{0};              ///< Width in pixels
    int miHeight{0};             ///< Height in pixels
} SizePropertyType;

/**
 * @struct RectPropertyType
 * @brief Integer rectangle property (position + size).
 *
 * Stores a rectangular region as top-left position (x, y) and
 * dimensions (width, height).
 *
 * **Members:**
 * - miXPosition: X coordinate of top-left corner
 * - miYPosition: Y coordinate of top-left corner
 * - miWidth: Rectangle width
 * - miHeight: Rectangle height
 *
 * **Example:**
 * @code
 * // Region of Interest (ROI)
 * RectPropertyType roi{100, 50, 200, 150};  // x, y, w, h
 * 
 * auto prop = std::make_shared<TypedProperty<RectPropertyType>>(
 *     "ROI", "roi", QMetaType::User, roi
 * );
 * @endcode
 *
 * **Usage with OpenCV:**
 * @code
 * RectPropertyType& roiProp = getRoiProperty();
 * cv::Rect roi(roiProp.miXPosition, roiProp.miYPosition,
 *              roiProp.miWidth, roiProp.miHeight);
 * cv::Mat cropped = input(roi);  // Extract ROI
 * @endcode
 */
typedef struct RectPropertyType {
    int miXPosition{0};          ///< X coordinate of top-left corner
    int miYPosition{0};          ///< Y coordinate of top-left corner
    int miWidth{0};              ///< Rectangle width
    int miHeight{0};             ///< Rectangle height
} RectPropertyType;

/**
 * @struct PointPropertyType
 * @brief Integer point property (x, y coordinates).
 *
 * Stores a 2D point with integer coordinates.
 *
 * **Members:**
 * - miXPosition: X coordinate
 * - miYPosition: Y coordinate
 *
 * **Example:**
 * @code
 * // Anchor point for drawing
 * PointPropertyType anchor{320, 240};  // Center of 640×480
 * 
 * auto prop = std::make_shared<TypedProperty<PointPropertyType>>(
 *     "Anchor", "anchor", QMetaType::User, anchor
 * );
 * @endcode
 *
 * **Usage with OpenCV:**
 * @code
 * PointPropertyType& anchorProp = getAnchorProperty();
 * cv::Point anchor(anchorProp.miXPosition, anchorProp.miYPosition);
 * cv::circle(image, anchor, 5, cv::Scalar(0, 255, 0), -1);
 * @endcode
 */
typedef struct PointPropertyType {
    int miXPosition{0};          ///< X coordinate
    int miYPosition{0};          ///< Y coordinate
} PointPropertyType;

/**
 * @struct SizeFPropertyType
 * @brief Floating-point size property (width × height).
 *
 * Stores 2D dimensions as float width and height, useful for
 * normalized sizes, scale factors, or sub-pixel precision.
 *
 * **Members:**
 * - mfWidth: Width (float)
 * - mfHeight: Height (float)
 *
 * **Example:**
 * @code
 * // Scale factor (0.5 = half size)
 * SizeFPropertyType scale{0.5f, 0.5f};
 * 
 * auto prop = std::make_shared<TypedProperty<SizeFPropertyType>>(
 *     "Scale", "scale", QMetaType::User, scale
 * );
 * @endcode
 */
typedef struct SizeFPropertyType {
    float mfWidth{0.};           ///< Width (float)
    float mfHeight{0.};          ///< Height (float)
} SizeFPropertyType;

/**
 * @struct PointFPropertyType
 * @brief Floating-point point property (x, y coordinates).
 *
 * Stores a 2D point with floating-point coordinates for sub-pixel
 * precision or normalized coordinates.
 *
 * **Members:**
 * - mfXPosition: X coordinate (float)
 * - mfYPosition: Y coordinate (float)
 *
 * **Example:**
 * @code
 * // Normalized center point (0.5, 0.5 = image center)
 * PointFPropertyType center{0.5f, 0.5f};
 * 
 * auto prop = std::make_shared<TypedProperty<PointFPropertyType>>(
 *     "Center", "center", QMetaType::User, center
 * );
 * @endcode
 *
 * **Usage:**
 * @code
 * PointFPropertyType& centerProp = getCenterProperty();
 * cv::Point2f center(centerProp.mfXPosition * image.cols,
 *                    centerProp.mfYPosition * image.rows);
 * @endcode
 */
typedef struct PointFPropertyType {
    float mfXPosition{0.};       ///< X coordinate (float)
    float mfYPosition{0.};       ///< Y coordinate (float)
} PointFPropertyType;

/**
 * @class Property
 * @brief Base class for node configuration properties.
 *
 * Abstract base class providing common interface for all property types.
 * Stores property metadata (name, ID, type) and serves as polymorphic base
 * for TypedProperty<T> specializations.
 *
 * **Core Functionality:**
 * - **Metadata Storage:** Name, ID, and Qt meta-type information
 * - **Polymorphic Access:** Virtual methods for property browser integration
 * - **Type Identification:** getType() returns QMetaType::Type
 *
 * **Property Hierarchy:**
 * @code
 * Property (abstract base)
 *   └── TypedProperty<T> (template specialization)
 *         ├── TypedProperty<IntPropertyType>
 *         ├── TypedProperty<DoublePropertyType>
 *         ├── TypedProperty<EnumPropertyType>
 *         └── ... (all property types)
 * @endcode
 *
 * **Usage:**
 * @code
 * // Properties stored polymorphically
 * PropertyVector properties;
 * 
 * properties.push_back(std::make_shared<TypedProperty<IntPropertyType>>(
 *     "Threshold", "threshold", QMetaType::Int, IntPropertyType{128, 255, 0}
 * ));
 * 
 * // Access via base class interface
 * for (auto& prop : properties) {
 *     qDebug() << prop->getName() << prop->getID();
 * }
 * @endcode
 *
 * @see TypedProperty for type-specific implementation
 * @see PropertyVector for property collection
 */
class Property
{
public:
    /**
     * @brief Constructs a property with metadata.
     *
     * @param name Display name shown in property browser
     * @param id Unique identifier for programmatic access
     * @param type Qt meta-type (QMetaType::Int, QMetaType::Double, etc.)
     *
     * **Example:**
     * @code
     * Property("Kernel Size", "kernel_size", QMetaType::Int)
     * @endcode
     */
    Property(const QString & name, const QString & id, int type ) : msName(name), msID(id), miType(type) {}

    /**
     * @brief Virtual destructor for polymorphic deletion.
     */
    virtual ~Property() {}

    /**
     * @brief Returns the display name of the property.
     *
     * @return QString Name shown in property browser
     *
     * **Example:**
     * @code
     * QString name = prop->getName();  // "Kernel Size"
     * @endcode
     */
    QString getName() { return msName; };

    /**
     * @brief Returns the unique identifier of the property.
     *
     * @return QString Property ID for programmatic access
     *
     * **Example:**
     * @code
     * QString id = prop->getID();  // "kernel_size"
     * @endcode
     */
    QString getID() { return msID; };

    /**
     * @brief Returns the Qt meta-type of the property.
     *
     * @return int QMetaType::Type enumeration value
     *
     * **Example:**
     * @code
     * int type = prop->getType();
     * if (type == QMetaType::Int) {
     *     // Handle as integer property
     * }
     * @endcode
     */
    int getType() { return miType; };

private:
    QString msName;   ///< Display name in property browser
    QString msID;     ///< Unique identifier for access
    int miType;       ///< Qt meta-type (QMetaType::Type)
};

/**
 * @class TypedProperty
 * @brief Template class for type-specific property implementation.
 *
 * Extends Property with typed data storage and access. Each property type
 * (IntPropertyType, DoublePropertyType, etc.) is stored in a TypedProperty<T>.
 *
 * **Template Parameter:**
 * @tparam T Property data type (e.g., IntPropertyType, EnumPropertyType)
 *
 * **Core Functionality:**
 * - **Typed Data:** Stores property-specific data structure
 * - **Data Access:** getData() returns mutable reference
 * - **Read-Only Support:** Optional read-only flag for display-only properties
 * - **Sub-Property Text:** Additional text for complex properties
 *
 * **Typical Usage:**
 * @code
 * // Create integer property
 * IntPropertyType data{5, 31, 1};  // value=5, max=31, min=1
 * auto prop = std::make_shared<TypedProperty<IntPropertyType>>(
 *     "Kernel Size",        // Display name
 *     "kernel_size",        // ID
 *     QMetaType::Int,       // Qt type
 *     data,                 // Property data
 *     "",                   // Sub-property text (optional)
 *     false                 // Read-only flag (optional)
 * );
 * 
 * // Access data
 * int kernelSize = prop->getData().miValue;
 * 
 * // Modify data
 * prop->getData().miValue = 7;
 * @endcode
 *
 * **Read-Only Properties:**
 * @code
 * // Create read-only display property
 * IntPropertyType imageWidth{640, 9999, 0};
 * auto prop = std::make_shared<TypedProperty<IntPropertyType>>(
 *     "Image Width", "img_width", QMetaType::Int,
 *     imageWidth, "", true  // Read-only = true
 * );
 * 
 * // Property browser shows value but disables editing
 * @endcode
 *
 * **Sub-Property Text:**
 * @code
 * // Add descriptive text for complex properties
 * EnumPropertyType mode{0, {"Auto", "Manual"}};
 * auto prop = std::make_shared<TypedProperty<EnumPropertyType>>(
 *     "Mode", "mode", QMetaType::User, mode,
 *     "Automatic detection recommended"  // Sub-text
 * );
 * @endcode
 *
 * @see Property for base class
 * @see PropertyVector for property collections
 */
template < typename T >
class TypedProperty : public Property
{
public:
    /**
     * @brief Constructs a typed property with full configuration.
     *
     * @param name Display name in property browser
     * @param id Unique identifier for programmatic access
     * @param type Qt meta-type (QMetaType::Int, QMetaType::Double, etc.)
     * @param data Property data structure (IntPropertyType, etc.)
     * @param sSubPropertyText Optional descriptive text (default: "")
     * @param bReadOnly Read-only flag for display-only properties (default: false)
     *
     * **Example:**
     * @code
     * DoublePropertyType sigmaData{1.5, 10.0, 0.0};
     * auto sigmaProp = std::make_shared<TypedProperty<DoublePropertyType>>(
     *     "Sigma",              // Name
     *     "sigma",              // ID
     *     QMetaType::Double,    // Type
     *     sigmaData,            // Data
     *     "Blur strength",      // Sub-text
     *     false                 // Editable
     * );
     * @endcode
     */
    TypedProperty( const QString & name, const QString & id, int type, const T & data, QString sSubPropertyText = "", bool bReadOnly = false )
        : Property(name, id, type), mData(data), msSubPropertyText(sSubPropertyText), mbReadOnly(bReadOnly) {};

    /**
     * @brief Returns a mutable reference to the property data.
     *
     * @return T& Reference to internal property data structure
     *
     * **Example:**
     * @code
     * // Integer property
     * auto intProp = std::dynamic_pointer_cast<TypedProperty<IntPropertyType>>(prop);
     * IntPropertyType& data = intProp->getData();
     * int value = data.miValue;
     * data.miValue = 10;  // Modify
     * 
     * // Enum property
     * auto enumProp = std::dynamic_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
     * EnumPropertyType& data = enumProp->getData();
     * int selectedIndex = data.miCurrentIndex;
     * QString selectedName = data.mslEnumNames[selectedIndex];
     * @endcode
     *
     * @note Returns mutable reference - changes affect stored data
     */
    T & getData() { return mData; };

    /**
     * @brief Returns optional sub-property text.
     *
     * @return QString Descriptive text or tooltip for property
     *
     * **Example:**
     * @code
     * QString subText = prop->getSubPropertyText();
     * if (!subText.isEmpty()) {
     *     propertyBrowser->setToolTip(subText);
     * }
     * @endcode
     */
    QString getSubPropertyText() { return msSubPropertyText; };

    /**
     * @brief Checks if property is read-only.
     *
     * @return bool True if property cannot be edited, false otherwise
     *
     * **Example:**
     * @code
     * if (prop->isReadOnly()) {
     *     propertyBrowser->setEnabled(false);  // Disable editing
     * }
     * @endcode
     */
    bool isReadOnly() { return mbReadOnly; };

private:
    T mData;                     ///< Property data (type-specific structure)
    QString msSubPropertyText;   ///< Optional descriptive text
    bool mbReadOnly;             ///< Read-only flag (true = display only)
};

/**
 * @typedef PropertyVector
 * @brief Vector of property shared pointers for node configuration.
 *
 * Standard container for storing a node's property collection.
 * Properties are stored polymorphically as shared_ptr<Property>.
 *
 * **Usage:**
 * @code
 * PropertyVector getProperties() const {
 *     PropertyVector props;
 *     
 *     props.push_back(std::make_shared<TypedProperty<IntPropertyType>>(
 *         "Threshold", "threshold", QMetaType::Int,
 *         IntPropertyType{128, 255, 0}
 *     ));
 *     
 *     props.push_back(std::make_shared<TypedProperty<EnumPropertyType>>(
 *         "Mode", "mode", QMetaType::User,
 *         EnumPropertyType{0, {"Binary", "Otsu", "Adaptive"}}
 *     ));
 *     
 *     return props;
 * }
 * @endcode
 *
 * **Iteration:**
 * @code
 * PropertyVector props = node->getProperties();
 * for (auto& prop : props) {
 *     qDebug() << prop->getName() << prop->getID();
 *     
 *     // Type-specific access via dynamic_pointer_cast
 *     if (auto intProp = std::dynamic_pointer_cast<TypedProperty<IntPropertyType>>(prop)) {
 *         qDebug() << "Int value:" << intProp->getData().miValue;
 *     }
 * }
 * @endcode
 *
 * @see Property for base class
 * @see TypedProperty for typed implementation
 */
typedef std::vector< std::shared_ptr<Property> > PropertyVector;
