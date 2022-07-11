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

#ifndef BASICNODEPLUGIN_HPP
#define BASICNODEPLUGIN_HPP

#include <PluginInterface.hpp>

#include <QObject>

class BasicNodePlugin : public QObject,
                        public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "CVDev.PluginInterface" FILE "basicnodes.json" )
    Q_INTERFACES( PluginInterface )

public:
    QStringList registerDataModel( std::shared_ptr< DataModelRegistry > model_regs );
};

#endif
