# Async Worker + Image Pool Refactoring Guide

This guide documents the systematic pattern for refactoring OpenCV processing models to use async worker threads with image pooling, as demonstrated in `CVCannyEdgeModel`, `CVAdditionModel`, `CVRGBtoGrayModel`, and `CVGaussianBlurModel`.
As of the latest refactors, the common completion logic is centralized in `PBAsyncDataModel::handleFrameReady`, so derived models no longer implement their own `handleFrameReady`.

## ✅ Completed Models

- **CVCannyEdgeModel** - Edge detection (with sync signal input)
- **CVAdditionModel** - Image addition with mask (with sync signal input)
- **CVRGBtoGrayModel** - Color space conversion (with sync signal input)
- **CVGaussianBlurModel** - Gaussian blur filtering (with sync signal input)
- **CVFilter2DModel** - Generic 2D convolution (with sync signal input)

## 📋 Remaining High Priority Models

1. **CVSobelAndScharrModel** - Gradient filters
2. **CVMorphologicalTransformationModel** - Morphology operations
3. **CVErodeAndDilateModel** - Erode and dilate
4. **CVThresholdingModel** - Thresholding operations
5. **CVColorSpaceModel** - Color space conversions
6. **CVTemplateMatchingModel** - Template matching
7. **CVConnectedComponentsModel** - Component labeling
8. **CVDistanceTransformModel** - Distance transform
9. **CVWatershedModel** - Watershed segmentation

---

## Pattern Overview

### Key Benefits

- **Non-blocking UI**: Heavy processing moved to worker thread
- **Zero-copy optimization**: OpenCV operations write directly to pre-allocated pool buffers
- **Memory efficiency**: Pre-allocated image pool eliminates per-frame cv::Mat allocations
- **Backpressure handling**: Queues pending frames when worker busy
- **Clean shutdown**: Proper thread lifecycle management
- **Configurable pooling**: Runtime pool size and sharing mode control
- **Sync signal support**: Optional sync input port for synchronized frame processing

### Architecture Components

1. **Worker Class**: QObject living in separate thread, processes frames
2. **Pool Management**: CVImagePool for pre-allocated cv::Mat buffers
3. **Async Dispatch**: QMetaObject::invokeMethod with queued connections
4. **Backpressure Queue**: Pending frame stored when worker busy
5. **Lifecycle**: late_constructor() for initialization, proper destructor cleanup

---

## Step-by-Step Refactoring Process

### STEP 1: Update Header File (.hpp)

#### 1.1 Add Includes

```cpp
#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <atomic>              // ADD THIS (optional - see note on atomics)
#include <QtCore/QMutex>       // ADD THIS
#include <QtCore/QThread>      // ADD THIS

#include "PBNodeDelegateModel.hpp"
#include "CVImageData.hpp"
#include "CVImagePool.hpp"     // ADD THIS
#include "SyncData.hpp"        // ADD THIS for sync signal support
```

#### 1.2 Add Using Declarations

```cpp
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

using CVDevLibrary::FrameSharingMode;  // ADD THIS
using CVDevLibrary::CVImagePool;       // ADD THIS
```

#### 1.3 Add Worker Class (BEFORE Main Model Class)

```cpp
// After the Parameters struct, before the main Model class:
class CV<ModelName>Worker : public QObject
{
    Q_OBJECT
public:
    explicit CV<ModelName>Worker(QObject *parent = nullptr) : QObject(parent) {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                     CV<ModelName>Parameters params,  // Use actual parameter type
                     FrameSharingMode mode,
                     std::shared_ptr<CVImagePool> pool,
                     long frameId,
                     QString producerId);

Q_SIGNALS:
    // CRITICAL: This signal MUST be declared in each worker class
    // CANNOT be inherited from base class due to Qt MOC limitation
    // Qt's Meta-Object Compiler generates class-specific signal code
    // See "Qt Signal/Slot Limitation" section below for details
    void frameReady(std::shared_ptr<CVImageData> img);
};
```

**Note**: For multi-input models (like CVAdditionModel), adapt the processFrame signature:

