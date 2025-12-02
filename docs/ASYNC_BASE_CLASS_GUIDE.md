# PBAsyncDataModel Base Class Guide

## Overview

`PBAsyncDataModel` is a base class that extracts common infrastructure from async worker + image pool pattern implementations (CVAdditionModel, CVRGBtoGrayModel, CVGaussianBlurModel, CVFilter2DModel, CVCannyEdgeModel).
It now also centralizes the common frame-completion logic via a public slot `handleFrameReady(std::shared_ptr<CVImageData>)` to reduce duplication across models.

## What It Provides

### Automatic Infrastructure

- ✅ Worker thread lifecycle management (creation, startup, shutdown)
- ✅ CVImagePool management with configurable size/mode
- ✅ Backpressure handling (busy + pending flags)
- ✅ Pool size property (1-128, default 3)
- ✅ Sharing mode property (Pool/Broadcast)
- ✅ Frame counter with auto-increment
- ✅ Graceful shutdown with 3-second timeout
- ✅ Thread-safe pool access with mutex
- ✅ Atomic shutdown flag

### What Derived Classes Must Implement

1. **`createWorker()`** - Factory method to instantiate worker
2. **`connectWorker(QObject*)`** - Connect worker signals to model slots *(cannot be in base - see Qt limitation below)*
3. **`dispatchPendingWork()`** - Dispatch pending work when worker becomes available

Note: The base class provides a public slot `handleFrameReady(...)` that updates the image output, emits `dataUpdated(0)`, schedules a sync `true` on port 1, and then calls `onWorkCompleted()`. Derived classes should connect their worker's `frameReady` signal directly to this base slot.

---

## ⚠️ Qt Signal/Slot Limitation

### Why Signals Cannot Be in Base Class

**CRITICAL**: Qt signals **CANNOT** be inherited from a base class. This is a fundamental limitation of Qt's Meta-Object Compiler (MOC), not a design choice.

#### The Technical Constraint

Qt's MOC generates signal implementation code that is **tightly coupled to the specific class** that declares the signal. When you write:

```cpp
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);
```

MOC generates class-specific metadata and implementation code. This code cannot be shared or inherited across classes.

#### What This Means for PBAsyncDataModel

**❌ This does NOT work:**

```cpp
// In PBAsyncDataModel base class - WILL NOT COMPILE
class PBAsyncWorkerBase : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);  // CANNOT share this!
};

class MyWorker : public PBAsyncWorkerBase {
    // Inheriting signal - Qt MOC will fail
};
```

**✅ This is the required pattern:**

```cpp
// Each worker class MUST declare its own signal
class CVMedianBlurWorker : public QObject {
    Q_OBJECT
public Q_SLOTS:
    void processFrame(...);
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);  // MUST be declared here
};

class CVBilateralFilterWorker : public QObject {
    Q_OBJECT
public Q_SLOTS:
    void processFrame(...);
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);  // MUST be declared here too
};
```

#### Why connectWorker() Must Be Overridden

Because each worker class has its own `frameReady` signal, the base class cannot connect it. Each derived model class must override `connectWorker()` and connect to the base slot:

```cpp
void CVMedianBlurModel::connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVMedianBlurWorker*>(worker);
    if (w) {
        connect(w, &CVMedianBlurWorker::frameReady,
                this, &PBAsyncDataModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}
```

#### Impact on Pattern

While PBAsyncDataModel **cannot** centralize:

- Worker's `frameReady` signal declaration
- Signal connection in `connectWorker()` (each derived class must override)

It **successfully centralizes** (80% of boilerplate):

- Thread lifecycle management
- Pool infrastructure
- Backpressure handling
- Sync signal support
- Property management
- Frame tracking
- Helper methods

**Conclusion**: This is as far as the pattern can be centralized given Qt's architecture. Each derived class needs ~5 lines of boilerplate for signal handling, which is acceptable given the hundreds of lines centralized.

---

## Usage Pattern

### Step 1: Worker Class (in .hpp or .cpp)

