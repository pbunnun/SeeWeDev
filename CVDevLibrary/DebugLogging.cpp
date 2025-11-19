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

#include "DebugLogging.hpp"

// Define the debug logging category
// By default, it's enabled. To disable at runtime:
// - Environment: export QT_LOGGING_RULES="DebugLogging.info=false" or "DebugLogging.warning=false"
// - Code: QLoggingCategory::setFilterRules("DebugLogging.info=false");
Q_LOGGING_CATEGORY(DebugLogging, "DebugLogging")
