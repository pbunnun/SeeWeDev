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

#include "PBFlowScene.hpp"
#include "PBNodeDataModel.hpp"
#include <QFile>
#include <QFileInfo>
#include <nodes/Node>

using QtNodes::PortType;

PBFlowScene::
PBFlowScene(QWidget *parent)
    : QtNodes::FlowScene(parent)
{

}

bool
PBFlowScene::
save(QString & sFilename) const
{
    if( !sFilename.isEmpty() )
    {
        QFile file(sFilename);
        if( file.open(QIODevice::WriteOnly) )
        {
            file.write(saveToMemory());
            return true;
        }
        else
            return false;
    }
    return false;
}

bool
PBFlowScene::
load(QString & sFilename)
{
    if( !QFileInfo::exists(sFilename) )
        return false;

    QFile file(sFilename);
    if( !file.open(QIODevice::ReadOnly) )
        return false;

    clearScene();

    QByteArray wholeFile = file.readAll();
    if( !loadFromMemory(wholeFile) )
        return false;

    auto nodes = allNodes();
    for( auto node : nodes )
    {
        auto dataModel = static_cast< PBNodeDataModel *>( node->nodeDataModel() );
        node->nodeGraphicsObject().lock_position(dataModel->isLockPosition());
    }

    return true;
}
