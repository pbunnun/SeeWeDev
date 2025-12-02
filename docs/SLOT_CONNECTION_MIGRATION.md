# Qt Automatic Slot Connection Migration

## Overview

This project has migrated from Qt's automatic slot connection mechanism to explicit `QObject::connect()` calls for improved type safety and clarity.

## What Changed

### Before (Automatic Connection - Deprecated)
```cpp
// Widget constructor - NO explicit connections needed
MyWidget::MyWidget(QWidget *parent) : QWidget(parent), ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    // Qt automatically connects slots based on naming convention
}

// Slot naming: on_<objectName>_<signalName>
private Q_SLOTS:
    void on_mpButton_clicked();
    void on_mpComboBox_currentIndexChanged(int index);
```

### After (Explicit Connection - Required)
```cpp
// Widget constructor - explicit connections required
#include <QPushButton>
#include <QComboBox>

MyWidget::MyWidget(QWidget *parent) : QWidget(parent), ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    
    // Explicit type-safe connections
    connect(ui->mpButton, &QPushButton::clicked, this, &MyWidget::button_clicked);
    connect(ui->mpComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MyWidget::combo_box_current_index_changed);
}

// Slot naming: descriptive names without on_ prefix
private Q_SLOTS:
    void button_clicked();
    void combo_box_current_index_changed(int index);
```

## Migration Statistics

- **Files converted**: ~30 widget files
- **Connections converted**: ~140+ automatic slot connections
- **Folders affected**: 
  - `Plugins/BasicNodes/` (15 widgets)
  - `Plugins/BaslerCameraNodes/` (1 widget, 27 slots)
  - `Plugins/IntelRealsenseNodes/` (2 widgets)
  - `Plugins/SMRExtraNodes/` (3 widgets)
  - `Plugins/FLRCameraNodes/` (1 widget)
  - `Plugins/GPLNodes/` (2 widgets)
  - `Plugins/Customers/*` (4 widgets)

## Pattern Reference

### Basic Connections

```cpp
// QPushButton::clicked
connect(ui->mpButton, &QPushButton::clicked, 
        this, &MyWidget::button_clicked);

// QLineEdit::textChanged
connect(ui->mpLineEdit, &QLineEdit::textChanged, 
        this, &MyWidget::line_edit_text_changed);

// QCheckBox with Qt version handling (6.7+ breaking change)
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(ui->mpCheckBox, QOverload<int>::of(&QCheckBox::stateChanged), 
            this, &MyWidget::check_box_state_changed);
#else
    connect(ui->mpCheckBox, &QCheckBox::checkStateChanged, 
            this, &MyWidget::check_box_check_state_changed);
#endif
```

### Overloaded Signal Connections

Some Qt widgets have overloaded signals that require explicit type specification:

```cpp
// QComboBox::currentIndexChanged has two overloads: (int) and (QString)
connect(ui->mpComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
        this, &MyWidget::combo_box_current_index_changed);

// QSpinBox::valueChanged also has overloads
connect(ui->mpSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
        this, &MyWidget::spin_box_value_changed);

// QSlider::valueChanged
connect(ui->mpSlider, &QSlider::valueChanged, 
        this, &MyWidget::slider_value_changed);

// QPushButton::clicked with parameter (toggled/checkable buttons)
connect(ui->mpToggleButton, QOverload<bool>::of(&QPushButton::clicked), 
        this, &MyWidget::toggle_button_clicked);
```

### Required Includes

When adding explicit connections, ensure you include the widget headers:

```cpp
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSlider>
#include <QRadioButton>
#include <QPlainTextEdit>
```

## Benefits of Explicit Connections

1. **Type Safety**: Compiler checks signal/slot signatures at compile time
2. **Clarity**: Connection points are visible in constructor code
3. **Flexibility**: Not bound by naming conventions
4. **Maintainability**: Easier to understand widget behavior
5. **IDE Support**: Better autocomplete and refactoring support
6. **No Magic**: No hidden automatic behavior

## Future Development

**All new widgets MUST use explicit connections.** Do not rely on the `on_mp*` naming convention.

### New Widget Checklist

- [ ] Add required widget includes (`#include <QWidgetType>`)
- [ ] Add explicit `connect()` calls in constructor after `ui->setupUi(this)`
- [ ] Use descriptive slot names without `on_` prefix
- [ ] Use `QOverload<Type>::of()` for overloaded signals
- [ ] Add Qt version conditionals for QCheckBox if using Qt 6.7+
- [ ] Test that all connections work correctly

## Qt Version Compatibility

### QCheckBox Signal Change (Qt 6.7+)

Qt 6.7 renamed `QCheckBox::stateChanged` to `checkStateChanged`. Use conditional compilation:

```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    void check_box_state_changed(int state);
#else
    void check_box_check_state_changed(Qt::CheckState state);
#endif
```

## Conversion Scripts

The following scripts were used for the migration (saved for reference):

- `convert_slots.py` - Basic slot renaming
- `convert_remaining_slots.sh` - Batch slot renaming with sed
- `convert_large_widgets.sh` - Large widget batch conversion
- `convert_gpl_customer.sh` - GPL/Customer plugin conversion
- `add_explicit_connections.py` - Add connect() statements
- `add_gpl_customer_connections.py` - GPL/Customer connect() statements

These scripts should not be needed for new development but are kept for documentation purposes.

## Migration Date

Completed: November 2025

## References

- Qt Documentation: [Signals & Slots](https://doc.qt.io/qt-6/signalsandslots.html)
- Qt Documentation: [QObject::connect()](https://doc.qt.io/qt-6/qobject.html#connect)
- Qt Wiki: [New Signal Slot Syntax](https://wiki.qt.io/New_Signal_Slot_Syntax)