```cpp
void processFrames(cv::Mat inputA, cv::Mat inputB, cv::Mat inputMask, ...)
```

#### 1.4 Update Destructor Declaration

```cpp
// Change from:
virtual ~CV<ModelName>() override {}

// To:
virtual ~CV<ModelName>() override;
```

#### 1.5 Add late_constructor() Declaration

```cpp
void
late_constructor() override;
```

#### 1.6 Connect to Base Completion Slot

No private slot is needed. You will connect your worker's `frameReady` to the base class slot `PBAsyncDataModel::handleFrameReady` in `connectWorker()`.

#### 1.7 Replace Private Members

**Remove**:

```cpp
void processData(const std::shared_ptr<CVImageData> &in, 
                 std::shared_ptr<CVImageData> &out,
                 const CV<ModelName>Parameters &params);
```

**Add**:

```cpp
private:
    void ensure_frame_pool(int width, int height, int type);
    void reset_frame_pool();
    void dispatch_pending();
    void process_cached_input();  // ADD THIS - extracts common logic

    // Pool configuration
    static constexpr int DefaultPoolSize = 3;
    int miPoolSize { DefaultPoolSize };
    FrameSharingMode meSharingMode { FrameSharingMode::PoolMode };

    // Pool management
    std::shared_ptr<CVImagePool> mpFramePool;
    int miPoolFrameWidth { 0 };
    int miPoolFrameHeight { 0 };
    int miActivePoolSize { 0 };
    QMutex mFramePoolMutex;
    long mFrameCounter { 0 };  // NOTE: Regular long, not atomic (main thread only)

    // Async processing
    QThread mWorkerThread;
    CV<ModelName>Worker* mpWorker { nullptr };
    bool mWorkerBusy { false };
    bool mHasPending { false };
    cv::Mat mPendingFrame;
    CV<ModelName>Parameters mPendingParams;
    std::atomic<bool> mShuttingDown { false };

    // Sync signal support (optional)
    std::shared_ptr<SyncData> mpSyncData { nullptr };
    bool mbUseSyncSignal { false };  // NOTE: Regular bool, not atomic (main thread only)

    // Existing members...
    CV<ModelName>Parameters mParams;
    std::shared_ptr<CVImageData> mpCVImageData { nullptr };
    std::shared_ptr<CVImageData> mpCVImageInData { nullptr };
    QPixmap _minPixmap;
};
```

---

### STEP 2: Update Implementation File (.cpp)

#### 2.1 Implement Worker processFrame()

```cpp
// Add AFTER includes, BEFORE const QString declarations

void CV<ModelName>Worker::processFrame(cv::Mat input,
                                       CV<ModelName>Parameters params,
                                       FrameSharingMode mode,
                                       std::shared_ptr<CVImagePool> pool,
                                       long frameId,
                                       QString producerId)
{
    // Validate input
    if(input.empty() || /* add type checks */)
    {
        Q_EMIT frameReady(nullptr);
        return;
    }

    // Prepare metadata
    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    
    // Try pool mode first - write DIRECTLY to pool buffer (zero-copy optimization)
    if(mode == FrameSharingMode::PoolMode && pool)
    {
        auto handle = pool->acquire(1, metadata);
        if(handle)
        {
            // IMPORTANT: Write directly to pool buffer - avoids temporary allocation + copyTo()
            cv::<OpenCVFunction>(input, handle.matrix(), /* params */);
            if(!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    
    // Fallback: allocate and move (BroadcastMode or pool acquire failed)
    if(!pooled)
    {
        cv::Mat result;
        cv::<OpenCVFunction>(input, result, /* params */);
        if(result.empty())
        {
            Q_EMIT frameReady(nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    Q_EMIT frameReady(newImageData);
}
```

#### 2.2 Update Constructor

