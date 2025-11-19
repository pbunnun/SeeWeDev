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
 * @file TemplateThreadModel.hpp
 * @brief Template for creating threaded node models with background processing.
 *
 * This file defines the TemplateThreadModel and TemplateThread classes, which demonstrate
 * how to implement background processing in node models using QThread. This pattern is
 * essential for long-running operations that must not block the UI or main processing
 * pipeline.
 *
 * **Purpose:** Reference implementation for threaded node development.
 */

#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>

#include "PBNodeDelegateModel.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class TemplateThread
 * @brief Worker thread template for background processing.
 *
 * This QThread subclass demonstrates the pattern for implementing worker threads
 * in node models. It provides thread lifecycle management, start/stop control,
 * semaphore-based synchronization, and error signaling.
 *
 * **Key Features:**
 * - Safe thread startup and shutdown
 * - Semaphore-based task synchronization
 * - Abort flag for graceful termination
 * - Error signal for reporting issues to main thread
 * - Thread-safe state management
 *
 * **Thread Lifecycle:**
 * 1. Construction: Thread created but not started
 * 2. start_thread(): Begins background execution
 * 3. run(): Continuous processing loop
 * 4. stop_thread(): Sets abort flag
 * 5. Destruction: Waits for thread completion
 *
 * **Use Cases:**
 * - Long-running computations (intensive processing)
 * - Continuous data acquisition (camera, sensors)
 * - Network I/O operations
 * - File system operations (large files)
 * - Database queries
 *
 * @see TemplateThreadModel
 * @see QThread
 */
class TemplateThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a TemplateThread.
     * @param parent Parent QObject (typically the model).
     *
     * Initializes thread with stopped state.
     */
    explicit
    TemplateThread( QObject *parent = nullptr );

    /**
     * @brief Destructor.
     *
     * Sets abort flag, releases semaphore, and waits for thread completion.
     */
    ~TemplateThread() override;

    /**
     * @brief Starts the thread execution.
     *
     * Marks thread as ready and releases semaphore to begin processing.
     */
    void
    start_thread( );

    /**
     * @brief Requests thread to stop.
     *
     * Sets abort flag to signal the run() loop to exit gracefully.
     */
    void
    stop_thread();

Q_SIGNALS:
    /**
     * @brief Signal emitted when an error occurs in the thread.
     * @param Error code or status.
     *
     * Allows the worker thread to report errors to the main thread for
     * handling and UI notification.
     */
    void
    error_signal(int);

protected:
    /**
     * @brief Thread execution loop.
     *
     * Continuously processes tasks while mbAbort is false. Uses semaphore
     * to wait for work. Exits when abort flag is set.
     *
     * **Implementation Pattern:**
     * @code
     * while (!mbAbort) {
     *     mWaitingSemaphore.acquire(); // Wait for signal
     *     if (mbAbort) break;
     *     // Perform work here
     *     if (error) emit error_signal(code);
     * }
     * @endcode
     */
    void
    run() override;

private:
    int miThreadStatus {0};           ///< Thread status code
    bool mbThreadReady {false};       ///< Thread readiness flag
    QSemaphore mWaitingSemaphore;     ///< Synchronization semaphore
    bool mbAbort {false};             ///< Abort flag for graceful shutdown
};

