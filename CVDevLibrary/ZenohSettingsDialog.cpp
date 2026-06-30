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
 * @file ZenohSettingsDialog.cpp
 * @brief Implementation of Zenoh settings dialog.
 */

#include "ZenohSettingsDialog.hpp"
#include "TransportModeManager.hpp"
#include "ZenohBridge.hpp"
#include "CVDevLibrary.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSettings>
#include <QRegularExpression>
#include <QUuid>
#include <QApplication>
#include <QSysInfo>
#include <QMetaObject>
#include <QPointer>

ZenohSettingsDialog::ZenohSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Zenoh Configuration");
    setMinimumWidth(600);
    setMinimumHeight(450);

    setupUI();
    loadSettings();
    onModeChanged(mModeComboBox->currentIndex());
    updatePreview();

    // Connect signals
    connect(mModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ZenohSettingsDialog::onModeChanged);
    connect(mConnectLineEdit, &QLineEdit::textChanged,
            this, &ZenohSettingsDialog::updatePreview);
    connect(mListenLineEdit, &QLineEdit::textChanged,
            this, &ZenohSettingsDialog::updatePreview);
    // `QCheckBox::checkStateChanged` was introduced in Qt 6.7.
    // Provide a fallback for older Qt versions so the project
    // can still compile with Qt < 6.7.
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(mMulticastCheckBox, &QCheckBox::checkStateChanged,
        this, &ZenohSettingsDialog::updatePreview);
    connect(mSharedMemoryCheckBox, &QCheckBox::checkStateChanged,
        this, &ZenohSettingsDialog::updatePreview);
#else
    // Use the older `stateChanged(int)` signal and a small lambda
    // adapter to call the no-argument `updatePreview()` slot.
    connect(mMulticastCheckBox, QOverload<int>::of(&QCheckBox::stateChanged),
        this, [this](int){ updatePreview(); });
    connect(mSharedMemoryCheckBox, QOverload<int>::of(&QCheckBox::stateChanged),
        this, [this](int){ updatePreview(); });
#endif
    connect(mDefaultsButton, &QPushButton::clicked,
            this, &ZenohSettingsDialog::onRestoreDefaults);
    connect(mTestButton, &QPushButton::clicked,
            this, &ZenohSettingsDialog::onTestConnection);
    connect(mComputerIdLineEdit, &QLineEdit::textChanged,
            this, &ZenohSettingsDialog::updatePreview);
    connect(mSecurityGroup, &QGroupBox::toggled,
            this, [this](bool checked) {
                if (checked && mSharedSecretLineEdit->text().isEmpty()) {
                    mSharedSecretLineEdit->setText(QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(12));
                }
                updatePreview();
            });
    connect(mSharedSecretLineEdit, &QLineEdit::textChanged,
            this, &ZenohSettingsDialog::updatePreview);
}

void ZenohSettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Keep the settings content scrollable so the dialog remains usable at small heights.
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollContent = new QWidget(scrollArea);
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    // Computer ID group
    mComputerIdGroup = new QGroupBox("Computer ID");
    QVBoxLayout *computerIdLayout = new QVBoxLayout(mComputerIdGroup);

    mComputerIdLineEdit = new QLineEdit(mComputerIdGroup);
    mComputerIdLineEdit->setPlaceholderText(QSysInfo::machineHostName());
    mComputerIdLineEdit->setMinimumHeight(28);

    QLabel *computerIdHint = new QLabel(
        "Identifies this computer/node in all Zenoh key prefixes. "
        "Defaults to the computer hostname. Change this to match the publisher/subscriber "
        "on the other machine so topics align across sessions.");
    computerIdHint->setWordWrap(true);
    computerIdHint->setStyleSheet("QLabel { color: #666; font-size: 11px; }");

    QLabel *computerIdFormatLabel = new QLabel(
        "Key format:  cvdev/<b>computer_id</b>/&lt;flow_filename&gt;/&lt;node_id&gt;/output/&lt;port&gt;/data");
    computerIdFormatLabel->setWordWrap(true);
    computerIdFormatLabel->setStyleSheet("QLabel { color: #555; font-size: 10px; }");

    computerIdLayout->addWidget(mComputerIdLineEdit);
    computerIdLayout->addWidget(computerIdHint);
    computerIdLayout->addWidget(computerIdFormatLabel);
    contentLayout->addWidget(mComputerIdGroup);

    // Mode selection group
    mModeGroup = new QGroupBox("Zenoh Mode");
    QVBoxLayout *modeLayout = new QVBoxLayout(mModeGroup);

    mModeComboBox = new QComboBox();
    mModeComboBox->addItem("Peer (Default)", "peer");
    mModeComboBox->addItem("Client (Lightweight)", "client");
    mModeComboBox->addItem("Router (Infrastructure)", "router");

    mModeHintLabel = new QLabel();
    mModeHintLabel->setWordWrap(true);
    mModeHintLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");

    modeLayout->addWidget(mModeComboBox);
    modeLayout->addWidget(mModeHintLabel);
    contentLayout->addWidget(mModeGroup);

    // Connection settings group
    mConnectionGroup = new QGroupBox("Connection Settings");
    QVBoxLayout *connectionLayout = new QVBoxLayout(mConnectionGroup);

    QLabel *connectionInfoLabel = new QLabel(
        "Optional endpoints. Leave fields empty to use default Zenoh behavior.");
    connectionInfoLabel->setWordWrap(true);
    connectionInfoLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    connectionLayout->addWidget(connectionInfoLabel);

    QLabel *connectLabel = new QLabel("Connect Endpoints");
    connectLabel->setStyleSheet("QLabel { font-weight: 600; }");
    connectionLayout->addWidget(connectLabel);

    mConnectLineEdit = new QLineEdit();
    mConnectLineEdit->setPlaceholderText("tcp/192.168.1.100:7447, tcp/192.168.1.101:7447");
    mConnectLineEdit->setMinimumHeight(28);
    connectionLayout->addWidget(mConnectLineEdit);

    mConnectHintLabel = new QLabel("Example: tcp/192.168.1.100:7447");
    mConnectHintLabel->setWordWrap(true);
    mConnectHintLabel->setStyleSheet("QLabel { color: #666; font-size: 10px; }");
    connectionLayout->addWidget(mConnectHintLabel);

    QLabel *listenLabel = new QLabel("Listen Endpoints");
    listenLabel->setStyleSheet("QLabel { font-weight: 600; }");
    connectionLayout->addWidget(listenLabel);

    mListenLineEdit = new QLineEdit();
    mListenLineEdit->setPlaceholderText("tcp/0.0.0.0:7447");
    mListenLineEdit->setMinimumHeight(28);
    connectionLayout->addWidget(mListenLineEdit);

    mListenHintLabel = new QLabel("Example: tcp/0.0.0.0:7447");
    mListenHintLabel->setWordWrap(true);
    mListenHintLabel->setStyleSheet("QLabel { color: #666; font-size: 10px; }");
    connectionLayout->addWidget(mListenHintLabel);

    connectionLayout->addSpacing(2);

    contentLayout->addWidget(mConnectionGroup);

    // Transport options group
    mTransportOptionsGroup = new QGroupBox("Transport Options");
    QVBoxLayout *transportLayout = new QVBoxLayout(mTransportOptionsGroup);

    mMulticastCheckBox = new QCheckBox("Enable Multicast Scouting");
    mMulticastCheckBox->setToolTip("Automatically discover peers on local network via multicast");
    mMulticastCheckBox->setChecked(true);

    mSharedMemoryCheckBox = new QCheckBox("Enable Shared Memory Transport");
    mSharedMemoryCheckBox->setToolTip("Use zero-copy shared memory for local process communication");
    mSharedMemoryCheckBox->setChecked(true);
    transportLayout->addWidget(mMulticastCheckBox);
    transportLayout->addWidget(mSharedMemoryCheckBox);
    contentLayout->addWidget(mTransportOptionsGroup);

    // Security settings group (Unified/Shared Security)
    mSecurityGroup = new QGroupBox("Transport Security");
    mSecurityGroup->setCheckable(true);
    mSecurityGroup->setChecked(false);
    QFormLayout *securityLayout = new QFormLayout(mSecurityGroup);

    mSharedSecretLineEdit = new QLineEdit();
    mSharedSecretLineEdit->setEchoMode(QLineEdit::Password);
    mSharedSecretLineEdit->setPlaceholderText("Pre-shared key secret");
    mSharedSecretLineEdit->setMinimumHeight(28);

    QHBoxLayout *secretLayout = new QHBoxLayout();
    secretLayout->setContentsMargins(0, 0, 0, 0);
    secretLayout->setSpacing(4);

    QPushButton *toggleShowButton = new QPushButton("Show");
    toggleShowButton->setCheckable(true);
    toggleShowButton->setMinimumHeight(28);
    toggleShowButton->setMaximumWidth(60);

    connect(toggleShowButton, &QPushButton::toggled, this, [this, toggleShowButton](bool checked) {
        if (checked) {
            mSharedSecretLineEdit->setEchoMode(QLineEdit::Normal);
            toggleShowButton->setText("Hide");
        } else {
            mSharedSecretLineEdit->setEchoMode(QLineEdit::Password);
            toggleShowButton->setText("Show");
        }
    });

    secretLayout->addWidget(mSharedSecretLineEdit);
    secretLayout->addWidget(toggleShowButton);

    securityLayout->addRow("Shared Secret", secretLayout);
    contentLayout->addWidget(mSecurityGroup);

    // Configuration preview group
    mPreviewGroup = new QGroupBox("Configuration Preview");
    QVBoxLayout *previewLayout = new QVBoxLayout(mPreviewGroup);

    mPreviewTextEdit = new QTextEdit();
    mPreviewTextEdit->setReadOnly(true);
    mPreviewTextEdit->setMaximumHeight(150);
    mPreviewTextEdit->setStyleSheet("QTextEdit { font-family: monospace; font-size: 11px; }");

    previewLayout->addWidget(mPreviewTextEdit);
    contentLayout->addWidget(mPreviewGroup);

    contentLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    // Action buttons
    QHBoxLayout *actionLayout = new QHBoxLayout();
    mDefaultsButton = new QPushButton("Restore Defaults");
    mTestButton = new QPushButton("Test Connection");
    actionLayout->addWidget(mDefaultsButton);
    actionLayout->addWidget(mTestButton);
    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void ZenohSettingsDialog::loadSettings()
{
    QSettings settings(CVDev::cvdevIniPath(), QSettings::IniFormat);
    
    // Look up computer_id (always in [General] - root)
    QString computerId = settings.value("computer_id", "").toString().trimmed();
    if (computerId.isEmpty()) {
        computerId = QSysInfo::machineHostName();
    }
    mComputerIdLineEdit->setText(computerId);

    settings.beginGroup("Zenoh");

    QString mode = settings.value("mode", "peer").toString();
    int modeIndex = mModeComboBox->findData(mode);
    if (modeIndex >= 0) {
        mModeComboBox->setCurrentIndex(modeIndex);
    }

    mConnectLineEdit->setText(settings.value("connect", "").toString());
    mListenLineEdit->setText(settings.value("listen", "").toString());
    mMulticastCheckBox->setChecked(settings.value("multicast", true).toBool());
    mSharedMemoryCheckBox->setChecked(settings.value("sharedMemory", true).toBool());
    settings.endGroup();

    // Unified security settings
    settings.beginGroup("Security");
    bool secEnabled = settings.value("enabled", false).toBool();
    QString sharedSecret = settings.value("shared_secret", "").toString();
    settings.endGroup();

    mSecurityGroup->setChecked(secEnabled);
    if (secEnabled && sharedSecret.isEmpty()) {
        sharedSecret = QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(12);
    }
    mSharedSecretLineEdit->setText(sharedSecret);
}