**Add AFTER mpCVImageData initialization**:
```cpp
CV<ModelName>::
CV<ModelName>()
    : PBNodeDelegateModel( _model_name ),
      _minPixmap( ":<IconName>.png" )
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());

    // ADD THESE:
    qRegisterMetaType<std::shared_ptr<CVImageData>>("std::shared_ptr<CVImageData>");
    qRegisterMetaType<std::shared_ptr<CVImagePool>>("std::shared_ptr<CVImagePool>");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<FrameSharingMode>("FrameSharingMode");
    qRegisterMetaType<CV<ModelName>Parameters>("CV<ModelName>Parameters");

    // Pool size property
    IntPropertyType poolSizeProperty;
    poolSizeProperty.miMin = 1;
    poolSizeProperty.miMax = 128;
    poolSizeProperty.miValue = miPoolSize;
    QString propId = "pool_size";
    auto propPoolSize = std::make_shared< TypedProperty< IntPropertyType > >( 
        "Pool Size", propId, QMetaType::Int, poolSizeProperty );
    mvProperty.push_back( propPoolSize );
    mMapIdToProperty[ propId ] = propPoolSize;

    // Sharing mode property
    EnumPropertyType sharingModeProperty;
    sharingModeProperty.mslEnumNames = { "Pool Mode", "Broadcast Mode" };
    sharingModeProperty.miCurrentIndex = ( meSharingMode == FrameSharingMode::PoolMode ) ? 0 : 1;
    propId = "sharing_mode";
    auto propSharingMode = std::make_shared< TypedProperty< EnumPropertyType > >( 
        "Sharing Mode", propId, QtVariantPropertyManager::enumTypeId(), sharingModeProperty );
    mvProperty.push_back( propSharingMode );
    mMapIdToProperty[ propId ] = propSharingMode;

    // ... rest of existing properties ...
}
```

#### 2.3 Implement Destructor

```cpp
CV<ModelName>::
~CV<ModelName>()
{
    mShuttingDown.store(true, std::memory_order_release);
    if(mpWorker)
    {
        disconnect(mpWorker, &CV<ModelName>Worker::frameReady, 
                   this, &CV<ModelName>::handleFrameReady);
    }
    mWorkerThread.quit();
    if(!mWorkerThread.wait(3000))
    {
        mWorkerThread.terminate();
        mWorkerThread.wait();
    }
}
```

#### 2.4 Implement late_constructor()

```cpp
void
CV<ModelName>::
late_constructor()
{
    PBNodeDelegateModel::late_constructor();
    
    if(!mpWorker)
    {
        mpWorker = new CV<ModelName>Worker();
        mpWorker->moveToThread(&mWorkerThread);
        connect(mpWorker, &CV<ModelName>Worker::frameReady,
            this, &PBAsyncDataModel::handleFrameReady,
            Qt::QueuedConnection);
        mWorkerThread.start();
    }
}
```

#### 2.5 Replace setInData()

**Remove old synchronous implementation, replace with**:
```cpp
void
CV<ModelName>::
setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if (!isEnable() || mShuttingDown.load(std::memory_order_acquire))
        return;

    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<CVImageData>(nodeData);
        if (d && !d->data().empty())
        {
            mpCVImageInData = d;
            cv::Mat input = d->data();
            
            if(mWorkerBusy)
            {
                mPendingFrame = input.clone();
                mPendingParams = mParams;
                mHasPending = true;
            }
            else
            {
                mWorkerBusy = true;
                
                ensure_frame_pool(input.cols, input.rows, input.type());  // Adjust type as needed
                
                long frameId = mFrameCounter++;  // Simple increment (main thread only)
                QString producerId = getNodeId();
                
                std::shared_ptr<CVImagePool> poolCopy;
                {
                    QMutexLocker locker(&mFramePoolMutex);
                    poolCopy = mpFramePool;
                }
                
                QMetaObject::invokeMethod(mpWorker, "processFrame",
                    Qt::QueuedConnection,
                    Q_ARG(cv::Mat, input.clone()),
                    Q_ARG(CV<ModelName>Parameters, mParams),
                    Q_ARG(FrameSharingMode, meSharingMode),
                    Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
                    Q_ARG(long, frameId),
                    Q_ARG(QString, producerId));
            }
        }
    }
}
```

#### 2.6 Update setModelProperty()

