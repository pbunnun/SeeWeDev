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

//#define __SAVE_LOG__

#if defined (__SAVE_LOG__)
#include <QStandardPaths>

QMutex lLockMutex;
QFile lLogFile;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&lLockMutex);

    QString message;
    switch (type) {
    case QtDebugMsg:
        message = QString("Debug: %1").arg(msg);
        break;
    case QtInfoMsg:
        message = QString("Info: %1").arg(msg);
        break;
    case QtWarningMsg:
        message = QString("Warning: %1").arg(msg);
        break;
    case QtCriticalMsg:
        message = QString("Critical: %1").arg(msg);
        break;
    case QtFatalMsg:
        message = QString("Fatal: %1").arg(msg);
        break;
    }

    QTextStream out(&lLogFile);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ") << message << "\n";
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

#if defined( __SAVE_LOG__)
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
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


    MainWindow window;
    window.show();
    return app.exec();
}
