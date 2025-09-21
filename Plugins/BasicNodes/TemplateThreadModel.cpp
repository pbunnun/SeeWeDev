//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "TemplateThreadModel.hpp"
#include <nodes/DataModelRegistry>
#include <QDebug>

TemplateThread::TemplateThread( QObject * parent )
    : QThread(parent)
{
    // Shall set this only after a working variable is ready to process data.
    // Might have a function to do this separately.
    mbThreadReady = true;
}


TemplateThread::
~TemplateThread()
{
    mbAbort = true;
    mWaitingSemaphore.release();
    wait();
}

void
TemplateThread::
    start_thread( )
{
    miThreadStatus = 1;
    if( !isRunning() )
        start();
}

void
TemplateThread::
    stop_thread()
{
    miThreadStatus = 2;
    mWaitingSemaphore.release();
}

void
TemplateThread::
run()
{
    while( !mbAbort )
    {
        mWaitingSemaphore.acquire();
        if( !mbThreadReady )
            continue;
        if( miThreadStatus == 2 )
        {
            miThreadStatus = 0;
            mbThreadReady = false;
            if( mWaitingSemaphore.available() != 0 )
                mWaitingSemaphore.acquire( mWaitingSemaphore.available() );
            continue;
        }
        if( 0 ) // If error occurs, send a signal to the calling thread.
        {
            Q_EMIT error_signal( 1 );
        }
    }
}

TemplateThreadModel::
TemplateThreadModel()
    : PBNodeDataModel( _model_name )
{

}

unsigned int
TemplateThreadModel::
nPorts(PortType portType) const
{
    unsigned int result = 0;

    switch (portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 0;
        break;

    default:
        break;
    }

    return result;
}

NodeDataType
TemplateThreadModel::
dataType(PortType, PortIndex portIndex) const
{
    if(portIndex == 0)
    {
        return NodeDataType();
    }
    return NodeDataType();
}

void
TemplateThreadModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;
    if( nodeData )
    {
        if( nodeData )
            processData( nodeData );
    }
}


QJsonObject
TemplateThreadModel::
save() const
{
    QJsonObject modelJson = PBNodeDataModel::save();
    QJsonObject cParams;

    modelJson["cParams"] = cParams;
    return modelJson;
}


void
TemplateThreadModel::
restore( QJsonObject const &p )
{
    PBNodeDataModel::restore( p );
    late_constructor();

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {

    }
}


void
TemplateThreadModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDataModel::setModelProperty( id, value );
    if( !mMapIdToProperty.contains( id ) )
        return;
}


void
TemplateThreadModel::
late_constructor()
{
    if( !mpTemplateThread )
    {
        mpTemplateThread = new TemplateThread(this);
        connect( mpTemplateThread, &TemplateThread::error_signal, this, &TemplateThreadModel::thread_error_occured );
    }
}


void
TemplateThreadModel::
processData(const std::shared_ptr<NodeData> & )
{

}


void
TemplateThreadModel::
    thread_error_occured( int )
{

}

const QString TemplateThreadModel::_category = QString("Template Category");

const QString TemplateThreadModel::_model_name = QString( "Template Thread Model" );
