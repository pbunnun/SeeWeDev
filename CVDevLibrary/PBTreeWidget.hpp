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
 * @file PBTreeWidget.hpp
 * @brief Custom tree widget for node palette in CVDev visual programming environment.
 *
 * This file defines the PBTreeWidget class, which provides a drag-enabled tree widget
 * for organizing and selecting node types in the node palette. Users can drag node
 * types from the palette onto the graph view to create new nodes.
 *
 * **Key Features:**
 * - **Hierarchical Organization:** Tree structure for node categories
 * - **Drag-and-Drop:** Drag node types to graph view
 * - **Custom Styling:** Support for themed appearance
 * - **Mouse Interaction:** Custom click and drag handling
 *
 * **Common Use Cases:**
 * - Node palette in main window sidebar
 * - Plugin node browser
 * - Node type selection interface
 * - Categorized node library
 *
 * **Integration Pattern:**
 * @code
 * // Setup node palette
 * auto* palette = new PBTreeWidget(parentWidget);
 * palette->setHeaderLabel("Available Nodes");
 * 
 * // Add categories and nodes
 * auto* category = new QTreeWidgetItem(palette);
 * category->setText(0, "Image Processing");
 * 
 * auto* nodeItem = new QTreeWidgetItem(category);
 * nodeItem->setText(0, "GaussianBlur");
 * nodeItem->setData(0, Qt::UserRole, "cv.GaussianBlur");
 * 
 * // User drags nodeItem to PBFlowGraphicsView
 * @endcode
 *
 * **Drag-and-Drop Flow:**
 * 1. User clicks on tree item (mousePressEvent)
 * 2. Drag initiates with node type MIME data
 * 3. dragMoveEvent validates drag operation
 * 4. PBFlowGraphicsView::dropEvent creates node
 *
 * @see PBFlowGraphicsView for drop target
 * @see QTreeWidget for base class
 */

#pragma once

#include "CVDevLibrary.hpp"

#include <QTreeWidget>

/**
 * @class PBTreeWidget
 * @brief Custom tree widget for drag-enabled node palette.
 *
 * Extends QTreeWidget to provide a specialized widget for displaying and organizing
 * node types in a hierarchical tree structure. Users can drag node types from the
 * tree to the graph view to create new instances.
 *
 * **Core Functionality:**
 * - **Tree Display:** Hierarchical organization of node categories and types
 * - **Drag Initiation:** Start drag operations with node type data
 * - **Custom Events:** Handle mouse press and drag move events
 *
 * **Inheritance:**
 * @code
 * QWidget
 *   └── QTreeWidget
 *         └── PBTreeWidget
 * @endcode
 *
 * **Typical Usage:**
 * @code
 * // Create node palette
 * auto* palette = new PBTreeWidget(mainWindow);
 * palette->setHeaderLabel("Node Library");
 * palette->setDragEnabled(true);
 * 
 * // Add category
 * auto* imageCategory = new QTreeWidgetItem();
 * imageCategory->setText(0, "Image Processing");
 * imageCategory->setFlags(Qt::ItemIsEnabled);  // Not draggable
 * palette->addTopLevelItem(imageCategory);
 * 
 * // Add draggable nodes
 * auto* blurNode = new QTreeWidgetItem(imageCategory);
 * blurNode->setText(0, "Gaussian Blur");
 * blurNode->setData(0, Qt::UserRole, "cv.GaussianBlur");  // Node type ID
 * blurNode->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
 * 
 * // User drags blurNode to graph view
 * // -> mousePressEvent() initiates drag
 * // -> MIME data contains "cv.GaussianBlur"
 * // -> PBFlowGraphicsView receives drop
 * @endcode
 *
 * **Drag-and-Drop Integration:**
 * @code
 * // When user drags from palette
 * void PBTreeWidget::mousePressEvent(QMouseEvent* event) {
 *     QTreeWidgetItem* item = itemAt(event->pos());
 *     if (item && item->flags() & Qt::ItemIsDragEnabled) {
 *         QDrag* drag = new QDrag(this);
 *         QMimeData* mimeData = new QMimeData();
 *         
 *         // Store node type identifier
 *         QString nodeType = item->data(0, Qt::UserRole).toString();
 *         mimeData->setText(nodeType);
 *         
 *         drag->setMimeData(mimeData);
 *         drag->exec(Qt::CopyAction);
 *     }
 * }
 * @endcode
 *
 * **Node Organization Patterns:**
 * @code
 * // By plugin
 * + BasicNodes
 *   - ImageLoader
 *   - ImageSaver
 *   - VideoCapture
 * + DNNNodes
 *   - YOLOv5
 *   - Classifier
 * 
 * // By function
 * + Input/Output
 *   - ImageLoader
 *   - VideoCapture
 * + Filters
 *   - GaussianBlur
 *   - MedianFilter
 * + Detection
 *   - YOLOv5
 *   - FaceDetector
 * @endcode
 *
 * **MIME Data Format:**
 * The tree widget uses text MIME data to transfer node type identifiers:
 * - Format: "text/plain"
 * - Content: Node type string (e.g., "cv.GaussianBlur", "io.ImageLoader")
 * - Received by: PBFlowGraphicsView::dropEvent()
 *
 * **Custom Styling:**
 * @code
 * // Apply custom style
 * palette->setStyleSheet(R"(
 *     QTreeWidget {
 *         background-color: #2b2b2b;
 *         color: #ffffff;
 *     }
 *     QTreeWidget::item:hover {
 *         background-color: #3c3c3c;
 *     }
 *     QTreeWidget::item:selected {
 *         background-color: #4a4a4a;
 *     }
 * )");
 * @endcode
 *
 * @see PBFlowGraphicsView for drop event handling
 * @see QTreeWidget for base tree widget functionality
 * @see QTreeWidgetItem for item management
 */