**Add pool property handling at START of function**:
```cpp
void
CV<ModelName>::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    
    // ADD THESE HANDLERS FIRST:
    if( id == "pool_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< IntPropertyType > >( prop );
        int newSize = qMax( 1, qMin(128, value.toInt()) );
        if( miPoolSize == newSize )
            return;

        typedProp->getData().miValue = newSize;
        miPoolSize = newSize;
        reset_frame_pool();
        return;
    }
    else if( id == "sharing_mode" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        int newIndex = qBound( 0, value.toInt(), 1 );
        FrameSharingMode newMode = ( newIndex == 0 ) ? FrameSharingMode::PoolMode : FrameSharingMode::BroadcastMode;
        if( meSharingMode == newMode )
            return;
        
        typedProp->getData().miCurrentIndex = newIndex;
        meSharingMode = newMode;
        {
            QMutexLocker locker( &mFramePoolMutex );
            if(mpFramePool)
                mpFramePool->setMode( meSharingMode );
        }
        if( meSharingMode != FrameSharingMode::PoolMode )
            reset_frame_pool();
        return;
    }
    else if( id == /* existing property */ )
    {
        // ... existing property handlers ...
    }
    
    // REPLACE END with async reprocessing:
    if( mpCVImageInData && !mShuttingDown.load(std::memory_order_acquire) )
    {
        auto d = mpCVImageInData;
        mpCVImageInData.reset();
        setInData(d, 0);
    }
}
```

#### 2.7 Completion Handling

Use the base slot `PBAsyncDataModel::handleFrameReady` which updates outputs, emits `dataUpdated(0)`, schedules a `true` sync on port 1, and calls `onWorkCompleted()` to clear busy and dispatch any pending work. Derived models should not implement their own completion slot unless they have special post-processing needs (in which case call the base slot or replicate its semantics carefully).

```cpp

void
CV<ModelName>::
ensure_frame_pool(int width, int height, int type)
{
    if( width <= 0 || height <= 0 )
        return;

    const int desiredSize = qMax( 1, miPoolSize );
    QMutexLocker locker( &mFramePoolMutex );
    const bool shouldRecreate = !mpFramePool ||
        miPoolFrameWidth != width ||
        miPoolFrameHeight != height ||
        miActivePoolSize != desiredSize;

    if( shouldRecreate )
    {
        mpFramePool = std::make_shared<CVImagePool>( getNodeId(), width, height, type, 
                                                       static_cast<size_t>( desiredSize ) );
        miPoolFrameWidth = width;
        miPoolFrameHeight = height;
        miActivePoolSize = desiredSize;
    }

    if( mpFramePool )
        mpFramePool->setMode( meSharingMode );
}

void
CV<ModelName>::
reset_frame_pool()
{
    QMutexLocker locker( &mFramePoolMutex );
    mpFramePool.reset();
    miPoolFrameWidth = 0;
    miPoolFrameHeight = 0;
    miActivePoolSize = 0;
}

void
CV<ModelName>::
dispatch_pending()
{
    if(!mHasPending || !mpWorker || mShuttingDown.load(std::memory_order_acquire))
        return;
    
    cv::Mat input = mPendingFrame.clone();
    CV<ModelName>Parameters params = mPendingParams;
    mHasPending = false;
    
    ensure_frame_pool(input.cols, input.rows, input.type());  // Adjust type as needed
    
    long frameId = mFrameCounter++;  // Simple increment (main thread only)
    QString producerId = getNodeId();
    
    std::shared_ptr<CVImagePool> poolCopy;
    {
        QMutexLocker locker(&mFramePoolMutex);
        poolCopy = mpFramePool;
    }
    
    mWorkerBusy = true;
    QMetaObject::invokeMethod(mpWorker, "processFrame",
        Qt::QueuedConnection,
        Q_ARG(cv::Mat, input),
        Q_ARG(CV<ModelName>Parameters, params),
        Q_ARG(FrameSharingMode, meSharingMode),
        Q_ARG(std::shared_ptr<CVImagePool>, poolCopy),
        Q_ARG(long, frameId),
        Q_ARG(QString, producerId));
}
```

#### 2.8 Remove Old processData()