/**
 * @class TemplateThreadModel
 * @brief Template node model demonstrating threaded background processing.
 *
 * This model shows how to integrate a worker thread (TemplateThread) into a node,
 * enabling background processing that doesn't block the UI or data pipeline. It's
 * a reference implementation for creating nodes that perform long-running operations.
 *
 * **Key Features:**
 * - Worker thread management (creation, lifecycle, cleanup)
 * - Error handling from background thread
 * - Thread-safe data passing
 * - Graceful shutdown in destructor
 * - late_constructor() pattern for thread initialization
 *
 * **Threading Model:**
 * - Main thread: UI, property updates, light processing
 * - Worker thread: Heavy computation, blocking I/O
 * - Communication: Signals/slots (thread-safe in Qt)
 *
 * **Thread Safety Considerations:**
 * - Use signals/slots for cross-thread communication
 * - Protect shared data with mutexes if necessary
 * - Avoid direct UI access from worker thread
 * - Copy data before passing to worker thread
 *
 * **Properties (Example):**
 * No specific properties in template, but demonstrates property system integration.
 *
 * **Use Cases:**
 * - Video encoding/decoding (computationally intensive)
 * - Large file I/O operations
 * - Network communication (camera streams, APIs)
 * - Database operations
 * - Machine learning inference (long processing times)
 * - Continuous sensor monitoring
 *
 * **Development Pattern:**
 * 1. Create worker thread class (inherit QThread)
 * 2. Implement run() with main processing loop
 * 3. Add semaphore/signal for task triggering
 * 4. Create thread in model's late_constructor()
 * 5. Pass data to thread via thread-safe methods
 * 6. Handle results via signals back to model
 * 7. Ensure proper cleanup in destructor
 *
 * **Example Customization:**
 * @code
 * class VideoEncoderThread : public QThread {
 *     void run() override {
 *         while (!mbAbort) {
 *             mFrameSemaphore.acquire();
 *             if (!mbAbort) {
 *                 encodeFrame(mCurrentFrame); // Heavy work
 *             }
 *         }
 *     }
 * };
 * 
 * class VideoEncoderModel : public PBNodeDelegateModel {
 *     void setInData(shared_ptr<NodeData> data, PortIndex) {
 *         // Copy frame and signal thread
 *         mpThread->add_frame(frame);
 *     }
 * };
 * @endcode
 *
 * **Error Handling:**
 * - Thread emits error_signal() on failure
 * - Model catches via thread_error_occured() slot
 * - Model can update UI or retry operation
 *
 * @see TemplateThread
 * @see QThread
 * @see VideoWriterModel (real-world threaded example)
 * @see SavingImageThread (another threaded example)
 */
class TemplateThreadModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a TemplateThreadModel.
     *
     * Initializes the model. Thread creation deferred to late_constructor().
     */
    TemplateThreadModel();

    /**
     * @brief Destructor.
     *
     * Stops and deletes the worker thread, ensuring graceful shutdown.
     */
    virtual
    ~TemplateThreadModel() override
    {
        if( mpTemplateThread )
            delete mpTemplateThread;
    }

    /**
     * @brief Saves model state to JSON.
     * @return QJsonObject containing model properties.
     */
    QJsonObject
    save() const override;

    /**
     * @brief Loads model state from JSON.
     * @param p QJsonObject with saved state.
     */
    void
    load(QJsonObject const &p) override;

    /**
     * @brief Returns the number of ports.
     * @param portType Input or Output.
     * @return Port count (customize based on needs).
     */
    unsigned int
    nPorts(PortType portType) const override;

    /**
     * @brief Returns the data type for a specific port.
     * @param portType Input or Output.
     * @param portIndex Port index.
     * @return NodeDataType for the port.
     */
    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Sets input data and potentially triggers thread processing.
     * @param nodeData Input data.
     * @param Port index.
     *
     * Receives input data, processes or forwards to worker thread.
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex ) override;

    /**
     * @brief Sets a model property.
     * @param Property name.
     * @param QVariant value.
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    /**
     * @brief Late constructor for thread initialization.
     *
     * Creates and starts the worker thread after full node construction.
     * This ensures all connections and resources are ready before threading begins.
     */
    void
    late_constructor() override;

    /**
     * @brief Returns nullptr (no embedded widget in template).
     * @return nullptr.
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    static const QString _category;   ///< Node category
    static const QString _model_name; ///< Node display name

private Q_SLOTS:
    /**
     * @brief Slot to handle errors from worker thread.
     * @param Error code.
     *
     * Called when worker thread encounters an error. Can update UI,
     * log error, or attempt recovery.
     */
    void
    thread_error_occured(int);

private:
    TemplateThread * mpTemplateThread { nullptr };  ///< Worker thread instance

    /**
     * @brief Processes input data (main thread or delegates to worker).
     * @param in Input NodeData.
     *
     * Can perform light processing immediately or copy data and signal
     * worker thread for heavy processing.
     */
    void processData(const std::shared_ptr< NodeData > & in );
};
