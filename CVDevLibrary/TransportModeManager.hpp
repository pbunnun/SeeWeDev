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
 * @file TransportModeManager.hpp
 * @brief Singleton manager for global transport mode and daemon identity state.
 *
 * Provides centralized access to current transport mode, remote backend selection,
 * and local daemon identity (compute node ID, daemon process ID) for distribution
 * across the application without threading complexity.
 */

#pragma once

#include "TransportMode.hpp"
#include <QString>
#include <QtNodes/Definitions>

/**
 * @class TransportModeManager
 * @brief Singleton managing global transport mode and daemon identity state.
 *
 * Thread-safe singleton instance that tracks:
 * - Current transport mode (QtOnly, remote monitor, etc.)
 * - Remote transport backend (Zenoh or CycloneDDS)
 * - Local daemon identity for discovery and targeting
 *
 * Use instance() to access the singleton; all state mutations are thread-safe
 * via Qt's implicit shared containers and atomic setters.
 */
class TransportModeManager
{
public:
    /**
     * @brief Returns the global singleton instance.
     */
    static TransportModeManager& instance();

    /**
     * @brief Sets the current transport mode and updates remote backend accordingly.
     *
     * When mode changes, remote backend is automatically updated based on
     * the new mode's backend compatibility (unless in monitor/control modes).
     *
     * @param mode Transport mode to activate.
     */
    void setTransportMode(TransportMode mode);

    /**
     * @brief Returns the current transport mode.
     */
    TransportMode getTransportMode() const;

    /**
     * @brief Converts a settings string representation of transport mode to TransportMode enum.
     */
    static TransportMode transportModeFromSetting(const QString& setting);

    /**
     * @brief Converts a TransportMode enum to its settings string representation.
     */
    static QString settingFromTransportMode(TransportMode mode);

    /**
     * @brief Generates the topic key for node output data transport.
     */
    static QString makeTransportOutputTopicKey(const QString& computerId,
                                               const QString& flowFilename,
                                               const QString& nodeId,
                                               QtNodes::PortIndex outPortIndex);

private:
    TransportModeManager() = default;

    TransportModeManager(const TransportModeManager&) = delete;
    TransportModeManager& operator=(const TransportModeManager&) = delete;

    /** Current transport mode. */
    TransportMode mTransportMode{TransportMode::QtOnly};
};
