#include "DNNNodePlugin.hpp"
#include "FaceDetectionDNNModel.hpp"
#include "OnnxClassificationDNNModel.hpp"
#include "TextDetectionDNNModel.hpp"
#include "TextRecognitionDNNModel.hpp"
#include "CVYoloDNNModel.hpp"

QStringList DNNNodePlugin::registerDataModel( std::shared_ptr< DataModelRegistry > model_regs )
{
    QStringList duplicate_model_names;

    registerModel< FaceDetectionDNNModel >( model_regs, duplicate_model_names );
    registerModel< OnnxClassificationDNNModel >( model_regs, duplicate_model_names );
    registerModel< TextDetectionDNNModel >( model_regs, duplicate_model_names );
    registerModel< TextRecognitionDNNModel >( model_regs, duplicate_model_names );
    registerModel< CVYoloDNNModel >( model_regs, duplicate_model_names );

    return duplicate_model_names;
}
