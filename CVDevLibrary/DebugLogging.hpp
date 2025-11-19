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

#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QLoggingCategory>

// Qt Logging Category for debug messages
// Can be controlled at runtime via:
// - Environment variable: QT_LOGGING_RULES="DebugLogging.info=false" or "DebugLogging.warning=false"
// - In code: QLoggingCategory::setFilterRules("DebugLogging.info=false");
Q_DECLARE_LOGGING_CATEGORY(DebugLogging)

// Logging macros with datetime and line number using Qt Logging Category
#define DEBUG_LOG_INFO() qCInfo(DebugLogging) << "[[Info] " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "|" << QFileInfo(__FILE__).fileName() << ":" << __LINE__ << "]"
#define DEBUG_LOG_WARNING() qCWarning(DebugLogging) << "[[Warning] " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "|" << QFileInfo(__FILE__).fileName() << ":" << __LINE__ << "]"
