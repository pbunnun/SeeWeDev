//Copyright © 2025 - 2026, NECTEC, all rights reserved

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
    
    // Set placeholder text with usage instructions
    mpCodeEditor->setPlaceholderText(
        "# Python Code Editor\n"
        "# Access inputs: input0, input1, ...\n"
        "# Set outputs: output0, output1, ...\n"
        "#\n"
        "# Available types:\n"
        "# - cv::Mat -> numpy.ndarray\n"
        "# - numbers -> float/int\n"
        "# - strings -> str\n"
        "# - points/rects -> dict\n"
        "#\n"
        "# Example:\n"
        "# import cv2\n"
        "# import numpy as np\n"
        "#\n"
        "# # Process image input\n"
        "# if 'input0' in globals():\n"
        "#     img = input0\n"
        "#     gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)\n"
        "#     output0 = gray\n"
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
