//Copyright © 2025 - 2026, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>

class PythonEditorEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PythonEditorEmbeddedWidget(QWidget* parent = nullptr);

    QString getPythonCode() const;
    void setPythonCode(const QString& code);
    
    int getNumInputs() const { return mpNumInputsSpinBox->value(); }
    void setNumInputs(int num) { mpNumInputsSpinBox->setValue(num); }
    
    int getNumOutputs() const { return mpNumOutputsSpinBox->value(); }
    void setNumOutputs(int num) { mpNumOutputsSpinBox->setValue(num); }

Q_SIGNALS:
    void executeClicked();
    void numInputsChanged();
    void numOutputsChanged();
    void widgetClicked(); // Signal to request node selection
    void editableWidgetSelectedChanged(bool isSelected);

protected:
    /**
     * @brief Event filter to capture focus events on child widgets.
     * @param obj The object receiving the event.
     * @param event The event being processed.
     * @return true if the event is handled, false to continue default processing.
     *
     * This filter detects when the code editor or spin boxes gain or lose focus,
     * emitting signals to notify the parent model about selection changes.
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

    /**
     * @brief Handles mouse press events to emit widgetClicked signal.
     * @param event Mouse event information.
     *
     * Overrides the default mousePressEvent to emit a signal when the widget
     * is clicked. This allows the parent model to respond by selecting or
     * focusing the corresponding node in the graph.
     */
    void mousePressEvent(QMouseEvent* event) override;

private:
    QTextEdit* mpCodeEditor;
    QPushButton* mpExecuteButton;
    QSpinBox* mpNumInputsSpinBox;
    QSpinBox* mpNumOutputsSpinBox;
};
