# NodeEditor v2 to v3 Migration Status

## Overview
This document tracks the migration of CVDev-NodeEditorV3 from NodeEditor version 2 to version 3.

**Date Started:** October 22, 2025  
**Current Status:** In Progress - Phase 2 ~75% Complete
**Strategy**: Option B - Full Rewrite (chosen)

---

## ✅ Completed Tasks

### Phase 1: Setup and Preparation ✅ COMPLETE

#### 1. NodeEditor v3 Integration
- Downloaded official NodeEditor v3 from GitHub into `NodeEditorV3/` directory
- Verified v3 architecture with DataFlowGraphModel, GraphicsView, NodeDelegateModel

#### 2. Build System Updates
- ✅ Updated root `CMakeLists.txt` to add NodeEditorV3 subdirectory
- ✅ Updated `CVDevLibrary/CMakeLists.txt`:
  - Changed include paths to use NodeEditorV3
  - Updated library linkage from `NodeEditor` to `QtNodes`
  - Added migration comments

#### 3. Header File Updates
- ✅ `MainWindow.hpp`: Updated includes and type declarations
  - `#include <FlowScene>` → `#include <QtNodes/DataFlowGraphModel>`
  - Added GraphicsView and related v3 headers
  - `Node*` → `NodeId` for selection tracking
  - Updated function signatures to use NodeId

- ✅ `PluginInterface.hpp`: Updated for v3 registry
  - `DataModelRegistry` → `NodeDelegateModelRegistry`
  - Updated all function signatures
  - Added migration comments

- ✅ `PluginInterface.cpp`: Updated implementations
  - All functions now use `NodeDelegateModelRegistry`

#### 4. Custom Method Commenting
Commented out all custom v2 methods with detailed TODO markers explaining:
- What the method was
- Why it's commented out  
- How to reimplement in v3

**Files Modified:**
- `MainWindow.cpp`: UpdateHistory, Undo, Redo, SetSnap2Grid, lock_position, addAnchor, goToAnchor, cut/copy/paste/delete wrappers
- `PBFlowScene.cpp`: lock_position calls
- `PBFlowView.cpp`: UpdateHistory calls

### Phase 2: Core Class Migration 🔄 ~75% COMPLETE

#### ✅ PBNodeDataModel (COMPLETE)
**Files**: `CVDevLibrary/PBNodeDataModel.hpp`, `CVDevLibrary/PBNodeDataModel.cpp`

**Changes Made:**
- ✅ Changed base class from `NodeDataModel` to `NodeDelegateModel`
- ✅ Added internal state tracking members:
  - `bool mbEnable` - tracks enabled/disabled state
  - `bool mbMinimize` - tracks minimized state  
  - `bool mbLockPosition` - tracks position lock state
  - `bool mbDrawEntries` - tracks entry drawing state
- ✅ Updated `save()` method:
  - Returns `QJsonObject` (v3) instead of taking `QJsonObject&` (v2)
  - Calls `NodeDelegateModel::save()` first to get base data
  - Stores internal state (enable, minimize, lock, draw_entries)
- ✅ Renamed `restore()` → `load()` to match v3 API
  - Takes `QJsonObject const&` parameter
  - Loads internal state from JSON
- ✅ Updated state mutation methods:
  - `setEnable()`, `setMinimize()`, `setLockPosition()`, `setDrawEntries()`
  - Now only update internal bool members (commented out v2 API calls)
- ✅ Updated state getter methods:
  - `enabled()`, `minimized()`, `locked_position()`, `draw_entries()`
  - Now return internal bool members

**Key Design Decision:**
- Custom v2 features (enable, minimize, lock, draw_entries) are now tracked **internally** in PBNodeDataModel
- These states are saved/loaded with node data
- Actual visual representation will need custom painter/geometry implementation later

#### ✅ PBDataFlowGraphModel (NEW - COMPLETE)
**Files**: `CVDevLibrary/PBDataFlowGraphModel.hpp`, `CVDevLibrary/PBDataFlowGraphModel.cpp`

**Purpose**: Replaces `PBFlowScene` functionality for v3

