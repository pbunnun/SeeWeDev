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

#include "PluginInterface.hpp"
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QPluginLoader>
#include <QMessageBox>

void load_plugins( std::shared_ptr< DataModelRegistry > model_regs )
{
    QDir pluginsDir = QDir(QCoreApplication::applicationDirPath());
    pluginsDir.cd( "plugins" );
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    const auto entryList = pluginsDir.entryList( QStringList() << "*.dll", QDir::Files);
#elif defined( __APPLE__ )
    const auto entryList = pluginsDir.entryList( QStringList() << "*.dylib", QDir::Files);
#elif defined( __linux__ )
    const auto entryList = pluginsDir.entryList( QStringList() << "*.so", QDir::Files);
#endif
    for(const QString & filename : entryList )
    {
        QPluginLoader loader(pluginsDir.absoluteFilePath(filename));
        qDebug() << "Found Files : " << filename;
        QObject *plugin = loader.instance();
        if( plugin )
        {
            qDebug() << "Load Plugins : " << filename;
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
        }
        else
        {
            qDebug() << loader.errorString();
        }
    }

}

void load_plugin( std::shared_ptr< DataModelRegistry > model_regs, QString filename )
{
   QPluginLoader loader(filename);
   QObject *plugin = loader.instance();
   if( plugin )
   {
       qDebug() << "Load Plugins : " << filename;
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
   }
   else
   {
       qDebug() << loader.errorString();
   }
}
