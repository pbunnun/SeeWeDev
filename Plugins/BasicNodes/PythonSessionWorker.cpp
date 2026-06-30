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

#include "PythonSessionWorker.hpp"

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
const char* kPythonHostScript = R"PY(
import base64
import json
import sys
import traceback

import numpy as np

try:
    import cv2
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False

_session = {}


def decode_input(value):
    if value is None:
        return None

    if isinstance(value, str):
        if value == 'null':
            return None
        try:
            payload = json.loads(value)
        except Exception:
            return None
    elif isinstance(value, dict):
        payload = value
    else:
        return None

    data_type = payload.get('type')

    if data_type == 'image' and HAS_CV2:
        img_data = base64.b64decode(payload.get('data', ''))
        nparr = np.frombuffer(img_data, np.uint8)
        return cv2.imdecode(nparr, cv2.IMREAD_UNCHANGED)

    if data_type == 'double':
        return float(payload.get('data', 0.0))

    if data_type == 'int':
        return int(payload.get('data', 0))

    if data_type == 'string':
        return str(payload.get('data', ''))

    if data_type == 'info' or data_type == 'information':
        return str(payload.get('data', ''))

    if data_type == 'point':
        return {'x': payload.get('x', 0), 'y': payload.get('y', 0)}

    if data_type == 'rect':
        return {
            'x': payload.get('x', 0),
            'y': payload.get('y', 0),
            'width': payload.get('width', 0),
            'height': payload.get('height', 0),
        }

    if data_type == 'size':
        return {'width': payload.get('width', 0), 'height': payload.get('height', 0)}

    if data_type == 'sync':
        return bool(payload.get('data', False))

    return None


def encode_output(value, output_name):
    if HAS_CV2 and isinstance(value, np.ndarray):
        ok, buffer = cv2.imencode('.png', value)
        if not ok:
            raise ValueError(f'{output_name} image encode failed')

        return {
            'type': 'image',
            'data': base64.b64encode(buffer).decode('utf-8'),
            'rows': value.shape[0],
            'cols': value.shape[1],
            'channels': value.shape[2] if len(value.shape) > 2 else 1,
            'dtype': int(value.dtype.num),
        }

    if isinstance(value, (bool, np.bool_)):
        return {'type': 'sync', 'data': bool(value)}

    if isinstance(value, (int, np.integer)):
        return {'type': 'int', 'data': int(value)}

    if isinstance(value, (float, np.floating)):
        return {'type': 'double', 'data': float(value)}

    if isinstance(value, str):
        return {'type': 'string', 'data': value}

    if isinstance(value, dict):
        if 'x' in value and 'y' in value and 'width' not in value:
            return {'type': 'point', 'x': value['x'], 'y': value['y']}

        if 'x' in value and 'y' in value and 'width' in value and 'height' in value:
            return {
                'type': 'rect',
                'x': value['x'],
                'y': value['y'],
                'width': value['width'],
                'height': value['height'],
            }

        if 'width' in value and 'height' in value:
            return {'type': 'size', 'width': value['width'], 'height': value['height']}

    raise TypeError(f"{output_name} has unsupported type: {type(value).__name__}")


def send_response(payload):
    print(json.dumps(payload), flush=True)


while True:
    line = sys.stdin.readline()
    if line == '':
        break

    line = line.strip()
    if not line:
        continue

    try:
        request = json.loads(line)
    except Exception as exc:
        send_response({'ok': False, 'error': f'Invalid request JSON: {exc}'})
        continue

    command = request.get('cmd', 'execute')
    if command == 'quit':
        break

    if command != 'execute':
        send_response({'ok': False, 'error': f"Unknown command: {command}"})
        continue

    code = request.get('code', '')
    num_outputs = int(request.get('num_outputs', 0))
    inputs = request.get('inputs', {})

    for key, value in inputs.items():
        _session[key] = decode_input(value)

    try:
        exec(code, _session)
    except Exception as exc:
        send_response({
            'ok': False,
            'error': f'{type(exc).__name__}: {exc}',
            'traceback': traceback.format_exc(),
        })
        continue

    outputs = {}
    try:
        for index in range(num_outputs):
            output_name = f'output{index}'
            if output_name in _session:
                outputs[output_name] = json.dumps(encode_output(_session[output_name], output_name))
    except Exception as exc:
        send_response({
            'ok': False,
            'error': f'{type(exc).__name__}: {exc}',
            'traceback': traceback.format_exc(),
        })
        continue

    send_response({'ok': True, 'outputs': outputs})
)PY";
}

PythonSessionWorker::PythonSessionWorker( QObject* parent )
    : QObject( parent )
{
}

PythonSessionWorker::~PythonSessionWorker()
{
    stopSession();
}

bool
PythonSessionWorker::createHostScriptFile()
{
    if( mpHostScriptFile )
    {
        mpHostScriptFile->close();
        delete mpHostScriptFile;
        mpHostScriptFile = nullptr;
    }

    mpHostScriptFile = new QTemporaryFile();
    mpHostScriptFile->setAutoRemove( true );
    mpHostScriptFile->setFileTemplate( QDir::tempPath() + "/cvdev_python_host_XXXXXX.py" );

    if( !mpHostScriptFile->open() )
        return false;

    const QByteArray scriptBytes( kPythonHostScript );
    if( mpHostScriptFile->write( scriptBytes ) != scriptBytes.size() )
        return false;

    mpHostScriptFile->flush();
    mpHostScriptFile->close();
    return true;
}

void
PythonSessionWorker::cleanupProcess()
{
    if( mpProcess )
    {
        mpProcess->disconnect( this );
        delete mpProcess;
        mpProcess = nullptr;
    }

    msStdoutBuffer.clear();
    msStderrBuffer.clear();
}

