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

#include "DNNNodePlugin.hpp"
#include "FaceDetectionDNNModel.hpp"
#include "OnnxClassificationDNNModel.hpp"
#include "TextDetectionDNNModel.hpp"
#include "TextRecognitionDNNModel.hpp"
#include "CVYoloDNNModel.hpp"
#include "NecMLClassificationModel.hpp"
#include "NomadMLClassificationModel.hpp"

QStringList DNNNodePlugin::registerDataModel( std::shared_ptr< DataModelRegistry > model_regs )
{
    QStringList duplicate_model_names;

    registerModel< FaceDetectionDNNModel >( model_regs, duplicate_model_names );
    registerModel< OnnxClassificationDNNModel >( model_regs, duplicate_model_names );
    registerModel< TextDetectionDNNModel >( model_regs, duplicate_model_names );
    registerModel< TextRecognitionDNNModel >( model_regs, duplicate_model_names );
    registerModel< CVYoloDNNModel >( model_regs, duplicate_model_names );
    registerModel< NecMLClassificationModel >( model_regs, duplicate_model_names );
    registerModel< NomadMLClassificationModel >( model_regs, duplicate_model_names );

    return duplicate_model_names;
}