class CVDEVSHAREDLIB_EXPORT PBTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a custom tree widget for node palette.
     *
     * Initializes the tree widget with drag-and-drop support for node creation.
     *
     * @param parent Optional parent widget for Qt ownership hierarchy (default: nullptr)
     *
     * **Example:**
     * @code
     * // Create palette in main window
     * auto* palette = new PBTreeWidget(mainWindow);
     * palette->setHeaderLabel("Available Nodes");
     * mainWindow->addDockWidget(Qt::LeftDockWidgetArea, 
     *                           new QDockWidget(palette));
     * @endcode
     */
    explicit PBTreeWidget(QWidget *parent = nullptr);

protected:
    /**
     * @brief Handles mouse press events to prepare for potential drag operations.
     *
     * Records the mouse press position. Drag only initiates in mouseMoveEvent
     * if the mouse moves beyond the drag start distance threshold.
     *
     * @param event Mouse press event containing button, position, and modifiers
     *
     * @see mouseMoveEvent() for drag initiation
     * @see QTreeWidget::mousePressEvent() for base implementation
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief Handles mouse move events to initiate drag operations.
     *
     * Initiates drag operation only if mouse has moved beyond the system's
     * drag start distance threshold (QApplication::startDragDistance()).
     *
     * @param event Mouse move event containing position and modifiers
     *
     * @see mousePressEvent() for initial press handling
     * @see QTreeWidget::mouseMoveEvent() for base implementation
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief Handles drag move events for custom drag behavior.
     *
     * Processes ongoing drag operations to provide visual feedback or custom
     * drag cursor behavior.
     *
     * @param event Drag move event containing position, modifiers, and mime data
     *
     * @note This can be used to customize drag cursor or provide additional feedback.
     * @note Base implementation usually sufficient for standard drag operations.
     *
     * **Custom Drag Cursor:**
     * @code
     * void PBTreeWidget::dragMoveEvent(QDragMoveEvent* event) {
     *     // Accept drag
     *     event->accept();
     *     
     *     // Custom cursor during drag
     *     setCursor(Qt::DragCopyCursor);
     * }
     * @endcode
     *
     * @see mousePressEvent() for drag initiation
     * @see QTreeWidget::dragMoveEvent() for base implementation
     */
    void dragMoveEvent(QDragMoveEvent *event) override;

private:
    /**
     * @brief Position where mouse was pressed, used for drag distance calculation.
     */
    QPoint mDragStartPosition;
};
