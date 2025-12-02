# Debug Logging Control

## Overview

The CVDevPro application uses Qt Logging Categories for debug logging, which allows you to enable or disable debug messages at runtime without recompiling.

## Logging Macros

All debug logging uses the `DebugLogging` category with two macros defined in `CVDevLibrary/DebugLogging.hpp`:

```cpp
DEBUG_LOG_INFO() << "Your info message here";
DEBUG_LOG_WARNING() << "Your warning message here";
```

Both macros use the same `DebugLogging` category, allowing unified control of all debug output.

To use these macros in your code, include the header:

```cpp
#include "DebugLogging.hpp"
```

**Note**: The logging macros are available throughout the CVDevLibrary and can be used in any file that includes `DebugLogging.hpp`, including:
- `PBNodeDelegateModel` and derived node models
- `ZenohBridge` and `ZenohSerializer`
- `MainWindow` and other UI components
- Plugin code that links against CVDevLibrary

## Controlling Log Output

### Method 1: Environment Variable (Recommended for Testing)

Set the `QT_LOGGING_RULES` environment variable before launching the application:

**Disable info messages only:**
```bash
export QT_LOGGING_RULES="DebugLogging.info=false"
./CVDevPro
```

**Disable warning messages only:**
```bash
export QT_LOGGING_RULES="DebugLogging.warning=false"
./CVDevPro
```

**Disable all debug logs (both info and warning):**
```bash
export QT_LOGGING_RULES="DebugLogging=false"
./CVDevPro
```

**Enable all debug logs (default):**
```bash
export QT_LOGGING_RULES="DebugLogging=true"
./CVDevPro
```

**Multiple categories:**
```bash
export QT_LOGGING_RULES="DebugLogging.info=false;qt.*.debug=false"
./CVDevPro
```

### Method 2: Compile-time Control in main.cpp

Edit the defines at the top of `CVDev/main.cpp` to control logging at compile time:

```cpp
//#define __SAVE_LOG__
#define __ENABLE_DEBUG_LOG_INFO__      // Comment out to disable INFO messages
#define __ENABLE_DEBUG_LOG_WARNING__   // Comment out to disable WARNING messages
#define __CHECK_LICENSE_KEY__
```

**Examples:**

Disable info messages only:
```cpp
//#define __ENABLE_DEBUG_LOG_INFO__      // Disabled
#define __ENABLE_DEBUG_LOG_WARNING__    // Enabled
```

Disable warning messages only:
```cpp
#define __ENABLE_DEBUG_LOG_INFO__       // Enabled
//#define __ENABLE_DEBUG_LOG_WARNING__   // Disabled
```

Disable all debug logging:
```cpp
//#define __ENABLE_DEBUG_LOG_INFO__      // Disabled
//#define __ENABLE_DEBUG_LOG_WARNING__   // Disabled
```

### Method 3: In Code (Programmatic Control)

Add this to your code for runtime control:

**Disable info logging:**
```cpp
#include <QLoggingCategory>

QLoggingCategory::setFilterRules("DebugLogging.info=false");
```

**Disable warning logging:**
```cpp
QLoggingCategory::setFilterRules("DebugLogging.warning=false");
```

**Disable all debug logging:**
```cpp
QLoggingCategory::setFilterRules("DebugLogging=false");
```

**Enable debug logging:**
```cpp
QLoggingCategory::setFilterRules("DebugLogging=true");
```

### Method 4: Qt Logging Configuration File

Create a file `qtlogging.ini` in one of these locations:
- `$XDG_CONFIG_HOME/QtProject/qtlogging.ini` (Unix)
- `~/Library/Preferences/QtProject/qtlogging.ini` (macOS)
- `%APPDATA%/QtProject/qtlogging.ini` (Windows)

**Content:**
```ini
[Rules]
# Disable all debug logs
DebugLogging=false

# Or disable only specific levels
DebugLogging.info=false
DebugLogging.warning=false
```

## Examples

### Temporarily Enable/Disable Logging for Debugging

```bash
# Terminal 1 - with all logging
QT_LOGGING_RULES="DebugLogging=true" ./CVDevPro

# Terminal 2 - without info messages (warnings still shown)
QT_LOGGING_RULES="DebugLogging.info=false" ./CVDevPro

# Terminal 3 - without warning messages (info still shown)
QT_LOGGING_RULES="DebugLogging.warning=false" ./CVDevPro

# Terminal 4 - without any debug logs
QT_LOGGING_RULES="DebugLogging=false" ./CVDevPro
```

### Enable Only Specific Categories

```bash
# Enable debug logs but disable Qt internal debug messages
export QT_LOGGING_RULES="DebugLogging=true;qt.*.debug=false"
```

## Log Message Format

Each log message includes:
- Message type: `[[Info]` or `[[Warning]`
- Timestamp: `yyyy-MM-dd hh:mm:ss.zzz`
- Source file name
- Line number

**Example output:**
```
[[Info] 2025-11-04 10:30:45.123 | PBNodeDelegateModel.cpp:465] [requestPropertyChange] propertyId: "num_inputs"
[[Warning] 2025-11-04 10:30:45.456 | ZenohBridge.cpp:123] Connection timeout detected
```

## Default Behavior

By default, both `DEBUG_LOG_INFO()` and `DEBUG_LOG_WARNING()` messages are **enabled** during development.

In production builds or for selective logging, you can control them individually in `main.cpp`:

```cpp
// In CVDev/main.cpp - Edit these defines:
//#define __SAVE_LOG__
#define __ENABLE_DEBUG_LOG_INFO__      // Comment out to disable INFO messages
#define __ENABLE_DEBUG_LOG_WARNING__   // Comment out to disable WARNING messages
#define __CHECK_LICENSE_KEY__
```

**Common configurations:**

1. **Full logging (development)**:
   ```cpp
   #define __ENABLE_DEBUG_LOG_INFO__
   #define __ENABLE_DEBUG_LOG_WARNING__
   ```

2. **Warnings only (testing)**:
   ```cpp
   //#define __ENABLE_DEBUG_LOG_INFO__     // Disabled
   #define __ENABLE_DEBUG_LOG_WARNING__   // Enabled
   ```

3. **No debug logging (production)**:
   ```cpp
   //#define __ENABLE_DEBUG_LOG_INFO__      // Disabled
   //#define __ENABLE_DEBUG_LOG_WARNING__   // Disabled
   ```

When a define is not enabled, the corresponding filter rule is set:
- `__ENABLE_DEBUG_LOG_INFO__` not defined → `DebugLogging.info=false`
- `__ENABLE_DEBUG_LOG_WARNING__` not defined → `DebugLogging.warning=false`

You can still override these settings at runtime using the `QT_LOGGING_RULES` environment variable.

## Performance Notes

- Disabled logging categories have minimal performance impact (the condition check is very fast)
- Log message formatting only occurs when logging is enabled
- Use environment variables for quick testing, code-based control for production

## See Also

- Qt Documentation: [QLoggingCategory](https://doc.qt.io/qt-6/qloggingcategory.html)
- Qt Documentation: [QtMessageHandler](https://doc.qt.io/qt-6/qtglobal.html#qInstallMessageHandler)