void ZenohSettingsDialog::saveSettings()
{
    QSettings settings(CVDev::cvdevIniPath(), QSettings::IniFormat);
    
    // Save computer_id always in [General] - root
    settings.setValue("computer_id", getComputerId());

    // Save to Zenoh group
    settings.beginGroup("Zenoh");
    settings.setValue("mode", getMode());
    settings.setValue("connect", getConnectEndpoints());
    settings.setValue("listen", getListenEndpoints());
    settings.setValue("multicast", isMulticastEnabled());
    settings.setValue("sharedMemory", isSharedMemoryEnabled());
    settings.endGroup();

    // Save unified security settings
    settings.beginGroup("Security");
    settings.setValue("enabled", isSecurityEnabled());
    QString secret = getSharedSecret();
    if (isSecurityEnabled() && secret.isEmpty()) {
        secret = QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(12);
        mSharedSecretLineEdit->setText(secret);
    }
    settings.setValue("shared_secret", secret);
    settings.endGroup();
}

QString ZenohSettingsDialog::getComputerId() const
{
    QString id = mComputerIdLineEdit->text().trimmed();
    return id.isEmpty() ? QSysInfo::machineHostName() : id;
}

QString ZenohSettingsDialog::getMode() const
{
    return mModeComboBox->currentData().toString();
}

