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
 * @file ExternalCommandModel.hpp
 * @brief Provides execution of external system commands from the dataflow pipeline.
 *
 * This file implements a node that executes external command-line programs or scripts,
 * allowing the dataflow graph to integrate with system utilities, custom tools, and
 * third-party applications that aren't directly available as native nodes.
 *
 * The ExternalCommand node bridges the gap between the visual dataflow environment
 * and the traditional command-line world, enabling:
 * - Execution of system utilities (ffmpeg, ImageMagick, custom scripts)
 * - Integration with external processing tools
 * - File format conversions using specialized tools
 * - Triggering of system actions (notifications, file operations)
 * - Launching of third-party applications
 * - Running custom Python/Shell/Batch scripts
 *
 * Functionality:
 * The node accepts configurable command and argument strings, which can be triggered
 * by incoming data (e.g., trigger on sync signal, or pass data as arguments).
 *
 * Key Features:
 * - Configurable command path (executable or script)
 * - Customizable arguments (can include placeholders for dynamic values)
 * - Asynchronous execution (non-blocking)
 * - Synchronous mode available (wait for completion)
 * - Error handling (capture return codes, stderr)
 * - Working directory specification
 *
 * Common Use Cases:
 * 1. Video Encoding: Execute ffmpeg to convert video files
 * 2. Image Conversion: Use ImageMagick for format conversion
 * 3. External Analytics: Run Python scripts for specialized processing
 * 4. File Management: Copy, move, or organize processed files
 * 5. Notifications: Send system notifications on events
 * 6. Database Operations: Execute SQL scripts or database tools
 * 7. Cloud Upload: Trigger upload scripts for processed data
 *
 * Security Considerations:
 * - Command validation to prevent injection attacks
 * - Argument sanitization
 * - Execution permissions and sandboxing
 * - User confirmation for potentially dangerous commands
 *
 * @see ExternalCommandModel, QProcess
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QQueue>
#include <QDir>
#include "PBNodeDelegateModel.hpp"
#include "InformationData.hpp"

#include <opencv2/videoio.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeValidationState;

