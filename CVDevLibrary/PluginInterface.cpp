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

#include "PluginInterface.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <QStandardPaths>

#include "BoolData.hpp"
#include "CVImageData.hpp"
#include "CVPointData.hpp"
#include "CVRectData.hpp"
#include "CVSizeData.hpp"
#include "DoubleData.hpp"
#include "FloatData.hpp"
#include "IntegerData.hpp"
#include "StdStringData.hpp"
#include "StdVectorNumberData.hpp"
#include "SyncData.hpp"

#include "InformationData.hpp"

std::shared_ptr<QtNodes::NodeData> converter(std::shared_ptr<QtNodes::NodeData> node_type)
{
    return node_type;
}
/*
// TODO NodeEditor v3 Migration: Updated to use NodeDelegateModelRegistry
void add_type_converters( std::shared_ptr< NodeDelegateModelRegistry > model_regs )
{
    auto inf_nodedata = std::make_shared<InformationData>();

    auto bool_nodedata = std::make_shared<BoolData>();
    model_regs->registerTypeConverter( std::make_pair( bool_nodedata->type(), inf_nodedata->type() ), converter);

    auto mat_nodedata = std::make_shared<CVImageData>();
    model_regs->registerTypeConverter( std::make_pair( mat_nodedata->type(), inf_nodedata->type() ), converter);

    auto point_nodedata = std::make_shared<CVPointData>();
    model_regs->registerTypeConverter( std::make_pair( point_nodedata->type(), inf_nodedata->type() ), converter);

    auto rect_nodedata = std::make_shared<CVRectData>();
    model_regs->registerTypeConverter( std::make_pair( rect_nodedata->type(), inf_nodedata->type() ), converter);

    auto size_nodedata = std::make_shared<CVSizeData>();
    model_regs->registerTypeConverter( std::make_pair( size_nodedata->type(), inf_nodedata->type() ), converter);

    auto double_nodedata = std::make_shared<DoubleData>();
    model_regs->registerTypeConverter( std::make_pair( double_nodedata->type(), inf_nodedata->type() ), converter);

    auto float_nodedata = std::make_shared<FloatData>();
    model_regs->registerTypeConverter( std::make_pair( float_nodedata->type(), inf_nodedata->type() ), converter);

    auto int_nodedata = std::make_shared<IntegerData>();
    model_regs->registerTypeConverter( std::make_pair( int_nodedata->type(), inf_nodedata->type() ), converter);

    auto string_nodedata = std::make_shared<StdStringData>();
    model_regs->registerTypeConverter( std::make_pair( string_nodedata->type(), inf_nodedata->type() ), converter);

    auto std_vec_nodedata = std::make_shared<StdVectorIntData>();
    model_regs->registerTypeConverter( std::make_pair( std_vec_nodedata->type(), inf_nodedata->type() ), converter);

    auto sync_nodedata = std::make_shared<SyncData>();
    model_regs->registerTypeConverter( std::make_pair( sync_nodedata->type(), inf_nodedata->type() ), converter);

}
*/
// TODO NodeEditor v3 Migration: Updated to use NodeDelegateModelRegistry
void load_plugins_from_dir(std::shared_ptr< NodeDelegateModelRegistry > model_regs, QList< QPluginLoader * > & plugins_list, QDir pluginsDir )
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    const auto entryList = pluginsDir.entryList( QStringList() << "*.dll", QDir::Files);
#elif defined( __APPLE__ )
    const auto entryList = pluginsDir.entryList( QStringList() << "*.dylib", QDir::Files);
#elif defined( __linux__ )
    const auto entryList = pluginsDir.entryList( QStringList() << "*.so", QDir::Files);
#endif
    for(const QString & filename : entryList )
    {
        QPluginLoader * loader = new QPluginLoader(pluginsDir.absoluteFilePath(filename));
        qInfo() << "Found Files : " << filename;
        QObject *plugin = loader->instance();
        if( plugin )
        {
            qInfo() << "Load Plugins : " << filename;
            auto plugin_interface = qobject_cast<PluginInterface *>( plugin );
            if( plugin_interface )
            {
                QStringList duplicate_model_names = plugin_interface->registerDataModel( model_regs );
                if( !duplicate_model_names.isEmpty() )
                {
                    QMessageBox msg;
                    QString msgText = "Please check " + filename + "\n Duplicate Model Names :";
                    for( const QString & model_name : duplicate_model_names )
                        msgText += " " + model_name;
                    msgText += ".";
                    msg.setIcon( QMessageBox::Warning );
                    msg.setText( msgText );
                    msg.exec();
                }
            }
            plugins_list.push_back( loader );
        }
        else
        {
            qCritical() << loader->errorString();
        }
    }
}

// TODO NodeEditor v3 Migration: Updated to use NodeDelegateModelRegistry
void load_plugins( std::shared_ptr< NodeDelegateModelRegistry > model_regs, QList< QPluginLoader * > & plugins_list )
{
    QDir pluginsDir = QDir(QCoreApplication::applicationDirPath());
    pluginsDir.cd( "cvdev_plugins" );
    load_plugins_from_dir( model_regs, plugins_list, pluginsDir );

    pluginsDir = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+"/.CVDevPro/cvdev_plugins");
    if( !pluginsDir.exists() )
        pluginsDir.mkpath(".");
    load_plugins_from_dir( model_regs, plugins_list, pluginsDir );
}

// TODO NodeEditor v3 Migration: Updated to use NodeDelegateModelRegistry
void load_plugin(std::shared_ptr< NodeDelegateModelRegistry > model_regs, QList< QPluginLoader *> & plugins_list , QString filename)
{
   QPluginLoader * loader = new QPluginLoader(filename);
   QObject *plugin = loader->instance();
   if( plugin )
   {
       qInfo() << "Load Plugins : " << filename;
       auto plugin_interface = qobject_cast<PluginInterface *>( plugin );
       if( plugin_interface )
       {
           QStringList duplicate_model_names = plugin_interface->registerDataModel( model_regs );
           if( !duplicate_model_names.isEmpty() )
           {
               QMessageBox msg;
               QString msgText = "Please check " + filename + "\n Duplicate Model Names :";
               for( const QString & model_name : duplicate_model_names )
                   msgText += " " + model_name;
               msgText += ".";
               msg.setIcon( QMessageBox::Warning );
               msg.setText( msgText );
               msg.exec();
           }
       }
       plugins_list.push_back( loader );
   }
   else
   {
       qCritical() << loader->errorString();
   }
}
