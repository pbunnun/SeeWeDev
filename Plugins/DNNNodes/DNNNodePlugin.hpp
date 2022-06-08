#ifndef DNNNODEPLUGIN_HPP
#define DNNNODEPLUGIN_HPP

#include <PluginInterface.hpp>

#include <QObject>

class DNNNodePlugin : public QObject,
                        public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "CVDevTool.PluginInterface" FILE "basicnodes.json" )
    Q_INTERFACES( PluginInterface )

public:
    QStringList registerDataModel( std::shared_ptr< DataModelRegistry > model_regs );
};

#endif
