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

#include "MainWindow.hpp"

#include <QApplication>
#include <QSurfaceFormat>

#include <QFileDialog>
#include <QCryptographicHash>
#include <QMessageBox>

#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QStandardPaths>
#include <QLoggingCategory>

//#define __SAVE_LOG__
//#define __ENABLE_DEBUG_LOG_INFO__
//#define __ENABLE_DEBUG_LOG_WARNING__

#define __CHECK_LICENSE_KEY__

#if defined (__SAVE_LOG__)

QMutex lLockMutex;
QFile lLogFile;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&lLockMutex);

    QTextStream out(&lLogFile);
    out << msg << "\n";
    out.flush();
}
#endif

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0) )
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/cvdev-64.png"));
    QCoreApplication::setOrganizationName("NECTEC");
    QCoreApplication::setApplicationName("CVDev");

    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#if defined( __SAVE_LOG__)
    QDir logDir(homePath + "/.CVDev/log");
    if( !logDir.exists() )
        logDir.mkpath(".");

    auto log_filename = logDir.filePath("log-" +  QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".txt");
    lLogFile.setFileName(log_filename);

    if( !lLogFile.open(QIODevice::Append | QIODevice::Text) )
    {
        QMessageBox::critical(nullptr, "CVDev", "<p>Could not open log file! "
                            "Please check a storage free space or log directory permission/</p>");
        return 1;
    }

    qInstallMessageHandler(messageHandler);
#endif   // __SAVE_LOG__

    // Configure debug logging - must set both rules together
#if defined(__ENABLE_DEBUG_LOG_INFO__) && defined(__ENABLE_DEBUG_LOG_WARNING__)
    QLoggingCategory::setFilterRules("DebugLogging.info=true\nDebugLogging.warning=true");
#elif defined(__ENABLE_DEBUG_LOG_INFO__)
    QLoggingCategory::setFilterRules("DebugLogging.info=true\nDebugLogging.warning=false");
#elif defined(__ENABLE_DEBUG_LOG_WARNING__)
    QLoggingCategory::setFilterRules("DebugLogging.info=false\nDebugLogging.warning=true");
#else
    QLoggingCategory::setFilterRules("DebugLogging.info=false\nDebugLogging.warning=false");
#endif

    MainWindow window;
    window.show();
    return app.exec();
}
