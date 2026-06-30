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

#include "PythonEditorEmbeddedWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QEvent>
#include <QMouseEvent>

PythonEditorEmbeddedWidget::PythonEditorEmbeddedWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Port configuration
    auto* portLayout = new QHBoxLayout();
    
    auto* inputLabel = new QLabel("Inputs:", this);
    mpNumInputsSpinBox = new QSpinBox(this);
    mpNumInputsSpinBox->setRange(0, 10);
    mpNumInputsSpinBox->setValue(1);
    mpNumInputsSpinBox->setToolTip("Number of input ports");
    
    auto* outputLabel = new QLabel("Outputs:", this);
    mpNumOutputsSpinBox = new QSpinBox(this);
    mpNumOutputsSpinBox->setRange(0, 10);
    mpNumOutputsSpinBox->setValue(1);
    mpNumOutputsSpinBox->setToolTip("Number of output ports");
    
    portLayout->addWidget(inputLabel);
    portLayout->addWidget(mpNumInputsSpinBox);
    portLayout->addSpacing(10);
    portLayout->addWidget(outputLabel);
    portLayout->addWidget(mpNumOutputsSpinBox);
    portLayout->addStretch();
    
    mainLayout->addLayout(portLayout);

    // Python code editor
    mpCodeEditor = new QTextEdit(this);
    mpCodeEditor->setMinimumSize(400, 300);
    mpCodeEditor->setAcceptRichText(false);
    
    // Install event filter to catch focus events
    mpCodeEditor->installEventFilter(this);
    mpNumInputsSpinBox->installEventFilter(this);
    mpNumOutputsSpinBox->installEventFilter(this);
    
    // Use monospace font for code
    QFont font("Courier");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(11);
    mpCodeEditor->setFont(font);
    
    // Use default template text instead of placeholder because QTextEdit
    // placeholder rendering can collapse multi-line hints on some platforms.
    mpCodeEditor->setPlainText(
        "# Example: run initialization once, process every input frame\n"
        "# Access input data via input0, input1, ... variables\n"
        "# Set output data via output0, output1, ... variables\n"
        "# import cv2\n"
        "#\n"
        "# # Runs only once per Python session\n"
        "# if 'model' not in globals():\n"
        "#     model = cv2.dnn.readNetFromONNX('/path/to/dog_detector.onnx')\n"
        "#\n"
        "# # Runs every time the node receives new input\n"
        "# if 'input0' in globals() and input0 is not None:\n"
        "#     img = input0\n"
        "#     # TODO: preprocess + inference using model\n"
        "#     output0 = img\n"
    );
    
    mainLayout->addWidget(mpCodeEditor);

    // Execute button
    mpExecuteButton = new QPushButton("Execute (Ctrl+Return)", this);
    mpExecuteButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 5px; }");
    mainLayout->addWidget(mpExecuteButton);

    // Connect signals
    connect(mpExecuteButton, &QPushButton::clicked, this, &PythonEditorEmbeddedWidget::executeClicked);
    connect(mpNumInputsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PythonEditorEmbeddedWidget::numInputsChanged);
    connect(mpNumOutputsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PythonEditorEmbeddedWidget::numOutputsChanged);

    setLayout(mainLayout);
}

QString PythonEditorEmbeddedWidget::getPythonCode() const
{
    return mpCodeEditor->toPlainText();
}

void PythonEditorEmbeddedWidget::setPythonCode(const QString& code)
{
    mpCodeEditor->setPlainText(code);
}

bool PythonEditorEmbeddedWidget::eventFilter(QObject* obj, QEvent* event)
{
    // Request node selection when any child widget gets focus
    if ((obj == mpCodeEditor || obj == mpNumInputsSpinBox || obj == mpNumOutputsSpinBox))
    {
        if( event->type() == QEvent::FocusIn )
        {
            Q_EMIT widgetClicked();
            Q_EMIT editableWidgetSelectedChanged(true);
        }
        else if( event->type() == QEvent::FocusOut )
        {
            Q_EMIT editableWidgetSelectedChanged(false);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PythonEditorEmbeddedWidget::mousePressEvent(QMouseEvent* event)
{
    // Request node selection when widget is clicked
    Q_EMIT widgetClicked();
    QWidget::mousePressEvent(event);
}
