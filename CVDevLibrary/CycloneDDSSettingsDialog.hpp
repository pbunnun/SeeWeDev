//Copyright © 2020 - 2026, NECTEC, all rights reserved

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

/**
 * @file CycloneDDSSettingsDialog.hpp
 * @brief Modal dialog for CycloneDDS configuration and testing.
 *
 * Provides UI controls for selecting DDS domain ID, partition, computer ID,
 * and shared secret (for remote control command signing). Integrates with
 * CycloneDDSBridge for connection testing and persists settings to QSettings.
 */

#include <QDialog>

class QComboBox;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTimer;

/**
 * @class CycloneDDSSettingsDialog
 * @brief Modal configuration dialog for DDS transport settings.
 *
 * **Features:**
 * - Computer ID selection (defaults to QSysInfo::machineHostName())
 * - Domain ID input (0-232, default 0)
 * - Optional partition specification
 * - Shared secret password for remote command signing
 * - Settings persistence to QSettings
 * - Connection test button (via CycloneDDSBridge::initialize())
 * - Restore defaults button
 *
 * **Usage:**
 * ```cpp
 * CycloneDDSSettingsDialog dialog(parentWidget);
 * if (dialog.exec() == QDialog::Accepted) {
 *     QString computerId = dialog.getComputerId();
 *     int domainId = dialog.getDomainId();
 *     // Use settings...
 * }
 * ```
 *
 * **Settings Persistence:**
 * - Loads/saves settings from CVDev settings INI file
 * - Falls back to computer ID defaults if not configured
 * - Settings group: "[CycloneDDS]"
 */
class CycloneDDSSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /// @name Construction & Configuration
    /// @{

    /**
     * @brief Constructs the DDS settings dialog.
     *
     * Initializes UI controls, loads saved settings, and sets up signal/slot
     * connections for button actions.
     *
     * @param parent Parent widget for this dialog.
     */
    explicit CycloneDDSSettingsDialog(QWidget *parent = nullptr);

    /// @}

    /// @name Settings Accessors
    /// @{

    /**
     * @brief Returns the configured computer ID.
     *
     * If line edit is empty, returns QSysInfo::machineHostName() as fallback.
     *
     * @return Computer ID (defaults to hostname if user didn't specify).
     */
    QString getComputerId() const;

    /**
     * @brief Returns the selected DDS domain ID.
     * @return Domain ID (0-232).
     */
    int getDomainId() const;

    /**
     * @brief Returns the optional DDS partition.
     * @return Partition string (trimmed, may be empty).
     */
    QString getPartition() const;
    bool isSecurityEnabled() const;
    /**
     * @brief Returns the shared secret for remote command signing.
     *
     * Used by RemoteCommandSecurity for HMAC-SHA256 message authentication.
     * Empty string means authentication is disabled (development default).
     *
     * @return Shared secret string (may be empty).
     */
    QString getSharedSecret() const;

    /// @}

private Q_SLOTS:
    /// @name Private Slot Handlers
    /// @{

    /**
     * @brief Restores all settings to factory defaults.
     *
     * Sets:
     * - Computer ID → QSysInfo::machineHostName()
     * - Domain ID → 0
     * - Partition → empty
     */
    void onRestoreDefaults();

    /**
     * @brief Tests DDS bridge initialization with current settings.
     *
     * Calls CycloneDDSBridge::initialize() with dialog values and shows
     * success/failure message. Useful for validating DDS configuration
     * before saving.
     */
    void onTestConnection();

    /// @}

private:
    /// @name UI Setup
    /// @{

    /**
     * @brief Constructs all dialog widgets and layout.
     *
     * Creates 4 sections:
     * 1. Computer ID group (with hint text)
     * 2. DDS Configuration group (domain ID, partition)
     * 3. Security group (shared secret with show/hide toggle)
     * 4. Action buttons (defaults, test, OK/Cancel)
     */
    void setupUI();

    /// @}

    /// @name Settings I/O
    /// @{

    /**
     * @brief Loads settings from QSettings into UI controls.
     *
     * Queries CVDev settings INI file. Falls back to defaults if keys
     * are not found.
     */
    void loadSettings();

    /**
     * @brief Saves all UI values back to QSettings.
     *
     * Called before dialog accepts (OK button).
     */
    void saveSettings();

    /// @}

    /// @name Widget References
    /// @{

    /** Computer ID input group. */
    QGroupBox *mComputerIdGroup{nullptr};

    /** DDS domain/partition configuration group. */
    QGroupBox *mDdsGroup{nullptr};

    /** Security (shared secret) group. */
    QGroupBox *mSecurityGroup{nullptr};

    /** Computer ID text input (hints: hostname). */
    QLineEdit *mComputerIdLineEdit{nullptr};

    /** Domain ID spinner (0-232). */
    QSpinBox *mDomainIdSpinBox{nullptr};

    /** Optional partition name input. */
    QLineEdit *mPartitionLineEdit{nullptr};

    /** Shared secret password input (password echo). */
    QLineEdit *mSharedSecretLineEdit{nullptr};

    /** "Restore Defaults" button. */
    QPushButton *mDefaultsButton{nullptr};

    /** "Test Connection" button. */
    QPushButton *mTestButton{nullptr};

    /// @}
};
