# Node lifecycle & deferred initialization

This document explains the node lifecycle used by the NodeEditor integration in CVDev
and why heavy initialization (threads, hardware, network) is centralized in the
graph model via `late_constructor()`.

## Overview

- **Interactive creation**
  1. User requests a new node (UI/menu)
  2. `DataFlowGraphModel::addNode(nodeType)` creates the delegate via the registry,
     connects basic signals, stores the delegate, and emits `nodeCreated(nodeId)`.
  3. `PBDataFlowGraphModel::addNode()` performs post-create steps (connects
     `embeddedWidgetSizeUpdated`) and calls `delegateModel->late_constructor()` so
     heavy initialization only occurs when a node is actually in the scene.

- **Load-from-file**
  1. `PBDataFlowGraphModel::load_from_file()` reads JSON and calls `load()`.
  2. `DataFlowGraphModel::loadNode(nodeJson)` creates the delegate for the saved id,
     emits `nodeCreated(restoredNodeId)`, sets node position, and calls `model->load(internalDataJson)`.
  3. After the base `loadNode()` returns, `PBDataFlowGraphModel::loadNode()` performs
     post-load steps: connects `embeddedWidgetSizeUpdated`, calls
     `delegateModel->late_constructor()`, restores the embedded widget size and
     re-applies it with a `QTimer::singleShot(0, ...)` to handle immediate resizes
     triggered by the model's initialization.

## Why centralize `late_constructor()`

- The registry (used to populate node menus) creates lightweight instances or
  descriptors for all registered node types. If those instances started threads or
  opened hardware/drivers during registry-time, the application would start heavy
  work before the user ever places a node into a scene.
- Centralizing deferred initialization in the graph model ensures heavy work only
  runs when a node actually exists in a scene (either created interactively or
  restored from disk). It also centralizes ordering so the code can: connect UI
  signals → restore widget size → then start threads/hardware.

## Best practices for implementers

- Put thread start / device open / long-running work in `late_constructor()`.
- Make `late_constructor()` idempotent. The graph model may call it for both add
  and load paths. Example pattern:

```cpp
void MyModel::late_constructor() override {
    if (mbLateConstructed) return;
    mbLateConstructed = true;
    // start threads, open device, connect signals, etc.
}
```

- Avoid performing heavy synchronous UI changes from background threads during
  initialization. If the model needs to resize its embedded widget during
  initialization, do so on the GUI thread and keep the resize stable so
  `PBDataFlowGraphModel`'s re-assert logic can restore saved sizes.

## Common pitfalls

- Calling `late_constructor()` in multiple places without a guard can start
  duplicate threads. Use the idempotent pattern above.
- Code that assumes `nodeCreated` happens after `load()` may observe different
  ordering — the NodeEditor base emits `nodeCreated` before calling `load()`,
  and `PBDataFlowGraphModel::loadNode()` executes additional post-load steps
  after the base call returns.

## Where to look in the code

- `NodeEditor/src/DataFlowGraphModel.cpp` — base `addNode()` and `loadNode()`
- `CVDevLibrary/PBDataFlowGraphModel.cpp` — application-level post-create and
  post-load steps (connect `embeddedWidgetSizeUpdated`, `late_constructor()`,
  widget-size restore)

---

## Developer checklist: migrating an existing model to the idempotent pattern

If you maintain node models that previously performed heavy initialization in
constructors or during registry instantiation, follow this checklist to migrate
them safely to the centralized `late_constructor()` pattern:

1. Identify heavy init points
   - Search for thread creation, device open calls, network clients, or long
     running tasks in the model's constructor or global/static initializers.

2. Move heavy init into `late_constructor()`
   - Remove thread/device startup from the constructor and place it inside
     `late_constructor()`.

3. Make `late_constructor()` idempotent
   - Add a boolean guard (e.g., `mbLateConstructed`) that prevents duplicate
     initialization. Example:

```cpp
class MyModel : public PBNodeDelegateModel {
    bool mbLateConstructed{false};
public:
    void late_constructor() override {
        if (mbLateConstructed) return;
        mbLateConstructed = true;
        // Heavy initialization here
    }
};
```

4. Ensure `load()` still restores state
   - Keep `load(QJsonObject const &p)` responsible for restoring saved properties
     (file paths, parameters, etc.). Do not start threads in `load()`; instead
     restore state and let `late_constructor()` start runtime activity.

5. Avoid GUI work in worker threads
   - If initialization needs to resize widgets or update UI, perform those
     actions on the GUI thread (signals/slots) to avoid races with the scene's
     geometry recalculation and saved-size re-apply.

6. Test both paths
   - Interactive creation: add node from menu and verify it initializes once.
   - Load from file: save a graph with the node, reload, and verify widget size
     and that the node's runtime starts once.

7. Optional: add a unit or integration test
   - Create a small smoke test that calls `PBDataFlowGraphModel::addNode()` and
     `loadNode()` for the model and ensures no duplicate threads or errors.

---

## Common migration examples (before → after)

### Example 1 — camera model (before)

```cpp
// In constructor (bad):
mpCameraThread = new CVCameraThread(...);
mpCameraThread->start();
```

### Example 1 — camera model (after)

```cpp
// In constructor: create objects but do not start threads
mpCameraThread = nullptr;

void CVCameraModel::late_constructor() {
    if (mbLateConstructed) return;
    mbLateConstructed = true;
    mpCameraThread = new CVCameraThread(this, mpCVImageData);
    mpCameraThread->start();
}
```

### Example 2 — VDO loader (before)

```cpp
// In load() or constructor (bad): open video and start decoding thread
mpVideoCapture.open(filename);
mpVDOLoaderThread = new CVVDOLoaderThread(...);
mpVDOLoaderThread->start();
```

### Example 2 — VDO loader (after)

```cpp
void CVVDOLoaderModel::load(QJsonObject const &p) {
    PBNodeDelegateModel::load(p);
    // restore filename and parameters but do not start thread
    msVideoFilename = p["cParams"].toObject()["filename"].toString();
}

void CVVDOLoaderModel::late_constructor() {
    if (mbLateConstructed) return;
    mbLateConstructed = true;
    mpVDOLoaderThread = new CVVDOLoaderThread(this, mpCVImageData);
    if (!msVideoFilename.isEmpty()) {
        mpVDOLoaderThread->open_video(msVideoFilename);
    }
}
```

## Notes and troubleshooting

- If you observe widget sizes being overridden during load, ensure your model's
  initialization doesn't immediately resize the embedded widget synchronously.
  Prefer emitting a signal to request a GUI resize and let `PBDataFlowGraphModel`
  re-apply the saved size via the single-shot timer.
- If a model must start background work during `load()` (rare), make sure the
  work is guarded and that the worker won't trigger UI updates directly from
  non-GUI threads.

## Contact

If you need help migrating a specific model, point me at the file and I can
propose a minimal patch that follows the pattern above.