QString ZenohSettingsDialog::getConnectEndpoints() const
{
    return mConnectLineEdit->text().trimmed();
}

QString ZenohSettingsDialog::getListenEndpoints() const
{
    return mListenLineEdit->text().trimmed();
}

bool ZenohSettingsDialog::isMulticastEnabled() const
{
    return mMulticastCheckBox->isChecked();
}

bool ZenohSettingsDialog::isSharedMemoryEnabled() const
{
    return mSharedMemoryCheckBox->isChecked();
}

bool ZenohSettingsDialog::isSecurityEnabled() const
{
    return mSecurityGroup->isChecked();
}

QString ZenohSettingsDialog::getSharedSecret() const
{
    return mSharedSecretLineEdit->text();
}

QString ZenohSettingsDialog::getConfigString() const
{
    // Build JSON5 configuration string
    QStringList config;
    config << "{";

    // Mode
    config << QString("  mode: \"%1\",").arg(getMode());

    // Connect endpoints
    QString connectEndpoints = getConnectEndpoints();
    if (!connectEndpoints.isEmpty()) {
        QStringList endpoints = connectEndpoints.split(',', Qt::SkipEmptyParts);
        config << "  connect: {";
        config << "    endpoints: [";
        for (int i = 0; i < endpoints.size(); ++i) {
            QString ep = endpoints[i].trimmed();
            config << QString("      \"%1\"%2").arg(ep).arg(i < endpoints.size() - 1 ? "," : "");
        }
        config << "    ]";
        config << "  },";
    }

    // Listen endpoints
    QString listenEndpoints = getListenEndpoints();
    if (!listenEndpoints.isEmpty()) {
        QStringList endpoints = listenEndpoints.split(',', Qt::SkipEmptyParts);
        config << "  listen: {";
        config << "    endpoints: [";
        for (int i = 0; i < endpoints.size(); ++i) {
            QString ep = endpoints[i].trimmed();
            config << QString("      \"%1\"%2").arg(ep).arg(i < endpoints.size() - 1 ? "," : "");
        }
        config << "    ]";
        config << "  },";
    }

    // Scouting (multicast)
    config << "  scouting: {";
    config << "    multicast: {";
    config << QString("      enabled: %1").arg(isMulticastEnabled() ? "true" : "false");
    config << "    }";
    config << "  },";

    // Shared memory
    config << "  transport: {";
    config << "    shared_memory: {";
    config << QString("      enabled: %1").arg(isSharedMemoryEnabled() ? "true" : "false");
    config << "    }";
    config << "  }";

    config << "}";

    return config.join("\n");
}

QString ZenohSettingsDialog::buildPreviewText() const
{
    QStringList preview;
    const auto mode = TransportModeManager::instance().getTransportMode();

    QString transportLabel = "Qt";
    if (mode == TransportMode::ZenohOnly) {
        transportLabel = "Zenoh";
    }

    preview << "=== Zenoh Configuration ===";
    preview << "";
    preview << QString("Computer ID: %1").arg(getComputerId());
    preview << "";
    preview << QString("Operation Mode: %1").arg(transportLabel);
    preview << "";
    preview << QString("Mode: %1").arg(getMode().toUpper());
    preview << "";

    QString connectEndpoints = getConnectEndpoints();
    if (!connectEndpoints.isEmpty()) {
        preview << "Connect to:";
        QStringList endpoints = connectEndpoints.split(',', Qt::SkipEmptyParts);
        for (const QString &ep : endpoints) {
            preview << QString("  • %1").arg(ep.trimmed());
        }
        preview << "";
    } else {
        preview << "Connect: Auto-discovery (multicast/scouting)";
        preview << "";
    }

    QString listenEndpoints = getListenEndpoints();
    if (!listenEndpoints.isEmpty()) {
        preview << "Listen on:";
        QStringList endpoints = listenEndpoints.split(',', Qt::SkipEmptyParts);
        for (const QString &ep : endpoints) {
            preview << QString("  • %1").arg(ep.trimmed());
        }
        preview << "";
    } else {
        preview << "Listen: Default (auto-assigned)";
        preview << "";
    }

    preview << "Transport:";
    preview << QString("  • Multicast Scouting: %1").arg(isMulticastEnabled() ? "Enabled" : "Disabled");
    preview << QString("  • Shared Memory: %1").arg(isSharedMemoryEnabled() ? "Enabled" : "Disabled");
    preview << "";
    preview << "Security:";
    preview << QString("  • Security Enabled: %1").arg(isSecurityEnabled() ? "Yes" : "No");
    if (isSecurityEnabled()) {
        preview << QString("  • Shared Secret: %1").arg(getSharedSecret().isEmpty() ? "(Not Configured)" : "********");
    }

    return preview.join("\n");
}