**Implementation:**
- ✅ Inherits from `QtNodes::DataFlowGraphModel`
- ✅ `bool save(QString const& filename)`:
  - Creates QJsonObject with node/connection data
  - Converts to QJsonDocument
  - Writes to file
  - Returns true on success
- ✅ `bool load(QString const& filename)`:
  - Reads file into QByteArray
  - Parses with QJsonDocument::fromJson()
  - Loads data into model
  - Returns true on success
- ✅ Added TODO for node locking implementation
  - v3 uses NodeFlags instead of lock_position() method
  - Need to iterate locked nodes after load and set flags

**Design Pattern:**
- Model-only class (no graphics)
- Graphics handled separately by DataFlowGraphicsScene
- Clean separation of data and presentation

#### ✅ PBFlowScene (COMPATIBILITY LAYER - COMPLETE)
**File**: `CVDevLibrary/PBFlowScene.hpp`

**Changes:**
- ✅ Converted to typedef: `using PBFlowScene = PBDataFlowGraphModel;`
- ✅ Allows existing code to reference PBFlowScene while using new implementation
- ✅ Commented out v2-specific method calls in `.cpp` file

**Rationale:**
- Minimizes changes needed in MainWindow and other code
- Clear migration path: just change the typedef target
- Maintains compatibility while modernizing underlying implementation

#### ✅ PBFlowView (COMPLETE - Basic Migration)
**Files**: `CVDevLibrary/PBFlowView.hpp`, `CVDevLibrary/PBFlowView.cpp`

**Header Changes:**
- ✅ Changed base class: `FlowView` → `GraphicsView`
- ✅ Updated includes to v3 headers
- ✅ Changed `center_on(const Node*)` → `center_on(NodeId)`
- ✅ Added `BasicGraphicsScene* _scene` member for scene access

**Implementation Changes:**
- ✅ **Constructor**: Updated to accept `BasicGraphicsScene*`
- ✅ **dropEvent**: Fully migrated to v3
  - Casts scene to `DataFlowGraphicsScene*`
  - Accesses `graphModel()` through scene
  - Uses `addNode(modelName)` returning `NodeId`
  - Sets position via `setNodeData(nodeId, NodeRole::Position, posView)`
  - Added TODO for late_constructor implementation
  - Commented out UpdateHistory() call
- ✅ **contextMenuEvent**: Simplified placeholder
  - Full implementation deferred (needs complex tree widget menu)
  - Marked with TODO for future work
  - Currently falls through to base QGraphicsView implementation
- ✅ **center_on(NodeId)**: Fully implemented
  - Gets node position from `graphModel.nodeData(nodeId, NodeRole::Position)`
  - Gets node size from `graphModel.nodeData(nodeId, NodeRole::Size)`
  - Calculates center point and calls `centerOn(centerPoint)`
- ✅ **center_on(QPointF)**: Simple pass-through to base `centerOn()`

**Status**: Basic migration complete, contextMenu full implementation deferred

---

## 🔄 In Progress / Remaining Tasks

### Phase 2 Completion (~25% Remaining)

#### ⬜ MainWindow Implementation Migration (HIGH PRIORITY)
**File**: `CVDevLibrary/MainWindow.cpp`

**Header Already Updated** ✅, Implementation Needs:
- [ ] Update `nodeCreated(NodeId)` implementation
  - Currently has signature but uses `Node&` internally
  - Need to access node data through graph model
  - Update `addToNodeTree()` call to use NodeId
  
- [ ] Update `nodeDeleted(NodeId)` implementation  
  - Replace `Node&` parameter with NodeId lookup
  - Update `removeFromNodeTree()` call
  
- [ ] Update `addToNodeTree(NodeId)` implementation
  - Currently takes `Node&`
  - Need to get node data via `graphModel.nodeData(nodeId, ...)`
  - Access delegate model via `graphModel.delegateModel<PBNodeDataModel>(nodeId)`
  
- [ ] Update `removeFromNodeTree(NodeId)` implementation
  - Similar changes to addToNodeTree
  
- [ ] Update `nodeInSceneSelectionChanged()` implementation
  - Get selected nodes as `std::unordered_set<NodeId>` from scene
  - Iterate NodeIds instead of Node pointers
  - Access properties through graph model
  
