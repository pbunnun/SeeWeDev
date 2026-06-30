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
 * @file TransportMode.hpp
 * @brief Transport mode enumeration and helper utilities.
 *
 * Defines available transport modes for CVDev and CVDevDaemon, including local
 * execution (Qt only, Qt+Zenoh, Qt+CycloneDDS), distributed backends (Zenoh only,
 * CycloneDDS only), and remote operation modes (Remote Monitor, Remote Control).
 * Provides constexpr helper functions for mode classification and capability checking.
 */

#pragma once

enum class TransportMode
{
    QtOnly = 0,
    ZenohOnly = 1,
    CycloneDDSOnly = 2
};

inline constexpr const char* kTransportModeQtOnly = "qt_only";
inline constexpr const char* kTransportModeZenohOnly = "zenoh_only";
inline constexpr const char* kTransportModeCycloneDDSOnly = "cyclonedds_only";

inline constexpr bool isZenohTransport(TransportMode mode)
{
    return mode == TransportMode::ZenohOnly;
}

inline constexpr bool isDDSTransport(TransportMode mode)
{
    return mode == TransportMode::CycloneDDSOnly;
}

inline constexpr bool isLocalRuntimeTransport(TransportMode mode)
{
    return mode == TransportMode::QtOnly ||
           mode == TransportMode::ZenohOnly ||
           mode == TransportMode::CycloneDDSOnly;
}

inline constexpr bool requiresZenoh(TransportMode mode)
{
    return isZenohTransport(mode);
}

inline constexpr bool requiresDDS(TransportMode mode)
{
    return isDDSTransport(mode);
}
