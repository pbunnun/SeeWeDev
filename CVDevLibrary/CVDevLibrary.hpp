//Copyright © 2022 - 2026, NECTEC, all rights reserved

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

#include <QtCore/QtGlobal>
#include <QStandardPaths>
#include <QString>

#if defined(CVDEV_LIBRARY)
#  define CVDEVSHAREDLIB_EXPORT Q_DECL_EXPORT
#else
#  define CVDEVSHAREDLIB_EXPORT Q_DECL_IMPORT
#endif

namespace CVDev
{

inline QString homePath()
{
	return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

inline QString appHomePath(const QString &appDirName = ".CVDev")
{
	return homePath() + "/" + appDirName;
}

/// Returns the canonical path to the shared CVDev settings INI file.
/// All components (MainWindow, ZenohSettingsDialog, CVDevDaemon) must
/// use this path so that settings are read and written to a single file.
inline QString cvdevIniPath()
{
	return appHomePath() + "/cvdev.ini";
}

} // namespace CVDev


