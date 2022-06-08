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