**Delete the entire old processData() function** - it's replaced by the worker's processFrame().

---

## Special Cases

### Multi-Input Models (like CVAdditionModel)

- Worker processFrame() takes multiple cv::Mat parameters
- setInData() dispatches all inputs together
- Store multiple pending frames: `mPendingFrameA`, `mPendingFrameB`, etc.

### Models with Complex Parameters

- Ensure parameter struct is copyable
- Register parameter type with qRegisterMetaType
- Pass parameters by value to worker (thread-safe copy)

### Models with Different Output Types

- Adjust ensure_frame_pool() type parameter (CV_8UC1, CV_8UC3, CV_32F, etc.)
- Handle type conversions in worker processFrame()

---

## Verification Checklist

After refactoring each model:

- [ ] Header includes atomic, QMutex, QThread, CVImagePool.hpp
- [ ] Using declarations for CVDevLibrary::FrameSharingMode and CVImagePool
- [ ] Worker class with Q_OBJECT added before main model
- [ ] Destructor declared (not inline)
- [ ] late_constructor() declared
- [ ] Private slot handleFrameReady() added
- [ ] Pool and async members added to private section
- [ ] Old processData() declaration removed

- [ ] Worker processFrame() implemented with pool handling
- [ ] Constructor registers meta types and adds pool properties
- [ ] Destructor implements proper shutdown sequence
- [ ] late_constructor() creates and starts worker thread
- [ ] setInData() implements async dispatch with backpressure
- [ ] setModelProperty() handles pool_size and sharing_mode
- [ ] setModelProperty() triggers async reprocessing at end
- [ ] handleFrameReady() implemented
- [ ] ensure_frame_pool(), reset_frame_pool(), dispatch_pending() implemented
- [ ] Old processData() function removed

- [ ] Model compiles without errors
- [ ] No warnings about unregistered meta types
- [ ] Build succeeds for BasicNodes target

---

## Build and Test

After completing refactoring:

```bash
cd /Users/pbunnun/Projects/CVDev/build
cmake --build . --target BasicNodes
```

Check for:

- No compilation errors
- No linker errors
- MOC runs successfully (worker class has Q_OBJECT)

---

## ⚠️ Qt Signal/Slot Limitation

### Why Each Worker Needs Its Own frameReady Signal

**IMPORTANT**: You **CANNOT** move the `frameReady` signal to a base worker class. This is a fundamental Qt MOC (Meta-Object Compiler) limitation.

#### The Technical Reason

When Qt's MOC sees:

```cpp
class MyWorker : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);
};
```

It generates **class-specific** signal implementation code. This code is tightly coupled to `MyWorker` and cannot be inherited or shared.

#### What Does NOT Work

```cpp
// ❌ WILL NOT COMPILE - signals cannot be inherited
class BaseWorker : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);  // Cannot share this!
};

class MyWorker : public BaseWorker {
    Q_OBJECT  // MOC will fail to generate proper signal code
    // Cannot use inherited frameReady signal
};
```

#### What You MUST Do

```cpp
// ✅ Each worker class must declare its own signal
class CVMedianBlurWorker : public QObject {
    Q_OBJECT
public Q_SLOTS:
    void processFrame(...);
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);  // Required here
};

class CVBilateralFilterWorker : public QObject {
    Q_OBJECT
public Q_SLOTS:
    void processFrame(...);
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);  // Required here too
};
```

#### Impact on Pattern

While you cannot centralize the signal declaration, the async worker + pool pattern still provides massive benefits:

- ✅ 80% of boilerplate centralized (thread, pool, backpressure)
- ✅ Only ~5 lines per worker for signal (acceptable overhead)
- ✅ Pattern remains consistent across all models
- ❌ Cannot eliminate signal boilerplate (Qt limitation)

**Takeaway**: Accept this as a Qt constraint. Each worker needs its Q_SIGNALS section with frameReady. Don't try to work around it.

---

## Threading and Atomic Usage

### Important Note: When Atomics Are NOT Needed

In the CVDev architecture, many state variables do **not** require atomic operations:

