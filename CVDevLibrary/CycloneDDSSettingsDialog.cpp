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

/**
 * @file CycloneDDSSettingsDialog.cpp
 * @brief Implementation of CycloneDDS configuration dialog.
 *
 * Implements UI construction (4-group layout with hints and validation),
 * settings I/O (QSettings integration), and connection testing via CycloneDDSBridge.
 * Provides user-friendly configuration for DDS domain, partition, security,
 * and computer ID settings.
 */

#include "CycloneDDSSettingsDialog.hpp"

#include "CycloneDDSBridge.hpp"
#include "CVDevLibrary.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QSysInfo>
#include <QTimer>
#include <QPointer>
#include <QUuid>
#include <QApplication>
#include <QVBoxLayout>

CycloneDDSSettingsDialog::CycloneDDSSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("CycloneDDS Configuration");
    setMinimumWidth(560);

    setupUI();
    loadSettings();

    connect(mDefaultsButton, &QPushButton::clicked,
            this, &CycloneDDSSettingsDialog::onRestoreDefaults);
    connect(mTestButton, &QPushButton::clicked,
            this, &CycloneDDSSettingsDialog::onTestConnection);
    connect(mSecurityGroup, &QGroupBox::toggled,
            this, [this](bool checked) {
                if (checked && mSharedSecretLineEdit->text().isEmpty()) {
                    mSharedSecretLineEdit->setText(QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(12));
                }
            });
}

QString CycloneDDSSettingsDialog::getComputerId() const
{
    const QString computerId = mComputerIdLineEdit->text().trimmed();
    return computerId.isEmpty() ? QSysInfo::machineHostName() : computerId;
}

int CycloneDDSSettingsDialog::getDomainId() const
{
    return mDomainIdSpinBox->value();
}

QString CycloneDDSSettingsDialog::getPartition() const
{
    return mPartitionLineEdit->text().trimmed();
}

bool CycloneDDSSettingsDialog::isSecurityEnabled() const
{
    return mSecurityGroup->isChecked();
}

QString CycloneDDSSettingsDialog::getSharedSecret() const
{
    return mSharedSecretLineEdit->text();
}

void CycloneDDSSettingsDialog::onRestoreDefaults()
{
    mComputerIdLineEdit->setText(QSysInfo::machineHostName());
    mDomainIdSpinBox->setValue(0);
    mPartitionLineEdit->clear();
    mSecurityGroup->setChecked(false);
    mSharedSecretLineEdit->clear();
}

void CycloneDDSSettingsDialog::onTestConnection()
{
#ifdef CYCLONEDDS_ENABLED
    const bool ok = CycloneDDSBridge::instance().initialize(
        getComputerId(),
        getDomainId(),
        getPartition());
    QMessageBox::information(this,
                             "CycloneDDS Test",
                             ok
                                 ? "CycloneDDS bridge initialized successfully."
                                 : "CycloneDDS bridge initialization failed.");
#else
    QMessageBox::warning(this,
                         "CycloneDDS Not Available",
                         "CycloneDDS support is not enabled in this build.");
#endif
}

void CycloneDDSSettingsDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    mComputerIdGroup = new QGroupBox("Computer ID", this);
    auto *computerIdLayout = new QVBoxLayout(mComputerIdGroup);
    mComputerIdLineEdit = new QLineEdit(mComputerIdGroup);
    mComputerIdLineEdit->setPlaceholderText(QSysInfo::machineHostName());
    computerIdLayout->addWidget(mComputerIdLineEdit);
    computerIdLayout->addWidget(new QLabel("Identifies this computer/node namespace for CycloneDDS topics.", mComputerIdGroup));
    mainLayout->addWidget(mComputerIdGroup);

    mDdsGroup = new QGroupBox("DDS Configuration", this);
    auto *ddsForm = new QFormLayout(mDdsGroup);
    mDomainIdSpinBox = new QSpinBox(mDdsGroup);
    mDomainIdSpinBox->setRange(0, 232);
    mPartitionLineEdit = new QLineEdit(mDdsGroup);
    mPartitionLineEdit->setPlaceholderText("Optional DDS partition");
    ddsForm->addRow("Domain ID", mDomainIdSpinBox);
    ddsForm->addRow("Partition", mPartitionLineEdit);
    mainLayout->addWidget(mDdsGroup);

    // Security settings group (Unified/Shared Security)
    mSecurityGroup = new QGroupBox("Transport Security", this);
    mSecurityGroup->setCheckable(true);
    mSecurityGroup->setChecked(false);
    auto *securityLayout = new QFormLayout(mSecurityGroup);

    mSharedSecretLineEdit = new QLineEdit(mSecurityGroup);
    mSharedSecretLineEdit->setEchoMode(QLineEdit::Password);
    mSharedSecretLineEdit->setPlaceholderText("Pre-shared key secret");
    mSharedSecretLineEdit->setMinimumHeight(28);

    auto *secretLayout = new QHBoxLayout();
    secretLayout->setContentsMargins(0, 0, 0, 0);
    secretLayout->setSpacing(4);

    auto *toggleShowButton = new QPushButton("Show", mSecurityGroup);
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
    mainLayout->addWidget(mSecurityGroup);

    auto *actionsLayout = new QHBoxLayout();
    mDefaultsButton = new QPushButton("Restore Defaults", this);
    mTestButton = new QPushButton("Test Connection", this);
    actionsLayout->addWidget(mDefaultsButton);
    actionsLayout->addWidget(mTestButton);
    actionsLayout->addStretch();
    mainLayout->addLayout(actionsLayout);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void CycloneDDSSettingsDialog::loadSettings()
{
    QSettings settings(CVDev::cvdevIniPath(), QSettings::IniFormat);
    
    // Look up computer_id (always in [General] - root)
    QString computerId = settings.value("computer_id", "").toString().trimmed();
    if (computerId.isEmpty()) {
        computerId = QSysInfo::machineHostName();
    }
    mComputerIdLineEdit->setText(computerId);

    settings.beginGroup("CycloneDDS");

    mDomainIdSpinBox->setValue(settings.value("domain_id", 0).toInt());
    mPartitionLineEdit->setText(settings.value("partition", "").toString());
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

void CycloneDDSSettingsDialog::saveSettings()
{
    QSettings settings(CVDev::cvdevIniPath(), QSettings::IniFormat);
    
    // Save computer_id always in [General] - root
    settings.setValue("computer_id", getComputerId());

    settings.beginGroup("CycloneDDS");
    settings.setValue("domain_id", getDomainId());
    settings.setValue("partition", getPartition());
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
