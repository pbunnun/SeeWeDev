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

/**
 * @file ZenohSettingsDialog.hpp
 * @brief Settings dialog for configuring Zenoh connection parameters.
 *
 * Provides UI for configuring Zenoh operational mode, router addresses,
 * and connection strings. Settings are persisted in application settings.
 *
 * **Configuration Options:**
 * - Mode: Peer (default), Client, Router
 * - Connect addresses (multicast, unicast, TCP/UDP endpoints)
 * - Listen addresses (for router/peer modes)
 * - Enable/disable shared memory transport
 *
 * **Typical Usage:**
 * @code
 * ZenohSettingsDialog dialog(this);
 * if (dialog.exec() == QDialog::Accepted) {
 *     QString config = dialog.getConfigString();
 *     ZenohBridge::instance().initialize(sessionId, config);
 * }
 * @endcode
 *
 * @see ZenohBridge for Zenoh session management
 */

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>

/**
 * @class ZenohSettingsDialog
 * @brief Dialog for configuring Zenoh connection settings.
 *
 * Provides a user-friendly interface for configuring Zenoh operational
 * parameters including mode, addresses, and transport options.
 *
 * **Settings Managed:**
 * - **Mode**: peer (default), client, router
 * - **Connect**: Endpoint(s) to connect to (e.g., tcp/192.168.1.100:7447)
 * - **Listen**: Endpoint(s) to listen on (e.g., tcp/0.0.0.0:7447)
 * - **Shared Memory**: Enable/disable local shared memory transport
 *
 * **Configuration String Format:**
 * JSON5 format used by Zenoh:
 * @code
 * {
 *   mode: "peer",
 *   connect: {
 *     endpoints: ["tcp/192.168.1.100:7447"]
 *   },
 *   listen: {
 *     endpoints: ["tcp/0.0.0.0:7447"]
 *   },
 *   scouting: {
 *     multicast: {
 *       enabled: true
 *     }
 *   }
 * }
 * @endcode
 */
class ZenohSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Zenoh settings dialog.
     *
     * Loads current settings from QSettings and populates the UI.
     *
     * @param parent Parent widget (typically MainWindow)
     *
     * **Example:**
     * @code
     * ZenohSettingsDialog dialog(mainWindow);
     * dialog.exec();
     * @endcode
     */
    explicit ZenohSettingsDialog(QWidget *parent = nullptr);

    /**
     * @brief Gets the computer ID used as the first key segment in all Zenoh topics.
     *
     * Defaults to the computer hostname. Users can change it so keys remain stable
     * across sessions and are human-readable.
     *
     * @return QString Computer ID string
     */
    QString getComputerId() const;

    /**
     * @brief Gets the complete Zenoh configuration string.
     *
     * Builds a JSON5 configuration string from current dialog values.
     * Empty string indicates default configuration (auto-discovery).
     *
     * @return QString Configuration string in JSON5 format, or empty for defaults
     *
     * **Example:**
     * @code
     * if (dialog.exec() == QDialog::Accepted) {
     *     QString config = dialog.getConfigString();
     *     if (!config.isEmpty()) {
     *         ZenohBridge::instance().initialize(sessionId, config);
     *     }
     * }
     * @endcode
     */
    QString getConfigString() const;

    /**
     * @brief Gets the selected Zenoh mode.
     *
     * @return QString Mode string: "peer", "client", or "router"
     *
     * **Mode Descriptions:**
     * - **peer**: Default. Participates in scouting, routes locally
     * - **client**: Lightweight. Connects to router, no routing
     * - **router**: Full routing. Infrastructure mode for multi-peer networks
     */
    QString getMode() const;

    /**
     * @brief Gets the connect endpoints string.
     *
     * Comma-separated list of endpoints to connect to.
     *
     * @return QString Endpoints (e.g., "tcp/192.168.1.100:7447,tcp/192.168.1.101:7447")
     */
    QString getConnectEndpoints() const;

    /**
     * @brief Gets the listen endpoints string.
     *
     * Comma-separated list of endpoints to listen on.
     *
     * @return QString Endpoints (e.g., "tcp/0.0.0.0:7447")
     */
    QString getListenEndpoints() const;

    /**
     * @brief Checks if multicast scouting is enabled.
     *
     * Multicast allows automatic peer discovery on local network.
     *
     * @return bool True if multicast enabled
     */
    bool isMulticastEnabled() const;

    /**
     * @brief Checks if shared memory transport is enabled.
     *
     * Shared memory provides zero-copy transport for local processes.
     *
     * @return bool True if shared memory enabled
     */
    bool isSharedMemoryEnabled() const;

    /**
     * @brief Gets whether the transport security feature is enabled.
     * @return true if security group is checked
     */
    bool isSecurityEnabled() const;

    /**
     * @brief Gets the configured transport shared secret key.
     * @return QString secret key
     */
    QString getSharedSecret() const;

    /**
     * @brief Gets global node transport mode.
     * @return QString "qt_only" or "zenoh_only"
     */

