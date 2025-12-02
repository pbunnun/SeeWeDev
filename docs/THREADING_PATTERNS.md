# Qt Threading Patterns Guide

This document explains the two threading patterns used in CVDev and provides boilerplate templates for each approach.

---

## Overview

CVDev uses two distinct threading patterns depending on the task nature:

1. **Dedicated QThread Subclass** - For continuous, blocking operations (e.g., video decoding)
2. **QObject Worker + moveToThread()** - For sporadic, event-driven operations (e.g., image processing)

---

## Pattern 1: Dedicated QThread Subclass

### When to Use
- ✅ Task is **continuous** (video decode, sensor polling, network streaming)
- ✅ Need **blocking I/O** (cv::VideoCapture, hardware APIs)
- ✅ Require **precise timing** control (semaphores, sleep intervals)
- ✅ Thread runs for **long duration** (minutes/hours)
- ✅ Simplicity > testability for this specific case

### Example Use Cases
- Video/camera capture
- Serial port monitoring
- Network packet receiver
- Hardware sensor polling

### Pros
- ✅ **Simple for continuous tasks**: Perfect for "run loop" patterns
- ✅ **Low overhead**: Thread is created once and reused continuously
- ✅ **Explicit control**: Direct access to semaphores, abort flags, timing control
- ✅ **No event loop needed**: Blocking operations work naturally
- ✅ **Deterministic lifecycle**: Thread starts/stops with task lifecycle

### Cons
- ❌ **Tight coupling**: Thread class directly references parent model
- ❌ **Less Qt-idiomatic**: Overriding `run()` is discouraged in modern Qt
- ❌ **Manual synchronization**: Must manually manage semaphores, mutexes, abort flags
- ❌ **Harder to test**: Can't easily mock or unit test the thread logic
- ❌ **Single-purpose**: Thread code is specific to one task; can't reuse pattern easily

### Real-World Example
See `CVVideoLoaderModel` and `CVVideoLoaderThread` in `Plugins/BasicNodes/`

---

## Pattern 2: QObject Worker + moveToThread()

### When to Use
- ✅ Task is **event-driven** (process on demand, not continuous loop)
- ✅ Need **flexibility** (multiple operations, reusable worker)
- ✅ Want **testability** (unit test worker independently)
- ✅ Qt integration is priority (signals/slots, event loop)
- ✅ Task is **short-lived** (milliseconds per invocation)

### Example Use Cases
- Image processing nodes (cv::add, filter, threshold)
- Database queries
- File I/O operations
- API requests
- Any "process input → produce output" node

### Pros
- ✅ **Qt-idiomatic**: Follows recommended Qt threading pattern (no `run()` override)
- ✅ **Event-driven**: Uses Qt's event loop for natural signal/slot communication
- ✅ **Loose coupling**: Worker is independent QObject, easily testable
- ✅ **Flexible**: Easy to add more slots/signals without changing thread code
- ✅ **Safe cleanup**: Qt handles queued events during destruction
- ✅ **Reusable pattern**: Worker pattern works for any asynchronous task
- ✅ **Natural threading isolation**: Data passed by value (Q_ARG) creates thread-safe copies

### Cons
- ❌ **Higher overhead**: Each task dispatch involves event queue + context switch
- ❌ **Not for continuous loops**: Inefficient for tight "while(true)" decode loops
- ❌ **Requires event loop**: Must call `exec()` on thread (done automatically by Qt)
- ❌ **Indirect control**: Backpressure/abort requires careful signal design
- ❌ **More boilerplate**: Need separate worker class, more connection code

### Real-World Example
See `CVAdditionModel` and `CVAdditionWorker` in `Plugins/BasicNodes/CVAdditionModel.cpp`

---

## Comparison Table

| Aspect | Dedicated QThread | Worker+moveToThread |
|--------|-------------------|---------------------|
| **Best for** | Continuous loops, I/O blocking | Sporadic event-driven processing |
| **Performance** | Lower latency (direct call) | Higher latency (queued dispatch) |
| **Coupling** | Tight (thread ↔ model) | Loose (worker is independent) |
| **Testability** | Hard | Easy |
| **Flexibility** | Single-purpose | Reusable pattern |
| **Qt Best Practice** | ❌ Discouraged | ✅ Recommended |
| **Cleanup Complexity** | Medium (manual wait) | Low (Qt helps) |

