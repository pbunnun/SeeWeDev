#include "GUINodePlugin.hpp"
#include "PushButtonModel.hpp"
#include "LCDNumberModel.hpp"
#include "ActivateAllNodesModel.hpp"
#include "DisplayTextModel.hpp"

QStringList GUINodePlugin::registerDataModel( std::shared_ptr< NodeDelegateModelRegistry > model_regs )
{
    QStringList duplicate_model_names;

    registerModel< PushButtonModel >( model_regs, duplicate_model_names );
    registerModel< LCDNumberModel >( model_regs, duplicate_model_names );
    registerModel< ActivateAllNodesModel >( model_regs, duplicate_model_names );
    registerModel< DisplayTextModel >( model_regs, duplicate_model_names );

    return duplicate_model_names;
}
