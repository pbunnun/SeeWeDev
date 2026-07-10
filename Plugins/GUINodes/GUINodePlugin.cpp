//Copyright © 2021 - 2026, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "GUINodePlugin.hpp"
#include "PushButtonModel.hpp"
#include "LCDNumberModel.hpp"
#include "ActivateAllNodesModel.hpp"
#include "DisplayTextModel.hpp"
#include "RGBLedModel.hpp"
#include "ToggleButtonModel.hpp"
#include "IntSpinButtonModel.hpp"
#include "FloatSpinButtonModel.hpp"
#include "DoubleSpinButtonModel.hpp"
#include "TableViewModel.hpp"

QStringList GUINodePlugin::registerDataModel( std::shared_ptr< NodeDelegateModelRegistry > model_regs )
{
    QStringList duplicate_model_names;

    registerModel< PushButtonModel >( model_regs, duplicate_model_names );
    registerModel< LCDNumberModel >( model_regs, duplicate_model_names );
    registerModel< ActivateAllNodesModel >( model_regs, duplicate_model_names );
    registerModel< DisplayTextModel >( model_regs, duplicate_model_names );
    registerModel< RGBLedModel >( model_regs, duplicate_model_names );
    registerModel< ToggleButtonModel >( model_regs, duplicate_model_names );
    registerModel< IntSpinButtonModel >( model_regs, duplicate_model_names );
    registerModel< FloatSpinButtonModel >( model_regs, duplicate_model_names );
    registerModel< DoubleSpinButtonModel >( model_regs, duplicate_model_names );
    registerModel< TableViewModel >( model_regs, duplicate_model_names );

    return duplicate_model_names;
}