private slots:
    /**
     * @brief Handles mode selection changes.
     *
     * Updates UI hints and enables/disables relevant fields based on mode.
     *
     * @param index ComboBox index (0=peer, 1=client, 2=router)
     */
    void onModeChanged(int index);

    /**
     * @brief Restores default settings.
     *
     * Resets all fields to default Zenoh configuration:
     * - Mode: peer
     * - Multicast: enabled
     * - Shared memory: enabled
     * - Connect/Listen: empty (auto)
     */
    void onRestoreDefaults();

    /**
     * @brief Tests the current configuration.
     *
     * Attempts to initialize a temporary Zenoh session with current
     * settings and displays success/error message.
     */
    void onTestConnection();

private:
    /**
     * @brief Initializes the dialog UI.
     *
     * Creates and layouts all widgets.
     */
    void setupUI();

    /**
     * @brief Loads settings from QSettings.
     *
     * Reads saved configuration from application settings and
     * populates dialog fields.
     */
    void loadSettings();

    /**
     * @brief Saves settings to QSettings.
     *
     * Persists current dialog values to application settings.
     * Called when dialog is accepted.
     */
    void saveSettings();

    /**
     * @brief Builds configuration preview text.
     *
     * Generates human-readable preview of current configuration
     * for display in the preview text area.
     *
     * @return QString Preview text
     */
    QString buildPreviewText() const;

    /**
     * @brief Updates the configuration preview.
     *
     * Refreshes the preview text area with current settings.
     */
    void updatePreview();

    // UI Widgets
    QGroupBox *mComputerIdGroup;        ///< Computer ID frame
    QGroupBox *mModeGroup;              ///< Zenoh mode frame
    QGroupBox *mConnectionGroup;        ///< Connection settings frame
    QGroupBox *mTransportOptionsGroup;  ///< Transport options frame
    QGroupBox *mSecurityGroup;          ///< Security settings frame
    QGroupBox *mPreviewGroup;           ///< Preview frame
    QLineEdit *mComputerIdLineEdit;     ///< Computer ID input
    QComboBox *mModeComboBox;           ///< Zenoh mode selector
    QLineEdit *mConnectLineEdit;        ///< Connect endpoints input
    QLineEdit *mListenLineEdit;         ///< Listen endpoints input
    QCheckBox *mMulticastCheckBox;      ///< Enable multicast scouting
    QCheckBox *mSharedMemoryCheckBox;   ///< Enable shared memory transport
    QLineEdit *mSharedSecretLineEdit;   ///< Pre-shared key secret input
    QTextEdit *mPreviewTextEdit;        ///< Configuration preview
    QPushButton *mTestButton;           ///< Test connection button
    QPushButton *mDefaultsButton;       ///< Restore defaults button

    // Labels for hints
    QLabel *mModeHintLabel;             ///< Mode description
    QLabel *mConnectHintLabel;          ///< Connect format hint
    QLabel *mListenHintLabel;           ///< Listen format hint
};
