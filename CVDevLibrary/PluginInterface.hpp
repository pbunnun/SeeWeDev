#ifndef PLUGININTERFACE_HPP
#define PLUGININTERFACE_HPP

#pragma once

#include <QtPlugin>
#include <DataModelRegistry>
#include "CVDevLibrary.hpp"

using QtNodes::DataModelRegistry;

void CVDEVSHAREDLIB_EXPORT load_plugins( std::shared_ptr< DataModelRegistry > model_regs );
void CVDEVSHAREDLIB_EXPORT load_plugin( std::shared_ptr< DataModelRegistry > model_regs, QString filename );

class PluginInterface
{
public:
    virtual ~PluginInterface() = default;

    virtual
    QStringList registerDataModel( std::shared_ptr< DataModelRegistry > model_regs ) = 0;

    template <typename ModelType>
    void
    registerModel(std::shared_ptr< DataModelRegistry > model_regs, QStringList & duplicate_model_names )
    {
        if( model_regs->registeredModelCreators().count( ModelType::_model_name ) )
            duplicate_model_names.append( ModelType::_model_name );
        else
            model_regs->registerModel< ModelType >( ModelType::_category );
    }
};

QT_BEGIN_NAMESPACE

#define PluginInterface_iid "CVDevTool.PluginInterface/1.0"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

QT_END_NAMESPACE

#endif