- [ ] Update `editorPropertyChanged()` implementation
  - Find node by NodeId instead of Node*
  - Access delegate model through graph model
  
- [ ] Update `SceneProperty` structure usage
  - Replace `PBFlowScene*` → `PBDataFlowGraphModel*`
  - Replace `PBFlowView*` → `QtNodes::GraphicsView*` (or keep PBFlowView)
  - Update scene/view creation in `createScene()`
  - Update scene/view deletion in `closeScene()`

**Complexity**: Medium-High  
**Estimated Time**: 3-4 hours

### Phase 3: Plugin System Migration (PENDING)

#### ⬜ Plugin Models Update (MEDIUM PRIORITY)
**Locations**: `Plugins/BasicNodes/`, `Plugins/GUINodes/`

**Required Changes:**
- [ ] All plugin node models inherit from `PBNodeDataModel`
- [ ] Since PBNodeDataModel now inherits from `NodeDelegateModel`, plugins should work automatically
- [ ] Need to verify each plugin model:
  - save()/load() methods compatible with v3
  - Property system works with v3
  - No direct v2 API calls (Node*, FlowScene, etc.)
  
**Testing Strategy:**
1. Test one simple plugin first (e.g., BasicNodes)
2. Verify node creation, deletion, save/load
3. Check property editing works
4. Expand to more complex plugins

**Estimated Time**: 2-3 hours

---

### Phase 4: Feature Restoration (DEFERRED)

These features were commented out during migration. Reimplementation deferred until basic functionality works:

#### ⬜ Undo/Redo System
**Current Status**: Commented out in MainWindow.cpp
**v2 Implementation**: Custom UpdateHistory() in FlowScene
**v3 Solution**: Use Qt's `QUndoStack` and `QUndoCommand`
**Priority**: Medium
**Estimated Time**: 4-6 hours

#### ⬜ Snap to Grid
**Current Status**: Commented out (SetSnap2Grid)
**v2 Implementation**: Custom in FlowScene
**v3 Solution**: Override mouseMoveEvent in GraphicsView or custom NodeGeometry
**Priority**: Low
**Estimated Time**: 1-2 hours

#### ⬜ Node Position Locking
**Current Status**: Commented out (lock_position calls)
**v2 Implementation**: Custom flag in NodeGraphicsObject
**v3 Solution**: Use NodeFlags::Locked in graph model
**Priority**: Medium
**Estimated Time**: 2-3 hours

#### ⬜ View Anchors/Bookmarks  
**Current Status**: Commented out (addAnchor, goToAnchor)
**v2 Implementation**: Custom in FlowView
**v3 Solution**: Store QTransform states, custom implementation
**Priority**: Low
**Estimated Time**: 2-3 hours

#### ⬜ Cut/Copy/Paste Operations
**Current Status**: Commented out wrapper methods
**v2 Implementation**: Built into FlowView
**v3 Solution**: Implement clipboard operations with node serialization
**Priority**: High (user-facing feature)
**Estimated Time**: 3-4 hours

#### ⬜ Context Menu Full Implementation
**Current Status**: Simplified placeholder in PBFlowView
**v2 Implementation**: Tree widget with filterable node list
**v3 Solution**: Reimplement using v3 registry and addNode() pattern
**Priority**: High (user-facing feature)  
**Estimated Time**: 2-3 hours

#### ⬜ Enable/Disable Node Visual Feedback
**Current Status**: State tracked in PBNodeDataModel internally
**v2 Implementation**: Visual styling changes
**v3 Solution**: Custom NodePainter or StyleCollection
**Priority**: Medium
**Estimated Time**: 2-3 hours

#### ⬜ Minimize Node Visual Feedback  
**Current Status**: State tracked in PBNodeDataModel internally
**v2 Implementation**: Custom geometry changes
**v3 Solution**: Custom NodeGeometry delegation
**Priority**: Medium
**Estimated Time**: 3-4 hours

---

## 📋 Testing Checklist

### Compilation Test
- [ ] Project compiles without errors
- [ ] No v2 API calls remaining
- [ ] All includes resolve correctly
- [ ] Plugins build successfully