/**
 * @class ExternalCommandModel
 * @brief Node for executing external command-line programs from the dataflow pipeline.
 *
 * This model enables integration with external tools and scripts by executing system
 * commands with configurable arguments. It acts as a bridge between the visual dataflow
 * environment and traditional command-line utilities, expanding the processing capabilities
 * beyond built-in nodes.
 *
 * Execution Model:
 * The node uses Qt's QProcess to spawn external processes:
 * ```cpp
 * QProcess process;
 * process.start(msExternalCommand, arguments_list);
 * process.waitForFinished();  // Or run asynchronously
 * ```
 *
 * Configuration Parameters:
 * - msExternalCommand: Path to executable or script (e.g., "/usr/bin/ffmpeg", "python3")
 * - msArguments: Command-line arguments as string (e.g., "-i input.mp4 output.avi")
 *
 * Execution Modes:
 *
 * 1. Synchronous (Blocking):
 *    - Waits for command to complete before continuing
 *    - Can capture return code and output
 *    - Blocks pipeline until finished
 *    - Suitable for quick operations (< 1 second)
 *
 * 2. Asynchronous (Non-Blocking):
 *    - Launches command and continues immediately
 *    - No blocking of pipeline
 *    - Suitable for long-running operations
 *    - May use QProcess signals for completion notification
 *
 * Common Use Cases:
 *
 * 1. Video Encoding with FFmpeg:
 *    ```
 *    SaveImage("frame_%04d.png") → Sync → ExternalCommand
 *    Command: "ffmpeg"
 *    Args: "-framerate 30 -i frame_%04d.png -c:v libx264 output.mp4"
 *    ```
 *    Converts image sequence to video
 *
 * 2. Image Format Conversion (ImageMagick):
 *    ```
 *    SaveImage → ExternalCommand
 *    Command: "convert"
 *    Args: "input.png -quality 90 output.jpg"
 *    ```
 *
 * 3. Python Script Execution:
 *    ```
 *    DataGenerator → ExternalCommand
 *    Command: "python3"
 *    Args: "process_data.py --input data.csv --output result.json"
 *    ```
 *
 * 4. File Operations:
 *    ```
 *    ProcessingComplete → ExternalCommand
 *    Command: "cp"
 *    Args: "output.png /archive/$(date +%Y%m%d_%H%M%S).png"
 *    ```
 *
 * 5. System Notifications:
 *    ```
 *    ErrorDetected → ExternalCommand
 *    Command: "notify-send"
 *    Args: "Alert 'Error detected in pipeline'"
 *    ```
 *
 * 6. Database Import:
 *    ```
 *    SaveResults → ExternalCommand
 *    Command: "psql"
 *    Args: "-d mydb -f results.sql"
 *    ```
 *
 * 7. Cloud Upload:
 *    ```
 *    FinalImage → SaveImage → ExternalCommand
 *    Command: "aws"
 *    Args: "s3 cp output.png s3://mybucket/results/"
 *    ```
 *
 * Argument Handling:
 *
 * Static Arguments:
 * ```
 * Command: "ffmpeg"
 * Args: "-i input.mp4 -vf scale=640:480 output.mp4"
 * ```
 *
 * Dynamic Arguments (with placeholders):
 * The arguments string could potentially support placeholders replaced by
 * incoming data (implementation-dependent):
 * ```
 * Args: "-i {filename} -q {quality} output.mp4"
 * // Where {filename} and {quality} come from input data
 * ```
 *
 * Working Directory:
 * The command executes in the application's working directory unless
 * specified otherwise via QProcess::setWorkingDirectory().
 *
 * Input/Output Handling:
 *
 * Standard Input:
 * - Can pipe data to command's stdin (if implemented)
 * - Useful for commands that read from standard input
 *
 * Standard Output/Error:
 * - Can capture stdout and stderr
 * - May output as InformationData for logging
 * - Error detection via return codes
 *
 * Environment Variables:
 * - Inherits application's environment
 * - Can set custom environment via QProcess::setProcessEnvironment()
 *
 * Security and Safety:
 *
 * Command Injection Prevention:
 * - Validate command paths (whitelist approved executables)
 * - Sanitize arguments (escape special characters)
 * - Avoid shell interpretation when possible (use argument lists)
 *
 * Permissions:
 * - Execute only permitted commands (configuration-based whitelist)
 * - Run with appropriate user privileges
 * - Consider sandboxing for untrusted commands
 *
 * Resource Management:
 * - Set timeouts to prevent hanging
 * - Limit concurrent external processes
 * - Monitor memory and CPU usage
 *
 * Error Handling:
 *
 * Execution Failures:
 * - Command not found: Log error, return failure status
 * - Permission denied: Notify user, suggest remediation
 * - Timeout: Kill process, report timeout error
 *
 * Return Codes:
 * - 0: Success
 * - Non-zero: Error (code meaning depends on command)
 * - Can trigger downstream error handlers based on return code
 *
 * Platform Considerations:
 *
 * Cross-Platform Paths:
 * - Windows: "C:\\Program Files\\Tool\\tool.exe"
 * - Unix/Linux: "/usr/bin/tool"
 * - macOS: "/Applications/Tool.app/Contents/MacOS/tool"
 *
 * Path Separators:
 * - Use Qt's QDir::toNativeSeparators() for paths
 * - Or use "/" which Qt handles on all platforms
 *
 * Shell Scripts:
 * - Windows: .bat, .cmd files or via "cmd /c script.bat"
 * - Unix: .sh files with shebang or via "bash script.sh"
 *
 * Performance Characteristics:
 * - Process Spawn Overhead: 5-50ms (varies by OS and command)
 * - Execution Time: Depends entirely on external command
 * - IPC Overhead: Minimal for argument passing, higher for large data pipes
 * - Typical Use: Commands ranging from milliseconds to hours
 *
 * Best Practices:
 *
 * 1. Use Absolute Paths: Avoid relying on PATH environment variable
 *    ```
 *    Good: "/usr/bin/convert"
 *    Avoid: "convert" (path-dependent)
 *    ```
 *
 * 2. Validate Commands: Check executable exists before execution
 *    ```cpp
 *    if (!QFile::exists(msExternalCommand)) {
 *        // Report error
 *    }
 *    ```
 *
 * 3. Escape Arguments: Properly quote arguments with spaces
 *    ```
 *    Args: "-i \"file with spaces.mp4\" output.mp4"
 *    ```
 *
 * 4. Handle Errors: Check return codes and stderr output
 *
 * 5. Use Timeouts: Prevent indefinite hangs
 *    ```cpp
 *    process.waitForFinished(30000);  // 30 second timeout
 *    ```
 *
 * 6. Log Execution: Record commands for debugging and audit trails
 *
 * 7. Test Thoroughly: External commands may behave differently across systems
 *
 * Design Rationale:
 *
 * Why External Commands?
 * - Reuse existing tools (no reimplementation needed)
 * - Access specialized functionality (format converters, codecs)
 * - Enable custom scripting (Python, shell scripts)
 * - System integration (file operations, notifications)
 *
 * Why Separate Command and Arguments?
 * - Security: Easier to validate and whitelist commands
 * - Clarity: Clear distinction between program and parameters
 * - Flexibility: Arguments can be dynamically generated
 *
 * Why Qt QProcess?
 * - Cross-platform (Windows, Linux, macOS)
 * - Asynchronous support (signals/slots)
 * - Output capture (stdout, stderr)
 * - Process control (kill, terminate)
 *
 * Limitations:
 * - Platform-specific: Commands may not be portable
 * - Security risks: Improper use can lead to command injection
 * - Performance overhead: Process spawning adds latency
 * - Limited integration: Harder to pass complex data structures
 * - Error handling: Depends on external command's behavior
 * - No guaranteed availability: External tools may not be installed
 *
 * Comparison with Alternatives:
 * - vs. Native Nodes: External commands more flexible but slower and less integrated
 * - vs. Plugins: External commands easier to add but less performant
 * - vs. Scripting Nodes: External commands more general-purpose, scripts more embedded
 *
 * @see QProcess, InformationData
 */