```cpp
class MyWorker : public QObject
{
    Q_OBJECT
public:
    MyWorker() = default;

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     MyParameters params,
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId)
    {
        // Validate input
        if (input.empty()) {
            Q_EMIT frameReady(nullptr);
            return;
        }

        // Prepare metadata
        FrameMetadata metadata;
        metadata.producerId = producerId;
        metadata.frameId = frameId;

        auto newImageData = std::make_shared<CVImageData>(cv::Mat());
        bool pooled = false;
        
        // Try pool first - zero-copy optimization
        if (mode == FrameSharingMode::PoolMode && pool) {
            auto handle = pool->acquire(1, metadata);
            if (handle) {
                // Write directly to pool buffer
                cv::yourOperation(input, handle.matrix(), /* params */);
                if (!handle.matrix().empty() && 
                    newImageData->adoptPoolFrame(std::move(handle)))
                    pooled = true;
            }
        }
        
        // Fallback
        if (!pooled) {
            cv::Mat result;
            cv::yourOperation(input, result, /* params */);
            if (result.empty()) {
                Q_EMIT frameReady(nullptr);
                return;
            }
            newImageData->updateMove(std::move(result), metadata);
        }
        Q_EMIT frameReady(newImageData);
    }

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);
};
```

### Step 2: Model Header (.hpp)

```cpp
#pragma once

#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"

// Forward declare worker if in separate file
class MyWorker;

class MyModel : public PBAsyncDataModel
{
    Q_OBJECT

public:
    MyModel();
    ~MyModel() override = default;  // Base handles cleanup

    void late_constructor() override;
    
    // Standard node interface
    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex) override;
    void setModelProperty(QString& id, const QVariant& value) override;
    
    QWidget* embeddedWidget() override { return nullptr; }
    QPixmap minPixmap() const override { return _minPixmap; }

    static const QString _category;
    static const QString _model_name;

protected:
    // Implement base class pure virtuals
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;

// No need to declare handleFrameReady; base provides a public slot.
// Also no need to override inputConnectionCreated/Deleted unless you need custom behavior.

private:
    void process_cached_input();

    // Model-specific state
    MyParameters mParams;
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    std::shared_ptr<SyncData> mpSyncData { nullptr };
    QPixmap _minPixmap;
    
    // Pending data for backpressure
    cv::Mat mPendingFrame;
    MyParameters mPendingParams;
};
```

### Step 3: Model Implementation (.cpp)

```cpp
#include "MyModel.hpp"
#include <QTimer>

// Define worker inline or include from separate file
// #include "MyWorker.hpp"

const QString MyModel::_category = QString("Image Processing");
const QString MyModel::_model_name = QString("My Operation");

MyModel::MyModel()
    : PBAsyncDataModel(_model_name)
    , _minPixmap(":MyIcon.png")
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
    mpSyncData = std::make_shared<SyncData>();

    // Register meta types
    qRegisterMetaType<std::shared_ptr<CVImageData>>("std::shared_ptr<CVImageData>");
    qRegisterMetaType<std::shared_ptr<CVImagePool>>("std::shared_ptr<CVImagePool>");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<FrameSharingMode>("FrameSharingMode");
    qRegisterMetaType<MyParameters>("MyParameters");

    // Add model-specific properties here
    // ...

    // Add pool properties (CALL AT END)
    addPoolProperties();
}

void MyModel::late_constructor()
{
    PBNodeDelegateModel::late_constructor();
    initializeWorkerThread();  // Base class method
}

QObject* MyModel::createWorker()
{
    return new MyWorker();  // Factory method
}

void MyModel::connectWorker(QObject* worker)
{
    auto typedWorker = static_cast<MyWorker*>(worker);
    connect(typedWorker, &MyWorker::frameReady,
            this, &PBAsyncDataModel::handleFrameReady,
            Qt::QueuedConnection);
}

void MyModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    if (!isEnable() || isShuttingDown())
        return;

    if (!nodeData)
        return;

    if (portIndex == 0)  // Image input
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d && !d->data().empty()) {
            mpCVImageInData = d;
            
            // In sync mode, wait for sync signal
            if (mbUseSyncSignal)
                return;
            
            // Process immediately
            process_cached_input();
        }
    }
    else if (portIndex == 1)  // Sync signal
    {
        auto d = std::dynamic_pointer_cast<SyncData>(nodeData);
        if (d && d->data()) {
            if (mpCVImageInData && !mpCVImageInData->data().empty())
                process_cached_input();
        }
    }
}

void MyModel::process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;
    
    cv::Mat input = mpCVImageInData->data();
    
    // Emit sync "false" signal
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });
    
    if (isWorkerBusy()) {
        // Store as pending
        mPendingFrame = input.clone();
        mPendingParams = mParams;
        setPendingWork(true);
    } else {
        // Dispatch immediately
        setWorkerBusy(true);
        
        ensure_frame_pool(input.cols, input.rows, CV_8UC1);  // Adjust output type
        
        long frameId = getNextFrameId();
        QString producerId = getNodeId();
        std::shared_ptr<CVImagePool> poolCopy = getFramePool();
        
        QMetaObject::invokeMethod(mpWorker, "processFrame",
            Qt::QueuedConnection,
            Q_ARG(cv::Mat, input.clone()),
            Q_ARG(MyParameters, mParams),
            Q_ARG(FrameSharingMode, getSharingMode()),
            Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
            Q_ARG(long, frameId),
            Q_ARG(QString, producerId));
    }
}

void MyModel::dispatchPendingWork()
{
    if (!hasPendingWork() || !mpWorker || isShuttingDown())
        return;
    
    cv::Mat input = mPendingFrame.clone();
    MyParameters params = mPendingParams;
    setPendingWork(false);
    
    ensure_frame_pool(input.cols, input.rows, CV_8UC1);  // Adjust output type
    
    long frameId = getNextFrameId();
    QString producerId = getNodeId();
    std::shared_ptr<CVImagePool> poolCopy = getFramePool();
    
    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
        Qt::QueuedConnection,
        Q_ARG(cv::Mat, input),
        Q_ARG(MyParameters, params),
        Q_ARG(FrameSharingMode, getSharingMode()),
        Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
        Q_ARG(long, frameId),
        Q_ARG(QString, producerId));
}

// No per-model handleFrameReady implementation needed. The base slot
// PBAsyncDataModel::handleFrameReady performs the standard completion
// flow (update output, emit, schedule sync true, onWorkCompleted).

void MyModel::setModelProperty(QString& id, const QVariant& value)
{
    // Call base first (handles pool_size and sharing_mode)
    PBAsyncDataModel::setModelProperty(id, value);

    if (!mMapIdToProperty.contains(id))
        return;
    
    // Handle model-specific properties
    if (id == "my_property") {
        // Update mParams...
    }
    
    // Reprocess if input cached
    if (mpCVImageInData && !isShuttingDown()) {
        auto d = mpCVImageInData;
        mpCVImageInData.reset();
        setInData(d, 0);
    }
    mpSyncData->data() = true;
    Q_EMIT dataUpdated(1);
}

// Base class already implements inputConnectionCreated/Deleted to manage
// sync-input behavior; override only if you have custom needs.

// Required for inline worker
#include "MyModel.moc"
```