### Basic Functionality
- [ ] Application launches
- [ ] Main window displays
- [ ] Node palette/menu accessible
- [ ] Can create nodes
- [ ] Can delete nodes
- [ ] Can create connections
- [ ] Can delete connections

### Data Persistence
- [ ] Can save graph to file
- [ ] Can load graph from file
- [ ] Node properties preserved
- [ ] Connection data preserved
- [ ] Custom node states (enable, minimize, etc.) preserved

### Plugin System
- [ ] Plugins load successfully
- [ ] Plugin nodes appear in menu
- [ ] Plugin nodes function correctly
- [ ] Plugin-specific properties work

### Property System
- [ ] Property browser displays node properties
- [ ] Can edit properties
- [ ] Property changes affect node behavior
- [ ] Property changes saved/loaded

### User Interaction
- [ ] Can select nodes
- [ ] Can move nodes (if not locked)
- [ ] Can zoom view
- [ ] Can pan view
- [ ] Keyboard shortcuts work

---

## 🔑 Key Architecture Changes

### v2 → v3 Mapping

| NodeEditor v2 | NodeEditor v3 | Notes |
|---------------|---------------|-------|
| `FlowScene` | `DataFlowGraphModel` + `DataFlowGraphicsScene` | Model-view separation |
| `FlowView` | `GraphicsView` | Simplified view class |
| `NodeDataModel` | `NodeDelegateModel` | Renamed, similar concept |
| `DataModelRegistry` | `NodeDelegateModelRegistry` | Renamed |
| `Node*` (pointer) | `NodeId` (unsigned int) | No direct node access |
| `Node::nodeGraphicsObject()` | Handled by scene | Graphics managed separately |
| `nodeDataModel()` | `delegateModel(nodeId)` | Access through model |

### Custom Features Status

| Feature | v2 Implementation | v3 Status | Migration Plan |
|---------|-------------------|-----------|----------------|
| Undo/Redo | Custom history in FlowScene | Not built-in | Use QUndoStack |
| Snap to Grid | Custom in FlowScene | Not built-in | Implement in view |
| Lock Position | Custom in NodeGraphicsObject | Not built-in | Use NodeFlags |
| Enable/Disable | Custom property | Not built-in | Use validation state |
| Minimize Node | Custom property | Not built-in | Custom painter/geometry |
| View Anchors | Custom in FlowView | Not built-in | Store transforms |
| Draw Entries | Custom property | Different | Use geometry/painter |

---

## ⚠️ Known Issues

1. **Compilation will fail** until Phase 2 is complete
   - v2 and v3 APIs are incompatible
   - Cannot mix FlowScene with DataFlowGraphModel

2. **Plugin compatibility**
   - All existing plugins use PBNodeDataModel (v2)
   - Will break until PBNodeDataModel is migrated

3. **Custom features**
   - Many custom features added to v2 don't exist in v3
   - Need alternative implementations

---

## 📝 Notes for Developers

### Important Decisions Needed

1. **Migration Strategy**
   - Should we maintain both v2 and v3 temporarily?
   - Create an adapter layer or do full rewrite?
   - Timeline/priority for completion?

2. **Feature Parity**
   - Which custom features are essential?
   - Which can be deferred or removed?
   - Are there v3 alternatives we should use?

### Code Markers

All migration-related changes are marked with:
```cpp
// TODO NodeEditor v3 Migration: [description]
```

Search for this pattern to find all migration points.

### Testing Strategy

1. Start with minimal viable migration
2. Get basic node graph working
3. Add features incrementally
4. Test with one plugin first
5. Expand to all plugins

---

## 📚 References

- [NodeEditor v3 Documentation](https://qtnodes.readthedocs.io/)
- [NodeEditor v3 GitHub](https://github.com/paceholder/nodeeditor)
- [NodeEditor v3 Examples](NodeEditorV3/examples/)
- v3 uses Model-View architecture similar to Qt's MVC pattern

---

## Timeline Estimate

- **Phase 1** (Completed): 2-3 hours - Setup and preparation
- **Phase 2** (In Progress): 8-12 hours - Core class migration
- **Phase 3** (Pending): 4-6 hours - Testing and refinement

**Total Estimated**: 14-21 hours of focused development time

---

*Last Updated: October 22, 2025*