void
PythonSessionWorker::startSession()
{
    stopSession();

    if( !createHostScriptFile() )
    {
        Q_EMIT sessionFailed( "Cannot create Python host script file" );
        return;
    }

    cleanupProcess();

    mbStopping = false;
    mpProcess = new QProcess( this );

    connect( mpProcess, &QProcess::readyReadStandardOutput, this, &PythonSessionWorker::onReadyReadStdout );
    connect( mpProcess, &QProcess::readyReadStandardError, this, &PythonSessionWorker::onReadyReadStderr );
    connect( mpProcess, &QProcess::errorOccurred, this, &PythonSessionWorker::onProcessError );
    connect( mpProcess,
             QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
             this,
             &PythonSessionWorker::onProcessFinished );

    connect( mpProcess,
             &QProcess::started,
             this,
             [this]()
             {
                 Q_EMIT sessionStarted();
             } );

    QStringList args;
    args << mpHostScriptFile->fileName();
    mpProcess->start( msExecutablePath, args );

    if( !mpProcess->waitForStarted( 3000 ) )
        Q_EMIT sessionFailed( "Cannot start Python interpreter" );
}

void
PythonSessionWorker::stopSession()
{
    mbStopping = true;

    if( mpProcess )
    {
        if( mpProcess->state() == QProcess::Running )
        {
            const QByteArray quitCommand = "{\"cmd\":\"quit\"}\n";
            mpProcess->write( quitCommand );
            mpProcess->waitForBytesWritten( 500 );

            if( !mpProcess->waitForFinished( 1500 ) )
            {
                mpProcess->terminate();
                if( !mpProcess->waitForFinished( 1000 ) )
                    mpProcess->kill();
            }
        }

        cleanupProcess();
    }

    if( mpHostScriptFile )
    {
        mpHostScriptFile->close();
        delete mpHostScriptFile;
        mpHostScriptFile = nullptr;
    }
}

void
PythonSessionWorker::executeFrame( const QString& code, const QString& inputsJson, int numOutputs )
{
    if( !mpProcess || mpProcess->state() != QProcess::Running )
    {
        Q_EMIT executionError( "Python session is not running" );
        return;
    }

    QJsonObject request;
    request["cmd"] = "execute";
    request["code"] = code;
    request["num_outputs"] = numOutputs;

    QJsonDocument inputsDoc = QJsonDocument::fromJson( inputsJson.toUtf8() );
    request["inputs"] = inputsDoc.isObject() ? inputsDoc.object() : QJsonObject();

    const QByteArray payload = QJsonDocument( request ).toJson( QJsonDocument::Compact ) + '\n';
    if( mpProcess->write( payload ) != payload.size() )
    {
        Q_EMIT executionError( "Failed to send execution request to Python session" );
        return;
    }

    mpProcess->waitForBytesWritten( 500 );
}

void
PythonSessionWorker::setExecutablePath( const QString& executablePath )
{
    msExecutablePath = executablePath.trimmed().isEmpty() ? QStringLiteral( "python3" ) : executablePath.trimmed();
}

void
PythonSessionWorker::onReadyReadStdout()
{
    if( !mpProcess )
        return;

    msStdoutBuffer += QString::fromUtf8( mpProcess->readAllStandardOutput() );

    int newlineIndex = msStdoutBuffer.indexOf( '\n' );
    while( newlineIndex >= 0 )
    {
        const QString line = msStdoutBuffer.left( newlineIndex ).trimmed();
        msStdoutBuffer.remove( 0, newlineIndex + 1 );

        if( line.isEmpty() )
        {
            newlineIndex = msStdoutBuffer.indexOf( '\n' );
            continue;
        }

        const QJsonDocument responseDoc = QJsonDocument::fromJson( line.toUtf8() );
        if( !responseDoc.isObject() )
        {
            Q_EMIT executionError( "Python session returned invalid JSON response" );
            newlineIndex = msStdoutBuffer.indexOf( '\n' );
            continue;
        }

        const QJsonObject responseObj = responseDoc.object();
        if( responseObj["ok"].toBool( false ) )
        {
            const QJsonObject outputsObj = responseObj["outputs"].toObject();
            Q_EMIT resultReady( QString::fromUtf8( QJsonDocument( outputsObj ).toJson( QJsonDocument::Compact ) ) );
        }
        else
        {
            QString errorMessage = responseObj["error"].toString( "Unknown execution error" );
            const QString traceback = responseObj["traceback"].toString();
            if( !traceback.isEmpty() )
                errorMessage += "\n" + traceback;
            Q_EMIT executionError( errorMessage );
        }

        newlineIndex = msStdoutBuffer.indexOf( '\n' );
    }
}

void
PythonSessionWorker::onReadyReadStderr()
{
    if( !mpProcess )
        return;

    msStderrBuffer += QString::fromUtf8( mpProcess->readAllStandardError() );
}

void
PythonSessionWorker::onProcessError( QProcess::ProcessError error )
{
    if( mbStopping )
        return;

    Q_EMIT sessionFailed( QStringLiteral( "Python process error %1" ).arg( static_cast<int>( error ) ) );
}

void
PythonSessionWorker::onProcessFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if( mbStopping )
        return;

    QString message = QStringLiteral( "Python session exited (code=%1, status=%2)" )
                          .arg( exitCode )
                          .arg( exitStatus == QProcess::NormalExit ? "normal" : "crash" );

    if( !msStderrBuffer.trimmed().isEmpty() )
    {
        message += "\n" + msStderrBuffer.trimmed().left( 300 );
    }

    Q_EMIT sessionFailed( message );
}