void ZenohSettingsDialog::updatePreview()
{
    mPreviewTextEdit->setPlainText(buildPreviewText());
}

void ZenohSettingsDialog::onModeChanged(int index)
{
    QString mode = mModeComboBox->itemData(index).toString();

    if (mode == "peer") {
        mModeHintLabel->setText(
            "Peer mode (default): Participates in scouting and can route messages. "
            "Best for distributed networks where all nodes can communicate directly."
        );
        mListenLineEdit->setEnabled(true);
        mMulticastCheckBox->setEnabled(true);
    } else if (mode == "client") {
        mModeHintLabel->setText(
            "Client mode: Lightweight. Connects to routers but doesn't route messages. "
            "Ideal for edge devices or when minimizing resource usage."
        );
        mListenLineEdit->setEnabled(false);
        mMulticastCheckBox->setEnabled(true);
    } else if (mode == "router") {
        mModeHintLabel->setText(
            "Router mode: Infrastructure mode. Routes messages between peers and clients. "
            "Use for dedicated routing infrastructure in large networks."
        );
        mListenLineEdit->setEnabled(true);
        mMulticastCheckBox->setEnabled(true);
    }

    updatePreview();
}

void ZenohSettingsDialog::onRestoreDefaults()
{
    mModeComboBox->setCurrentIndex(0); // peer
    mComputerIdLineEdit->setText(QSysInfo::machineHostName());
    mConnectLineEdit->clear();
    mListenLineEdit->clear();
    mMulticastCheckBox->setChecked(true);
    mSharedMemoryCheckBox->setChecked(true);
    mSecurityGroup->setChecked(false);
    mSharedSecretLineEdit->clear();
    updatePreview();

    QMessageBox::information(this, "Defaults Restored",
        "All settings have been restored to defaults:\n\n"
        "• Computer ID: " + QSysInfo::machineHostName() + "\n"
        "• Mode: Peer\n"
        "• Connect: Auto-discovery\n"
        "• Listen: Default\n"
        "• Multicast: Enabled\n"
        "• Shared Memory: Enabled\n"
        "• Transport Security: Disabled"
    );
}

void ZenohSettingsDialog::onTestConnection()
{
#ifdef ZENOH_ENABLED
    QString configStr = getConfigString();
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Testing Zenoh Configuration");
    msgBox.setText("Testing connection with current settings...");
    msgBox.setStandardButtons(QMessageBox::NoButton);
    msgBox.show();
    QApplication::processEvents();

    // Try to initialize a test session
    QString testSessionId = "test-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Note: This is a simplified test. In production, you might want to
    // create a separate test instance rather than using the singleton.
    bool success = ZenohBridge::instance().initialize(testSessionId, configStr);
    
    if (success) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("✓ Connection Successful");
        msgBox.setInformativeText(
            QString("Zenoh session initialized successfully.\n\n"
                    "Mode: %1\n"
                    "Session: %2\n\n"
                    "Note: This is a test connection. "
                    "Apply settings and restart to use the new configuration.")
                .arg(getMode())
                .arg(testSessionId)
        );
    } else {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("✗ Connection Failed");
        msgBox.setInformativeText(
            "Failed to initialize Zenoh session.\n\n"
            "Possible causes:\n"
            "• Invalid endpoint format\n"
            "• Unreachable router address\n"
            "• Network configuration issues\n"
            "• Zenoh not installed/compiled\n\n"
            "Check the configuration and try again."
        );
    }
    
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
#else
    QMessageBox::information(this, "Zenoh Not Available",
        "Zenoh support is not compiled into this build.\n\n"
        "To enable Zenoh:\n"
        "1. Install Zenoh C library (zenohc)\n"
        "2. Reconfigure with CMake\n"
        "3. Rebuild the application\n\n"
        "See ZENOH_HYBRID_IMPLEMENTATION.md for details."
    );
#endif
}