## Benefits

### Code Reduction

- **Before**: ~200 lines of boilerplate per model
- **After**: ~140-150 lines per model (≈25-30% reduction)

### Consistency

- All async models use identical pool management
- Identical shutdown sequence
- Identical property handling

### Maintainability

- Bug fixes in one place
- Easy to add new features (e.g., performance metrics)
- Clear contract via pure virtual methods

### Type Safety

- Compiler enforces implementation of required methods
- Protected members prevent accidental misuse

## Migration Guide

### Converting Existing Model to Base Class

1. **Change inheritance**:

    ```cpp
   // Before
   class MyModel : public PBNodeDelegateModel
   
   // After
   class MyModel : public PBAsyncDataModel
   ```

2. **Remove duplicate members** from .hpp:

    ```cpp
   // DELETE THESE (now in base):
   QThread mWorkerThread;
   MyWorker* mpWorker { nullptr };
   bool mWorkerBusy { false };
   bool mHasPending { false };
   long mFrameCounter { 0 };
   std::atomic<bool> mShuttingDown { false };
   int miPoolSize { 3 };
   FrameSharingMode meSharingMode { FrameSharingMode::PoolMode };
   std::shared_ptr<CVImagePool> mpFramePool;
   int miPoolFrameWidth { 0 };
   int miPoolFrameHeight { 0 };
   int miActivePoolSize { 0 };
   QMutex mFramePoolMutex;
   ```

3. **Add pure virtual overrides**:

    ```cpp
   protected:
       QObject* createWorker() override;
       void connectWorker(QObject* worker) override;
       void dispatchPendingWork() override;
   ```

4. **Update constructor**:

    ```cpp
   // Change call
   MyModel::MyModel()
       : PBAsyncDataModel(_model_name)  // Was PBNodeDelegateModel
   {
       // ... existing code ...
    // Pool/sharing properties are added by the base automatically.
   }
   ```

5. **Remove destructor** (base handles it):

    ```cpp
   // DELETE entire destructor - base class handles shutdown
   ```

6. **Update late_constructor**:
    The base implements `late_constructor()` to create the worker thread and call your `createWorker()` and `connectWorker()`. You normally do not need to override it unless adding extra late init.

