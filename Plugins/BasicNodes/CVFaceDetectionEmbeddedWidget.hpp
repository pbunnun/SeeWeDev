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
 * @file CVFaceDetectionEmbeddedWidget.hpp
 * @brief Embedded widget for face detection classifier selection.
 *
 * This file defines the CVFaceDetectionEmbeddedWidget class, which provides a user interface
 * component for selecting pre-trained Haar cascade classifiers used in face detection.
 * The widget is embedded within the FaceDetectionModel node to allow users to choose
 * between different face detection models (e.g., frontal face, profile face) during runtime.
 */

#pragma once

#include <QWidget>

namespace Ui {
    class CVFaceDetectionEmbeddedWidget;
}

/**
 * @class CVFaceDetectionEmbeddedWidget
 * @brief Widget for selecting Haar cascade classifiers for face detection.
 *
 * This widget provides a combo box interface for choosing between different pre-trained
 * Haar cascade classifiers used in face detection operations. Common options include
 * frontal face, profile face, and other specialized face detection models.
 *
 * The widget is typically embedded in the FaceDetectionModel node and allows users to
 * select the appropriate classifier model without manually loading XML files.
 *
 * **Key Features:**
 * - Combo box with predefined Haar cascade classifier options
 * - Runtime classifier switching without reloading the node
 * - Signal emission on classifier change for model updates
 *
 * **Typical Classifier Options:**
 * - Frontal Face (default): haarcascade_frontalface_default.xml
 * - Frontal Face Alt: haarcascade_frontalface_alt.xml
 * - Profile Face: haarcascade_profileface.xml
 * - Eye Detection: haarcascade_eye.xml
 *
 * **Usage Workflow:**
 * 1. User selects a classifier from the combo box
 * 2. Widget emits button_clicked_signal() with classifier index
 * 3. Parent model loads corresponding Haar cascade XML file
 * 4. Detection proceeds with selected classifier
 *
 * @see FaceDetectionModel
 * @see cv::CascadeClassifier
 */
class CVFaceDetectionEmbeddedWidget : public QWidget {
    Q_OBJECT

    public:
        /**
         * @brief Constructs a CVFaceDetectionEmbeddedWidget.
         * @param parent Parent widget (typically nullptr for embedded widgets).
         *
         * Initializes the UI with a combo box populated with available Haar cascade
         * classifier options. Default selection is typically frontal face detection.
         */
        explicit CVFaceDetectionEmbeddedWidget( QWidget *parent = nullptr );
        
        /**
         * @brief Destructor.
         *
         * Cleans up the UI components allocated during construction.
         */
        ~CVFaceDetectionEmbeddedWidget();

        /**
         * @brief Retrieves the list of available classifier names.
         * @return QStringList containing all classifier options in the combo box.
         *
         * Returns the complete list of Haar cascade classifier options that can
         * be selected by the user. This list is typically populated during widget
         * initialization.
         */
        QStringList get_combobox_string_list();

        /**
         * @brief Sets the selected classifier by name.
         * @param value The classifier name to select (e.g., "Frontal Face").
         *
         * Programmatically sets the combo box selection to the specified classifier.
         * Used when loading saved node configurations to restore the previous selection.
         */
        void set_combobox_value( QString value );

        /**
         * @brief Retrieves the currently selected classifier name.
         * @return QString containing the selected classifier text.
         *
         * Returns the display text of the currently selected classifier option.
         */
        QString get_combobox_text();

    Q_SIGNALS:
        /**
         * @brief Signal emitted when the classifier selection changes.
         * @param button Index of the selected classifier in the combo box.
         *
         * This signal is emitted whenever the user changes the classifier selection,
         * allowing the parent model to reload the appropriate Haar cascade file.
         */
        void button_clicked_signal( int button );

    public Q_SLOTS:
        /**
         * @brief Slot triggered when combo box selection changes.
         * @param Index of the newly selected item.
         *
         * Handles the currentIndexChanged signal from the combo box and emits
         * button_clicked_signal() to notify the parent model of the change.
         */
        void combo_box_current_index_changed( int );

    private:
        Ui::CVFaceDetectionEmbeddedWidget *ui; ///< UI components generated by Qt Designer
};