---

## When Atomics Are NOT Needed

### CVDev Worker Pattern Threading Model

In CVDev's worker+moveToThread pattern, many model state variables do **not** require atomic operations:

#### Key Insight: Qt's Architecture Provides Natural Isolation

```
┌─────────────────────┐         ┌──────────────────────┐
│   Main/GUI Thread   │         │   Worker Thread      │
│                     │         │                      │
│  NodeModel          │         │  Worker              │
│  ├─ mbUseSyncSignal │         │  ├─ processFrame()   │
│  ├─ mFrameCounter   │         │  └─ (receives copies)│
│  ├─ setInData()     │──┐      │                      │
│  └─ handleReady()   │  │      │                      │
└─────────────────────┘  │      └──────────────────────┘
                         │
                    Q_ARG(long, frameId)
                    (passes by VALUE)
```

#### Variables That Don't Need Atomics

**1. Connection State Flags** (e.g., `mbUseSyncSignal`, `mbMaskActive`)
- ✅ Only written in inputConnectionCreated/Deleted callbacks (main thread)
- ✅ Only read in setInData() (main thread)
- ✅ Worker never accesses these flags
- **Use**: `bool mbUseSyncSignal { false };` (NOT atomic)

**2. Frame Counters** (e.g., `mFrameCounter`)
- ✅ Only incremented in process_cached_input() and dispatch_pending() (main thread)
- ✅ Worker receives counter value as **copy** via Q_ARG()
- ✅ Worker never accesses the original counter variable
- **Use**: `long mFrameCounter { 0 };` (NOT atomic)
- **Increment**: `mFrameCounter++;` (NOT fetch_add)

**3. Pending State** (e.g., `mWorkerBusy`, `mHasPending`)
- ✅ Only accessed in main thread methods
- ✅ Modified in setInData() and handleFrameReady() (both main thread)
- **Use**: `bool mWorkerBusy { false };` (NOT atomic)

#### Variables That DO Need Atomics

**1. Shutdown Flag** (`mShuttingDown`)
- ❌ Written in destructor (may be any thread)
- ❌ Read in worker callback handleFrameReady() (main thread via queued connection)
- ❌ Potential race during shutdown
- **Use**: `std::atomic<bool> mShuttingDown { false };`

#### Why This Works: Qt's Threading Guarantees