- **mbUseSyncSignal**: Regular `bool` is sufficient
  - Only accessed in inputConnectionCreated/Deleted (main thread)
  - Only read in setInData() (main thread)
  
- **mFrameCounter**: Regular `long` is sufficient
  - Only incremented in process_cached_input() and dispatch_pending() (both main thread)
  - Worker receives counter value as a **copy** via Q_ARG(), never accesses the original
  
- **Threading Model**:
  - All model objects live in Qt main/GUI thread
  - Worker objects live in separate threads but receive data via **value copies** (Q_ARG)
  - Worker threads never access model's state variables directly
  - Qt::QueuedConnection ensures slots execute on object's thread (main for models)
  - Qt's event loop provides natural serialization on main thread

**Conclusion**: Use atomics only when variables are accessed from multiple threads. In this pattern, `mShuttingDown` needs atomics (checked from worker callbacks), but `mbUseSyncSignal` and `mFrameCounter` do not.

**Code Pattern**:

```cpp
// Use regular types for main-thread-only access
bool mbUseSyncSignal { false };         // Not atomic
long mFrameCounter { 0 };               // Not atomic

// Use atomic only for cross-thread shutdown flag
std::atomic<bool> mShuttingDown { false };

// Increment is simple
mFrameCounter++;  // Not fetch_add()
```

---

## Zero-Copy Pool Optimization Pattern

### The Performance Win

CVImagePool's biggest benefit comes from **writing OpenCV results directly into pre-allocated pool buffers**, eliminating temporary allocations and `copyTo()` operations.

### ❌ Inefficient Pattern (Old Way)

```cpp
void Worker::processFrame(...)
{
    cv::Mat result;                    // Allocation #1
    cv::GaussianBlur(input, result, size, sigma);
    
    auto handle = pool->acquire(...);
    result.copyTo(handle.matrix());    // Allocation #2 + copy operation
    // ...
}
```

**Cost per frame**: 1 allocation + 1 copy = ~2x memory bandwidth + allocation overhead

### ✅ Optimized Pattern (Zero-Copy)

```cpp
void Worker::processFrame(...)
{
    auto handle = pool->acquire(...);  // Get pre-allocated buffer
    cv::GaussianBlur(input, handle.matrix(), size, sigma);  // Write DIRECTLY to pool
    // ...
}
```

**Cost per frame**: 0 allocations + 0 copies = memory bandwidth only

### Implementation Strategy

**Try pool first, fallback to allocation:**

```cpp
bool pooled = false;
if(mode == FrameSharingMode::PoolMode && pool)
{
    auto handle = pool->acquire(1, metadata);
    if(handle)
    {
        // Write directly to pool buffer
        cv::YourOperation(input, handle.matrix(), params...);
        if(!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
            pooled = true;
    }
}
if(!pooled)
{
    // Fallback: BroadcastMode or pool exhausted
    cv::Mat result;
    cv::YourOperation(input, result, params...);
    if(result.empty())
    {
        Q_EMIT frameReady(nullptr);
        return;
    }
    newImageData->updateMove(std::move(result), metadata);
}
```

### Why This Works

1. **Pool buffers are pre-allocated** at correct size/type during `ensure_frame_pool()`
2. **OpenCV functions write to output Mat** - doesn't matter if it's temporary or pool buffer
3. **handle.matrix()** returns a cv::Mat referencing the pool's memory
4. **No copy needed** - result is already in the pool buffer
5. **Fallback safety** - if pool fails, allocate normally (graceful degradation)

### Performance Impact

For a 1920×1080 RGB image (6.2 MB):

- **Old way**: ~12.4 MB bandwidth (allocate result + copy to pool) + malloc overhead
- **New way**: ~6.2 MB bandwidth (direct write) + zero malloc overhead
- **Savings**: ~50% memory bandwidth + no fragmentation

At 30 FPS, this saves ~186 MB/s of memory traffic and ~30 allocations/sec.

### Critical Pattern Details

**Always check handle validity:**

```cpp
if(handle)  // Pool might be exhausted or in wrong mode
{
    cv::operation(input, handle.matrix(), ...);  // Safe to write
}
```