class ExternalCommandModel : public PBNodeDelegateModel
{
    Q_OBJECT

public:
    ExternalCommandModel();

    virtual
    ~ExternalCommandModel() override {}

    QJsonObject
    save() const override;

    void
    load(QJsonObject const &p) override;

    unsigned int
    nPorts( PortType portType ) const override;

    NodeDataType
    dataType( PortType portType, PortIndex portIndex ) const override;

    /**
     * @brief Receives trigger data to execute the external command.
     *
     * When data arrives at the input port (typically a sync signal or trigger),
     * this method executes the configured external command with the specified arguments.
     *
     * Execution Flow:
     * 1. Validate that msExternalCommand is set and executable exists
     * 2. Parse msArguments into argument list
     * 3. Create QProcess instance
     * 4. Set working directory if configured
     * 5. Start process: process.start(msExternalCommand, argList)
     * 6. Wait for completion (sync) or return immediately (async)
     * 7. Capture output and return code if needed
     * 8. Log results or errors
     *
     * Example:
     * ```cpp
     * // User configured:
     * // Command: "ffmpeg"
     * // Args: "-i input.mp4 output.avi"
     *
     * setInData(syncSignal, 0);  // Trigger arrives
     *   ↓
     * QProcess executes: ffmpeg -i input.mp4 output.avi
     *   ↓
     * Wait for completion or continue
     * ```
     *
     * @param nodeData Trigger data (often SyncData or any data to initiate execution)
     * @param port Input port index (typically 0)
     *
     * @note May block if running in synchronous mode
     * @note Check for errors via QProcess error codes
     */
    void
    setInData( std::shared_ptr< NodeData > nodeData, PortIndex port ) override;

    /**
     * @brief No embedded widget provided.
     * @return nullptr (configuration via property system)
     */
    QWidget *
    embeddedWidget() override { return nullptr; }

    QPixmap
    minPixmap() const override{ return _minPixmap; }

    /**
     * @brief Handles property updates for command and arguments configuration.
     *
     * This method receives property changes from the property browser, allowing
     * users to configure the external command and its arguments.
     *
     * Supported Properties:
     * - "command" or "externalCommand": Set msExternalCommand (executable path)
     * - "arguments" or "args": Set msArguments (command-line arguments string)
     * - "workingDirectory": Set working directory for command execution
     * - "timeout": Set maximum execution time (milliseconds)
     * - "async": Enable/disable asynchronous execution mode
     *
     * Validation:
     * - Check if command path exists (QFile::exists)
     * - Verify executable permissions (may vary by platform)
     * - Sanitize arguments for safety
     *
     * Example:
     * ```cpp
     * setModelProperty("command", "/usr/bin/ffmpeg");
     * setModelProperty("arguments", "-i input.mp4 -vf scale=640:480 output.mp4");
     * ```
     *
     * @param property Property name
     * @param value New property value
     *
     * @note Property names may be predefined or dynamic based on implementation
     */
    void
    setModelProperty( QString &, const QVariant & ) override;

    static const QString _category;

    static const QString _model_name;

private:
    /**
     * @brief Path to the external command or executable.
     *
     * This can be:
     * - Absolute path: "/usr/bin/ffmpeg", "C:\\Program Files\\Tool\\tool.exe"
     * - Relative path: "scripts/process.sh" (relative to working directory)
     * - Command in PATH: "python3", "convert" (relies on system PATH)
     *
     * Best Practice: Use absolute paths for reliability and security.
     *
     * Examples:
     * - Windows: "C:\\ffmpeg\\bin\\ffmpeg.exe"
     * - Linux: "/usr/bin/convert"
     * - macOS: "/usr/local/bin/python3"
     * - Script: "/home/user/scripts/process.py"
     *
     * Default: Empty string (must be configured before execution)
     */
    QString msExternalCommand{""};
    
    /**
     * @brief Command-line arguments passed to the external command.
     *
     * This string contains all arguments as they would appear on the command line.
     * The string is typically parsed into individual arguments before passing to QProcess.
     *
     * Argument Parsing:
     * - Space-separated: "arg1 arg2 arg3"
     * - Quoted strings: "-i \"file with spaces.mp4\" output.mp4"
     * - Special characters: Escape as needed for shell interpretation
     *
     * Examples:
     * - FFmpeg: "-i input.mp4 -c:v libx264 -crf 23 output.mp4"
     * - ImageMagick: "input.png -resize 50% -quality 90 output.jpg"
     * - Python: "script.py --verbose --input data.csv"
     * - Shell: "-c 'echo Hello > output.txt'"
     *
     * Dynamic Arguments:
     * May support placeholder substitution (implementation-dependent):
     * - "{filename}": Replaced with incoming filename data
     * - "{count}": Replaced with integer data
     * - "{timestamp}": Replaced with current timestamp
     *
     * Default: Empty string (command runs with no arguments)
     *
     * @note Proper quoting is essential for arguments containing spaces or special chars
     * @note Some implementations may parse this string, others may pass directly to shell
     */
    QString msArguments{""};

    QPixmap _minPixmap;
};

