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
 * @file TransportModeManager.cpp
 * @brief Implementation of transport mode and daemon identity singleton.
 *
 * Manages state persistence and backend compatibility when transport mode changes.
 */

#include "TransportModeManager.hpp"

/**
 * @brief Returns the global singleton instance.
 */
TransportModeManager& TransportModeManager::instance()
{
    static TransportModeManager sInstance;
    return sInstance;
}

/**
 * @brief Sets transport mode and updates backend compatibility.
 */
void TransportModeManager::setTransportMode(TransportMode mode)
{
    mTransportMode = mode;
}

/**
 * @brief Returns the current transport mode.
 */
TransportMode TransportModeManager::getTransportMode() const
{
    return mTransportMode;
}

TransportMode TransportModeManager::transportModeFromSetting(const QString& setting)
{
    if (setting == QString::fromLatin1(kTransportModeZenohOnly)) {
        return TransportMode::ZenohOnly;
    }
    if (setting == QString::fromLatin1(kTransportModeCycloneDDSOnly)) {
        return TransportMode::CycloneDDSOnly;
    }
    return TransportMode::QtOnly;
}

QString TransportModeManager::settingFromTransportMode(TransportMode mode)
{
    switch (mode) {
    case TransportMode::ZenohOnly:
        return QString::fromLatin1(kTransportModeZenohOnly);
    case TransportMode::CycloneDDSOnly:
        return QString::fromLatin1(kTransportModeCycloneDDSOnly);
    case TransportMode::QtOnly:
    default:
        return QString::fromLatin1(kTransportModeQtOnly);
    }
}

QString TransportModeManager::makeTransportOutputTopicKey(const QString& computerId,
                                                          const QString& flowFilename,
                                                          const QString& nodeId,
                                                          QtNodes::PortIndex outPortIndex)
{
    const QString effectiveFlowFilename = flowFilename.trimmed().isEmpty()
        ? QStringLiteral("Untitle")
        : flowFilename.trimmed();

    return QStringLiteral("cvdev/%1/%2/%3/output/%4/data")
        .arg(computerId.trimmed(),
             effectiveFlowFilename,
             nodeId,
             QString::number(static_cast<int>(outPortIndex)));
}