1. **Model Lives in Main Thread**: All model methods execute on Qt main/GUI thread
2. **Worker Lives in Worker Thread**: processFrame() executes in worker thread
3. **Qt::QueuedConnection**: Ensures handleFrameReady() slot executes on main thread (model's thread)
4. **Q_ARG Passes Copies**: Data passed to worker via Q_ARG() is **copied by value**
5. **Event Loop Serialization**: Qt's event loop naturally serializes main thread operations

#### Real-World Example

```cpp
// CVAdditionModel.hpp - CORRECT pattern
class CVAdditionModel : public PBNodeDelegateModel
{
    // ...
private:
    // Main thread only - no atomics needed
    bool mbUseSyncSignal { false };  // ✅ Regular bool
    bool mbMaskActive { false };      // ✅ Regular bool
    long mFrameCounter { 0 };         // ✅ Regular long
    bool mWorkerBusy { false };       // ✅ Regular bool
    
    // Cross-thread access - atomic required
    std::atomic<bool> mShuttingDown { false };  // ✅ Atomic
};

// CVAdditionModel.cpp - CORRECT increment
void CVAdditionModel::process_cached_input()
{
    // ...
    long frameId = mFrameCounter++;  // ✅ Simple increment, not fetch_add
    
    QMetaObject::invokeMethod(mpWorker, "processFrames",
        Qt::QueuedConnection,
        Q_ARG(long, frameId));  // ✅ Passes COPY, not reference
}
```

#### Summary Table

| Variable | Type | Reason |
|----------|------|--------|
| mbUseSyncSignal | `bool` | Main thread only (connection callbacks + setInData) |
| mbMaskActive | `bool` | Main thread only (connection callbacks + setInData) |
| mFrameCounter | `long` | Main thread only, worker gets copy via Q_ARG |
| mWorkerBusy | `bool` | Main thread only (setInData + handleFrameReady) |
| mHasPending | `bool` | Main thread only (setInData + handleFrameReady) |
| mShuttingDown | `std::atomic<bool>` | Accessed during shutdown from destructor |

**Key Takeaway**: Don't use atomics unless you have confirmed cross-thread access. In Qt's worker pattern with Q_ARG, most model state variables are main-thread-only and don't need atomic operations.

---

# Boilerplate Templates

## Template 1: Dedicated QThread Subclass

### Header File (MyWorkerThread.hpp)

```cpp
#pragma once

#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <atomic>

// Forward declaration
class MyNodeModel;

/**
 * @class MyWorkerThread
 * @brief Dedicated thread for continuous task execution
 * 
 * This thread runs a continuous loop performing blocking operations.
 * Use for video capture, sensor polling, or other continuous I/O tasks.
 */
class MyWorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit MyWorkerThread(QObject *parent, MyNodeModel *model);
    ~MyWorkerThread() override = default;

    /**
     * @brief Request thread to abort gracefully
     */
    void request_abort();

    /**
     * @brief Start the continuous task
     * @return true if started successfully
     */
    bool start_task(/* parameters */);

    /**
     * @brief Stop the continuous task
     */
    void stop_task();

Q_SIGNALS:
    /**
     * @brief Emitted when work is completed (if needed)
     */
    void workCompleted(/* result type */);

    /**
     * @brief Emitted on error
     */
    void errorOccurred(const QString& error);

protected:
    /**
     * @brief Main thread loop - override from QThread
     */
    void run() override;

private:
    /**
     * @brief Perform one iteration of work
     */
    void do_work_iteration();

    MyNodeModel* mpModel;           ///< Pointer to parent model
    std::atomic<bool> mbAbort{false}; ///< Abort flag
    std::atomic<bool> mbPaused{false}; ///< Pause flag
    QMutex mMutex;                  ///< Mutex for shared data
    QSemaphore mSemaphore;          ///< Semaphore for pacing
    
    // Task-specific state
    // Add your worker state variables here
};
```

### Implementation File (MyWorkerThread.cpp)

```cpp
#include "MyWorkerThread.hpp"
#include "MyNodeModel.hpp"
#include <QDebug>

MyWorkerThread::MyWorkerThread(QObject *parent, MyNodeModel *model)
    : QThread(parent)
    , mpModel(model)
{
}

void MyWorkerThread::request_abort()
{
    mbAbort.store(true, std::memory_order_release);
    mSemaphore.release(); // Wake thread if waiting
}

bool MyWorkerThread::start_task(/* parameters */)
{
    QMutexLocker locker(&mMutex);
    
    // Initialize your task here
    // e.g., open file, connect to device, etc.
    
    mbAbort.store(false, std::memory_order_release);
    mbPaused.store(false, std::memory_order_release);
    
    // Start the thread if not running
    if (!isRunning()) {
        start();
    }
    
    return true;
}

void MyWorkerThread::stop_task()
{
    QMutexLocker locker(&mMutex);
    
    // Cleanup your task here
    // e.g., close file, disconnect device, etc.
    
    mbPaused.store(true, std::memory_order_release);
}

void MyWorkerThread::run()
{
    while (!mbAbort.load(std::memory_order_acquire))
    {
        if (mbPaused.load(std::memory_order_acquire)) {
            // Wait or sleep when paused
            msleep(10);
            continue;
        }

        // Wait for pacing signal if needed
        // mSemaphore.acquire();
        
        // Check abort again after wait
        if (mbAbort.load(std::memory_order_acquire))
            break;

        // Perform work
        do_work_iteration();
        
        // Optional: sleep for timing control
        // msleep(10);
    }
}

void MyWorkerThread::do_work_iteration()
{
    // Perform your blocking work here
    // Example: read from device, decode frame, etc.
    
    try {
        // Your work code here
        
        // Example: emit result
        // Q_EMIT workCompleted(result);
        
    } catch (const std::exception& e) {
        Q_EMIT errorOccurred(QString::fromStdString(e.what()));
    }
}
```

### Usage in Node Model

```cpp
// In MyNodeModel.hpp
#include "MyWorkerThread.hpp"

class MyNodeModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    MyNodeModel();
    ~MyNodeModel() override;

private:
    MyWorkerThread* mpWorkerThread{nullptr};
};

// In MyNodeModel.cpp
MyNodeModel::MyNodeModel()
{
    mpWorkerThread = new MyWorkerThread(this, this);
    
    connect(mpWorkerThread, &MyWorkerThread::workCompleted,
            this, &MyNodeModel::handleWorkCompleted);
    connect(mpWorkerThread, &MyWorkerThread::errorOccurred,
            this, &MyNodeModel::handleError);
}

MyNodeModel::~MyNodeModel()
{
    if (mpWorkerThread) {
        mpWorkerThread->request_abort();
        mpWorkerThread->wait(3000); // Wait up to 3 seconds
        if (mpWorkerThread->isRunning()) {
            mpWorkerThread->terminate();
            mpWorkerThread->wait();
        }
        delete mpWorkerThread;
        mpWorkerThread = nullptr;
    }
}
```

---

## Template 2: QObject Worker + moveToThread()

### Worker Class (Inline in .cpp or separate file)

```cpp
// In MyNodeModel.cpp or separate MyWorker.hpp

/**
 * @class MyWorker
 * @brief Worker object for event-driven processing
 * 
 * This worker lives on a dedicated thread and processes tasks
 * via queued slot invocations.
 */
class MyWorker : public QObject
{
    Q_OBJECT

public:
    MyWorker() = default;
    ~MyWorker() override = default;

public Q_SLOTS:
    /**
     * @brief Process a task asynchronously
     * @param input Input data (pass by value for thread safety)
     */
    void processTask(/* input parameters by value */)
    {
        try {
            // Perform your work here
            // Example: process image, query database, etc.
            
            // Prepare result
            // auto result = ...;
            
            // Emit result
            Q_EMIT taskCompleted(/* result */);
            
        } catch (const std::exception& e) {
            Q_EMIT errorOccurred(QString::fromStdString(e.what()));
        }
    }

Q_SIGNALS:
    /**
     * @brief Emitted when task completes successfully
     */
    void taskCompleted(/* result type */);

    /**
     * @brief Emitted on error
     */
    void errorOccurred(const QString& error);
};
```

### Node Model Header (MyNodeModel.hpp)

```cpp
#pragma once

#include <QThread>
#include <atomic>
#include "PBNodeDelegateModel.hpp"

// Forward declare worker (if in separate file)
class MyWorker;

class MyNodeModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    MyNodeModel();
    ~MyNodeModel() override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;

private Q_SLOTS:
    /**
     * @brief Handle completed work from worker thread
     */
    void handleTaskCompleted(/* result type */);

private:
    /**
     * @brief Lazily start worker thread if needed
     */
    void start_worker_if_needed();

    /**
     * @brief Dispatch pending work if any
     */
    void dispatch_pending();

    QThread mWorkerThread;
    MyWorker* mpWorker{nullptr};
    bool mWorkerBusy{false};              // Regular bool (main thread only)
    bool mHasPending{false};              // Regular bool (main thread only)
    std::atomic<bool> mShuttingDown{false}; // Atomic (shutdown flag)
    
    // Store pending input (if implementing backpressure)
    // YourInputType mPendingInput;
    
    // Store output
    std::shared_ptr<NodeData> mpOutputData;
};
```

### Node Model Implementation (MyNodeModel.cpp)

```cpp
#include "MyNodeModel.hpp"
#include <QMetaObject>
#include <QMutexLocker>

// Include worker definition (if inline)
#include "MyWorker.hpp" // or define inline above

MyNodeModel::MyNodeModel()
    : PBNodeDelegateModel("My Node")
{
    // Register custom types if needed
    qRegisterMetaType<std::shared_ptr<NodeData>>("std::shared_ptr<NodeData>");
    // Register other custom types as needed
}

MyNodeModel::~MyNodeModel()
{
    // Set shutdown flag first
    mShuttingDown.store(true, std::memory_order_release);
    
    if (mpWorker) {
        // Disconnect signals to prevent callbacks during cleanup
        disconnect(mpWorker, nullptr, this, nullptr);
        
        // Request thread termination
        mWorkerThread.quit();
        
        // Wait with timeout
        if (!mWorkerThread.wait(3000)) {
            // Force terminate if graceful shutdown fails
            mWorkerThread.terminate();
            mWorkerThread.wait();
        }
        
        // Delete worker
        delete mpWorker;
        mpWorker = nullptr;
    }
}

void MyNodeModel::start_worker_if_needed()
{
    if (mpWorker)
        return;
    
    // Create worker
    mpWorker = new MyWorker();
    
    // Move to thread
    mpWorker->moveToThread(&mWorkerThread);
    
    // Connect signals (use Qt::QueuedConnection for cross-thread)
    connect(mpWorker, &MyWorker::taskCompleted,
            this, &MyNodeModel::handleTaskCompleted,
            Qt::QueuedConnection);
    connect(mpWorker, &MyWorker::errorOccurred,
            this, [this](const QString& error) {
                qWarning() << "Worker error:" << error;
            }, Qt::QueuedConnection);
    
    // Start thread
    mWorkerThread.start();
}

void MyNodeModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (!nodeData)
        return;
    
    // Check if shutting down
    if (mShuttingDown.load(std::memory_order_acquire))
        return;
    
    // Extract your input data
    // auto inputData = std::dynamic_pointer_cast<YourDataType>(nodeData);
    // if (!inputData) return;
    
    // Start worker if needed
    start_worker_if_needed();
    
    // Prepare input (copy/clone for thread safety)
    // auto input = inputData->getData().clone();
    
    // Dispatch or queue
    if (mWorkerBusy) {
        // Store as pending (drop old pending)
        // mPendingInput = input;
        mHasPending = true;
    } else {
        // Dispatch to worker
        mWorkerBusy = true;
        QMetaObject::invokeMethod(mpWorker,
                                  "processTask",
                                  Qt::QueuedConnection,
                                  Q_ARG(/* type */, /* input */));
    }
}

void MyNodeModel::handleTaskCompleted(/* result type */)
{
    // Check if shutting down
    if (mShuttingDown.load(std::memory_order_acquire)) {
        mWorkerBusy = false;
        return;
    }
    
    // Update output data
    // mpOutputData = std::make_shared<YourDataType>(result);
    
    // Emit update signal
    Q_EMIT dataUpdated(0);
    
    // Mark not busy
    mWorkerBusy = false;
    
    // Dispatch pending if any
    if (mHasPending) {
        dispatch_pending();
    }
}

void MyNodeModel::dispatch_pending()
{
    if (!mHasPending || !mpWorker || mShuttingDown.load(std::memory_order_acquire))
        return;
    
    // Get pending input
    // auto input = mPendingInput;
    mHasPending = false;
    
    // Dispatch
    mWorkerBusy = true;
    QMetaObject::invokeMethod(mpWorker,
                              "processTask",
                              Qt::QueuedConnection,
                              Q_ARG(/* type */, /* input */));
}

std::shared_ptr<NodeData> MyNodeModel::outData(PortIndex)
{
    return mpOutputData;
}

// Don't forget to include moc if worker is in .cpp
#include "MyNodeModel.moc"
```

---

## Key Differences Summary

### Dedicated QThread Subclass
```cpp
// Thread controls everything
class MyThread : public QThread {
    void run() override {
        while (!abort) {
            blocking_operation(); // Direct call
        }
    }
};

// Usage
thread->start();
thread->request_abort();
thread->wait();
```

### Worker + moveToThread()
```cpp
// Worker is passive
class MyWorker : public QObject {
public slots:
    void process() {
        non_blocking_operation(); // Via event loop
        emit done();
    }
};

// Usage
worker->moveToThread(&thread);
thread.start();
QMetaObject::invokeMethod(worker, "process", Qt::QueuedConnection);
thread.quit();
thread.wait();
```

---

## Recommendations

1. **For new continuous/blocking tasks**: Use Dedicated QThread pattern despite it being less idiomatic
2. **For new event-driven tasks**: Use Worker+moveToThread pattern (Qt recommended)
3. **Don't mix patterns in one class**: Choose one approach per node
4. **Always implement graceful shutdown**: Set flags, wait with timeout, terminate if needed
5. **Test destruction thoroughly**: Run application multiple times to catch shutdown races

---

## References

- Qt Documentation: [Threading Basics](https://doc.qt.io/qt-6/thread-basics.html)
- Qt Blog: [You're doing it wrong](https://www.qt.io/blog/2010/06/17/youre-doing-it-wrong)
- CVDev Examples:
  - `CVVideoLoaderModel` - Dedicated QThread pattern
  - `CVAdditionModel` - Worker+moveToThread pattern