7. **Replace helper methods**:

    ```cpp
   // DELETE these methods (now in base):
   // - ensure_frame_pool()
   // - reset_frame_pool()
   
   // RENAME dispatch_pending() → dispatchPendingWork()
   // (make it override)
   ```

8. **Update variable access**:

    ```cpp
   // Before
   mWorkerBusy = true;
   mFrameCounter++;
   mShuttingDown.load()
   
   // After
   setWorkerBusy(true);
   getNextFrameId()
   isShuttingDown()
   ```

9. **Remove per-model handleFrameReady**:
    Delete any `handleFrameReady` in derived models and connect your worker to `&PBAsyncDataModel::handleFrameReady` instead.

10. **Update setModelProperty**:

    ```cpp
    void MyModel::setModelProperty(QString& id, const QVariant& value)
    {
        // ADD THIS FIRST:
        PBAsyncDataModel::setModelProperty(id, value);
        
        // ... rest of code ...
        
        // DELETE pool_size and sharing_mode handling (now in base)
    }
    ```

## Example: CVRGBtoGrayModel Conversion

See git diff for complete before/after comparison of converting CVRGBtoGrayModel to use PBAsyncDataModel base class.

## Testing Checklist

After conversion:

- [ ] Model compiles without errors
- [ ] Node appears in palette
- [ ] Can create node instance
- [ ] Can process single frame
- [ ] Can process continuous stream
- [ ] Pool properties appear and work
- [ ] Sharing mode switches correctly
- [ ] Shutdown is graceful (no crashes)
- [ ] Sync signal input works
- [ ] Properties persist in .flow file

## Common Pitfalls

### 1. Attempting to Move Signals to Base Class

**Error**: "use of undeclared identifier 'frameReady'" in worker class

**Cause**: Trying to centralize the `frameReady` signal in PBAsyncDataModel base class

**Solution**: **DO NOT ATTEMPT THIS**. Qt's MOC architecture requires signals to be declared in the class that emits them. Each worker class must have its own `Q_SIGNALS` section with `frameReady` declared. This is a Qt framework limitation, not a design choice. See "Qt Signal/Slot Limitation" section above for full explanation.

### 2. Not Overriding connectWorker()

**Error**: Worker signal not connected, no output frames produced

**Cause**: Forgot to override `connectWorker()` in derived class

**Solution**: Every derived model class **must** override connectWorker() to connect its specific worker's signal (cannot be done in base class due to Qt limitation):

```cpp
void CVMedianBlurModel::connectWorker(QObject* worker)
{
    auto* w = qobject_cast<CVMedianBlurWorker*>(worker);
    if (w) {
        connect(w, &CVMedianBlurWorker::frameReady,
                this, &CVMedianBlurModel::handleFrameReady,
                Qt::QueuedConnection);
    }
}
```

**Why this is necessary**: Each worker class has its own `frameReady` signal (Qt MOC requirement), so each model class must connect its specific worker type's signal. The base class cannot provide a generic connection.

### 3. Forgetting qRegisterMetaType

**Error**: "QObject::connect: Cannot queue arguments of type 'cv::Mat'"

**Solution**: Add qRegisterMetaType calls in constructor:

```cpp
qRegisterMetaType<cv::Mat>("cv::Mat");
qRegisterMetaType<CVMedianBlurParameters>("CVMedianBlurParameters");
```

### 4. Forgetting `addPoolProperties()`

**Solution**: Add at END of constructor

### 5. Not calling `PBAsyncDataModel::setModelProperty()`

**Solution**: Must call base first in setModelProperty()

### 6. Accessing base members directly

**Solution**: Use accessor methods instead

### 7. Wrong output type in `ensure_frame_pool()`

**Solution**: Match your operation's output

### 8. Keeping old destructor

**Solution**: Delete it, base handles cleanup

## Future Enhancements

Potential additions to base class:

- Performance metrics (FPS, latency tracking)
- Automatic thread affinity hints
- Built-in frame drop detection
- Pool exhaustion warnings
- Automatic pool size tuning

## Questions?

See existing implementations:

- `CVRGBtoGrayModel` - Simplest example (single input)
- `CVGaussianBlurModel` - With parameters
- `CVAdditionModel` - Multiple inputs
- `CVFilter2DModel` - Complex processing

Or refer to `ASYNC_WORKER_POOL_REFACTORING_GUIDE.md` for detailed pattern documentation.
