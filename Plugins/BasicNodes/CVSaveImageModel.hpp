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
 * @file CVSaveImageModel.hpp
 * @brief Model for saving images to disk with threading support.
 *
 * This file defines the CVSaveImageModel class and SavingImageThread helper class for
 * asynchronous image saving operations. The model supports automatic filename generation,
 * user-provided filenames, sync-triggered saves, and configurable output directories.
 * Threading ensures that disk I/O does not block the main processing pipeline.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QQueue>
#include <QDir>
#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"
#include "SyncData.hpp"
#include "CVImageData.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class SavingImageThread
 * @brief Worker thread for asynchronous image saving operations.
 *
 * This QThread subclass handles disk I/O operations for saving images without blocking
 * the main processing pipeline. It maintains a queue of images and filenames, processing
 * them sequentially in a separate thread using OpenCV's cv::imwrite() function.
 *
 * **Key Features:**
 * - Asynchronous queue-based image saving
 * - Thread-safe image queue with semaphore synchronization
 * - Configurable output directory
 * - Graceful shutdown on destruction
 *
 * **Threading Model:**
 * - Main thread: Enqueues images via add_new_image()
 * - Worker thread: Dequeues and saves images to disk
 * - Semaphore ensures thread waits when queue is empty
 *
 * **Use Case:**
 * High-speed image capture where disk I/O must not slow down frame processing.
 *
 * @see CVSaveImageModel
 * @see cv::imwrite
 */
class SavingImageThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a SavingImageThread.
     * @param parent Parent QObject (typically the CVSaveImageModel).
     */
    explicit
    SavingImageThread( QObject *parent = nullptr );

    /**
     * @brief Destructor.
     *
     * Sets abort flag, releases semaphore, and waits for thread completion.
     */
    ~SavingImageThread() override;

    /**
     * @brief Adds an image to the save queue.
     * @param image cv::Mat image to save (moved into queue).
     * @param filename Full path and filename for the output file.
     *
     * Thread-safe method to enqueue an image for saving. 
     *
     * **Example:**
     * @code
     * thread->add_new_image(cvImage, "/path/to/output/image_0001.jpg");
     * @endcode
     */
    void
    add_new_image( cv::Mat & image, QString filename );

    /**
     * @brief Sets the output directory for saved images.
     * @param dirname Directory path where images will be saved.
     *
     * Configures the base directory. Filenames provided to add_new_image()
     * may include this directory or be relative paths.
     */
    void
    set_saving_directory( QString dirname );

protected:
    /**
     * @brief Thread execution loop.
     *
     * Continuously processes the image queue, saving each image to disk using
     * cv::imwrite(). Blocks on semaphore when queue is empty. Exits when abort
     * flag is set.
     */
    void
    run() override;

private:
    bool mbAbort{false};                    ///< Abort flag for graceful shutdown
    QSemaphore mNoImageSemaphore;           ///< Synchronization for queue availability
    QQueue<cv::Mat> mImageQueue;            ///< Queue of images awaiting save
    QQueue<QString> mFilenameQueue;         ///< Queue of corresponding filenames

    QDir mqDirname;                         ///< Output directory
};

/**
 * @class CVSaveImageModel
 * @brief Node model for saving images to disk with flexible naming options.
 *
 * This model provides comprehensive image saving functionality with automatic filename
 * generation, user-provided naming, sync-triggered operation, and threaded I/O to prevent
 * pipeline blocking. It supports various image formats (JPG, PNG, BMP, etc.) and
 * configurable output directories.
 *
 * **Input Ports:**
 * 1. **CVImageData** - Image to save (required)
 * 2. **InformationData** - Custom filename (optional)
 * 3. **SyncData** - Trigger signal (optional, if mbSyncData2SaveImage=true)
 *
 * **Output Ports:**
 * None (sink node)
 *
 * **Key Features:**
 * - Automatic filename generation with counter (prefix_10000.jpg, prefix_10001.jpg, ...)
 * - User-provided filename support via information port
 * - Sync-triggered saving (save only when sync signal received)
 * - Configurable output directory (default: C:\\ on Windows, ./ on Unix)
 * - Multiple image format support (jpg, png, bmp, tiff, etc.)
 * - Threaded saving to prevent blocking
 * - Queue-based I/O for handling high-speed capture
 *
 * **Filename Generation Modes:**
 * 1. **Automatic (default):** {prefix}_{counter}.{format}
 *    - Example: "image_10000.jpg", "image_10001.jpg"
 *    - Counter increments with each save
 * 2. **User-provided:** Filename from InformationData port
 *    - Full control over naming scheme
 *
 * **Sync-Triggered Mode:**
 * When mbSyncData2SaveImage = true:
 * - Image is saved only when sync signal is active
 * - Allows conditional saving based on external triggers
 * - Use case: Save only when quality threshold met, save on button press, etc.
 *
 * **Properties (Configurable):**
 * - **directory:** Output directory path
 * - **prefix:** Filename prefix for automatic mode
 * - **format:** Image format (jpg, png, bmp, tiff)
 * - **use_provided_filename:** Enable user-provided filenames
 * - **sync_to_save:** Enable sync-triggered saving
 *
 * **Image Format Support:**
 * All formats supported by cv::imwrite():
 * - JPEG (.jpg, .jpeg) - Lossy compression, adjustable quality
 * - PNG (.png) - Lossless compression
 * - BMP (.bmp) - Uncompressed bitmap
 * - TIFF (.tiff, .tif) - Lossless, supports 16-bit depth
 * - WebP (.webp) - Modern lossy/lossless format
 *
 * **Use Cases:**
 * - Save processed video frames for later review
 * - Capture images at regular intervals (with TimerModel)
 * - Export detection results with timestamp filenames
 * - Save only "interesting" frames (with conditional triggers)
 * - Create image datasets from live camera feeds
 * - Archive every Nth frame for quality control
 *
 * **Performance Considerations:**
 * - Threading prevents disk I/O from blocking pipeline
 * - Queue allows burst capture without dropping frames
 * - Consider SSD for high-speed saving (>30 fps)
 * - JPEG compression fastest, PNG slower but lossless
 *
 * **Example Workflow:**
 * @code
 * // Automatic mode: Save every frame with incremental names
 * CVSaveImageModel:
 *   directory: "./output/"
 *   prefix: "frame"
 *   format: "jpg"
 * // Saves: frame_10000.jpg, frame_10001.jpg, ...
 * 
 * // Sync-triggered mode: Save only on sync signal
 * CVSaveImageModel:
 *   sync_to_save: true
 * [Camera] -> [ProcessingNode] -> [SaveImage]
 *                                       ^
 *                                   [SyncGate] (saves when condition met)
 * @endcode
 *
 * @see SavingImageThread
 * @see cv::imwrite
 * @see CVImageData
 * @see InformationData
 * @see SyncData
 */
class CVSaveImageModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a CVSaveImageModel.
     *
     * Initializes the model with default settings (auto-naming, "./" directory,
     * "jpg" format) and creates the saving thread.
     */
    CVSaveImageModel();

    /**
     * @brief Destructor.
     *
     * Deletes the saving thread (which triggers graceful shutdown).
     */
    virtual
    ~CVSaveImageModel() override
    {
        if( mpSavingImageThread )
            delete mpSavingImageThread;
    }

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing all properties.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved properties.
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return 3 for input (image, optional filename, optional sync), 0 for output.
     */
    unsigned int
    nPorts( PortType portType ) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return CVImageData for port 0, InformationData for port 1, SyncData for port 2.
     */
    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Returns output data (none for this sink node).
     * @param Port index.
     * @return nullptr (no outputs).
     */
    std::shared_ptr<NodeData>
    outData(PortIndex) override;

    /**
     * @brief Sets input data and triggers save operation.
     * @param nodeData Input data (CVImageData, InformationData, or SyncData).
     * @param port Port index (0=image, 1=filename, 2=sync).
     *
     * When image data is received:
     * - Checks sync condition (if enabled)
     * - Generates or retrieves filename
     * - Enqueues image for threaded saving
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    /**
     * @brief Returns nullptr (no embedded widget).
     * @return nullptr.
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    /**
     * @brief Late constructor for thread initialization.
     *
     * Creates and starts the SavingImageThread after node construction.
     */
    void
    late_constructor() override;

    /**
     * @brief Sets a model property.
     * @param Property name ("directory", "prefix", "format", etc.).
     * @param QVariant value.
     *
     * **Supported Properties:**
     * - "directory": Output path
     * - "prefix": Filename prefix
     * - "format": Image format (jpg, png, bmp, etc.)
     * - "use_provided_filename": Boolean
     * - "sync_to_save": Boolean
     */
    void
    setModelProperty( QString &, const QVariant & ) override;
    
    /**
     * @brief Indicates if the node is resizable.
     * @return false (fixed size).
     */
    bool
    resizable() const override { return false; }

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private Q_SLOTS:
    /**
     * @brief Handles input connection creation.
     * @param Connection ID.
     */
    void
    inputConnectionCreated(QtNodes::ConnectionId const&) override;

    /**
     * @brief Handles input connection deletion.
     * @param Connection ID.
     */
    void
    inputConnectionDeleted(QtNodes::ConnectionId const&) override;

private:
    SavingImageThread * mpSavingImageThread { nullptr };             ///< Worker thread for saving
    std::shared_ptr< SyncData > mpSyncData{ nullptr };               ///< Sync signal

    cv::Mat mCVMatInImage;                                           ///< Input image matrix
    QString msFilename;                                              ///< Resolved filename to save 

#ifdef _WIN32
    QString msDirname{"C:\\"};                             ///< Default Windows directory
#else
    QString msDirname{"./"};                               ///< Default Unix directory
#endif

    bool mbSyncData2SaveImage{false};                      ///< Save only on sync signal
    int miCounter{10000};                                  ///< Auto-naming counter

    QString msPrefix_Filename {"image"};                   ///< Filename prefix
    QString msImage_Format {"jpg"};                        ///< Output format

    QPixmap _minPixmap;
};