**Always validate output:**

```cpp
if(!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
    pooled = true;  // Success
```

**Always provide fallback:**

```cpp
if(!pooled)  // Pool failed or BroadcastMode
{
    cv::Mat result;  // Allocate as needed
    // ...
}
```

### Common OpenCV Operations

**All of these can write directly to pool buffers:**

```cpp
// Image processing
cv::GaussianBlur(input, handle.matrix(), size, sigma, ...);
cv::filter2D(input, handle.matrix(), ddepth, kernel, ...);
cv::Canny(input, handle.matrix(), low, high, ...);
cv::cvtColor(input, handle.matrix(), code);

// Arithmetic
cv::add(a, b, handle.matrix(), mask);
cv::subtract(a, b, handle.matrix(), mask);
cv::multiply(a, b, handle.matrix());

// Morphology
cv::erode(input, handle.matrix(), kernel, ...);
cv::dilate(input, handle.matrix(), kernel, ...);
cv::morphologyEx(input, handle.matrix(), op, kernel, ...);

// Transformations
cv::resize(input, handle.matrix(), size, ...);
cv::warpAffine(input, handle.matrix(), M, size, ...);
cv::threshold(input, handle.matrix(), thresh, maxval, type);
```

**Key principle**: If OpenCV function takes `OutputArray dst` parameter, you can write directly to `handle.matrix()`.

---

## Common Issues and Solutions

### Issue: "use of undeclared identifier 'frameReady'" in worker

**Cause**: Attempting to move frameReady signal to a base worker class or forgetting to declare it

**Solution**: **Each worker class MUST have its own Q_SIGNALS section declaring frameReady**. This is a Qt MOC requirement - signals cannot be inherited. See "Qt Signal/Slot Limitation" section above.

```cpp
// ✅ CORRECT - signal declared in worker class
class MyWorker : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);  // MUST be here
};
```

### Issue: "no type named 'FrameSharingMode' in CVImagePool"

**Solution**: Use `using CVDevLibrary::FrameSharingMode;` not `using FrameSharingMode = CVImagePool::FrameSharingMode;`

### Issue: "use of undeclared identifier 'processData'"

**Solution**: Remove all calls to old processData() - they should be replaced by async dispatch in setInData/setModelProperty

### Issue: MOC errors about Q_OBJECT

**Solution**: Ensure worker class is in .hpp file, not .cpp. AUTOMOC handles headers automatically.

### Issue: "cannot pass object of non-trivial type"

**Solution**: Ensure parameter struct is registered with qRegisterMetaType and is copyable.

### Issue: Deadlock or crash on shutdown

**Solution**: Check destructor sets mShuttingDown, disconnects signals, calls quit(), waits with timeout.

---

## Example: Complete CVGaussianBlurModel

See `/Users/pbunnun/Projects/CVDev/Plugins/BasicNodes/CVGaussianBlurModel.hpp` and `.cpp` for a fully working reference implementation, including connecting to the base completion slot.

---

## Progress Tracking

| Model | Status | Notes |
|-------|--------|-------|
| CVCannyEdgeModel | ✅ Complete | Reference implementation + sync signals |
| CVAdditionModel | ✅ Complete | Multi-input reference + sync signals |
| CVRGBtoGrayModel | ✅ Complete | Simple single-input + sync signals |
| CVGaussianBlurModel | ✅ Complete | Verified building + sync signals |
| CVFilter2DModel | ✅ Complete | Full implementation + sync signals |
| CVSobelAndScharrModel | ⏳ Pending | Connect worker to base completion slot |
| CVMorphologicalTransformationModel | ⏳ Pending | Connect worker to base completion slot |
| CVErodeAndDilateModel | ⏳ Pending | - |
| CVThresholdingModel | ⏳ Pending | - |
| CVColorSpaceModel | ⏳ Pending | - |
| CVTemplateMatchingModel | ⏳ Pending | - |
| CVConnectedComponentsModel | ⏳ Pending | - |
| CVDistanceTransformModel | ⏳ Pending | - |
| CVWatershedModel | ⏳ Pending | - |
