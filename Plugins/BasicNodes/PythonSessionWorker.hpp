//Copyright © 2026, NECTEC, all rights reserved

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

#include <QObject>
#include <QProcess>
#include <QTemporaryFile>
#include <QString>

class PythonSessionWorker : public QObject
{
    Q_OBJECT

public:
    explicit PythonSessionWorker( QObject* parent = nullptr );
    ~PythonSessionWorker() override;

public Q_SLOTS:
    void startSession();
    void stopSession();
    void executeFrame( const QString& code, const QString& inputsJson, int numOutputs );
    void setExecutablePath( const QString& executablePath );

Q_SIGNALS:
    void sessionStarted();
    void sessionFailed( const QString& errorMessage );
    void resultReady( const QString& outputsJson );
    void executionError( const QString& errorMessage );

private Q_SLOTS:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessError( QProcess::ProcessError error );
    void onProcessFinished( int exitCode, QProcess::ExitStatus exitStatus );

private:
    bool createHostScriptFile();
    void cleanupProcess();

    QProcess* mpProcess { nullptr };
    QTemporaryFile* mpHostScriptFile { nullptr };
    QString msExecutablePath { "python3" };

    QString msStdoutBuffer;
    QString msStderrBuffer;

    bool mbStopping { false };
};
