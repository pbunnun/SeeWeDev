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

#include "BasicNodePlugin.hpp"
#include "CVCannyEdgeModel.hpp"
#include "CVRGBsetValueModel.hpp"
#include "CVImageDisplayModel.hpp"
#include "InformationDisplayModel.hpp"
#include "CVImageLoaderModel.hpp"
#include "CVVideoLoaderModel.hpp"
#include "CVRGBtoGrayModel.hpp"
#include "CVColorSpaceModel.hpp"
#include "Test_SharpenModel.hpp"
#include "CVCameraModel.hpp"
#include "CVGaussianBlurModel.hpp"
#include "TemplateModel.hpp"
#include "CVSobelAndScharrModel.hpp"
#include "CVCreateHistogramModel.hpp"
#include "CVErodeAndDilateModel.hpp"
#include "CVInvertGrayModel.hpp"
#include "CVThresholdingModel.hpp"
#include "CVBlendImagesModel.hpp"
#include "CVFloodFillModel.hpp"
#include "CVMakeBorderModel.hpp"
#include "CVBitwiseOperationModel.hpp"
#include "CVAdditionModel.hpp"
#include "CVOverlayImageModel.hpp"
#include "CVImageROIModel.hpp"
#include "CVImageROINewModel.hpp"
#include "CVImagePropertiesModel.hpp"
#include "CVMorphologicalTransformationModel.hpp"
#include "CVHoughCircleTransfromModel.hpp"
#include "CVDistanceTransformModel.hpp"
#include "CVFilter2DModel.hpp"
#include "CVSplitImageModel.hpp"
#include "CVTemplateMatchingModel.hpp"
#include "CVMatrixOperationModel.hpp"
#include "CVMinMaxLocationModel.hpp"
#include "CVConnectedComponentsModel.hpp"
#include "CVConvertDepthModel.hpp"
#include "CVPixelIterationModel.hpp"
#include "ScalarOperationModel.hpp"
#include "DataGeneratorModel.hpp"
#include "SyncGateModel.hpp"
#include "CVNormalizationModel.hpp"
#include "CVWatershedModel.hpp"
#include "CVColorMapModel.hpp"
#include "TimerModel.hpp"
#include "NodeDataTimerModel.hpp"
#include "CVVideoWriterModel.hpp"
#include "CVRotateImageModel.hpp"
#include "CVImageResizeModel.hpp"
#include "InfoConcatenateModel.hpp"
#include "CVSaveImageModel.hpp"
#include "CVImageInRangeModel.hpp"
#include "ExternalCommandModel.hpp"
#include "NotSyncDataModel.hpp"
#include "MathIntegerSumModel.hpp"
#include "CVFindContourModel.hpp"
#include "CVDrawContourModel.hpp"
#include "CVFindAndDrawContourModel.hpp"
#include "CVMatSumModel.hpp"
//#include "CVCameraCalibrationModel.hpp"
#include "MathConditionModel.hpp"
#include "MathConvertToIntModel.hpp"
#include "CombineSyncModel.hpp"

//#include "FaceDetectionModel.hpp"

QStringList BasicNodePlugin::registerDataModel( std::shared_ptr< NodeDelegateModelRegistry > model_regs )
{
    QStringList duplicate_model_names;

    registerModel< CVImageDisplayModel >( model_regs, duplicate_model_names );
    registerModel< CVImagePropertiesModel >( model_regs, duplicate_model_names );
    registerModel< InformationDisplayModel > ( model_regs, duplicate_model_names );
    registerModel< NodeDataTimerModel > ( model_regs, duplicate_model_names );

    registerModel< CVCameraModel >( model_regs, duplicate_model_names );
    registerModel< CVImageLoaderModel >( model_regs, duplicate_model_names );
    registerModel< CVVideoLoaderModel >( model_regs, duplicate_model_names );

    registerModel< CVBitwiseOperationModel >( model_regs, duplicate_model_names );
    registerModel< CVAdditionModel >( model_regs, duplicate_model_names );
    registerModel< CVOverlayImageModel >( model_regs, duplicate_model_names );
    registerModel< CVBlendImagesModel >( model_regs, duplicate_model_names );
    registerModel< CVCannyEdgeModel >( model_regs, duplicate_model_names );
    registerModel< CVColorMapModel >( model_regs, duplicate_model_names );
    registerModel< CVColorSpaceModel >( model_regs, duplicate_model_names );
    registerModel< CVConnectedComponentsModel >( model_regs, duplicate_model_names );
    registerModel< CVConvertDepthModel >( model_regs, duplicate_model_names );
    registerModel< CVCreateHistogramModel >( model_regs, duplicate_model_names );
    registerModel< DataGeneratorModel >( model_regs, duplicate_model_names );
    registerModel< CVDistanceTransformModel >( model_regs, duplicate_model_names );
    registerModel< CVErodeAndDilateModel >( model_regs, duplicate_model_names );
//    registerModel< CVFaceDetectionModel >( model_regs, duplicate_model_names );
    registerModel< CVFilter2DModel >( model_regs, duplicate_model_names );
    registerModel< CVFloodFillModel >( model_regs, duplicate_model_names );
    registerModel< CVGaussianBlurModel >( model_regs, duplicate_model_names );
    registerModel< CVHoughCircleTransformModel >( model_regs, duplicate_model_names );
    registerModel< CVImageROIModel >( model_regs, duplicate_model_names );
    registerModel< CVImageROINewModel > ( model_regs, duplicate_model_names );
    registerModel< CVImageResizeModel > ( model_regs, duplicate_model_names );
    registerModel< CVInvertGrayModel >( model_regs, duplicate_model_names );
    registerModel< CVMakeBorderModel >( model_regs, duplicate_model_names );
    registerModel< CVMatrixOperationModel >( model_regs, duplicate_model_names );
    registerModel< CVMinMaxLocationModel >( model_regs, duplicate_model_names );
    registerModel< CVMorphologicalTransformationModel >( model_regs, duplicate_model_names );
    registerModel< CVNormalizationModel >(model_regs, duplicate_model_names);
    registerModel< CVPixelIterationModel >(model_regs, duplicate_model_names);
    registerModel< CVRGBsetValueModel >(model_regs, duplicate_model_names);
    registerModel< CVRGBtoGrayModel >( model_regs, duplicate_model_names );
    registerModel< ScalarOperationModel >( model_regs, duplicate_model_names );
    registerModel< CVSobelAndScharrModel >( model_regs, duplicate_model_names );
    registerModel< CVSplitImageModel >( model_regs, duplicate_model_names );
    registerModel< SyncGateModel >( model_regs, duplicate_model_names );
    registerModel< CVTemplateMatchingModel >( model_regs, duplicate_model_names );
    registerModel< CVThresholdingModel >( model_regs, duplicate_model_names );
    registerModel< CVWatershedModel >( model_regs, duplicate_model_names );
    registerModel< TimerModel >( model_regs, duplicate_model_names );
    registerModel< CVVideoWriterModel >( model_regs, duplicate_model_names );
    registerModel< CVRotateImageModel >( model_regs, duplicate_model_names );
    registerModel< InfoConcatenateModel >( model_regs, duplicate_model_names );
    registerModel< CVSaveImageModel >( model_regs, duplicate_model_names );
    registerModel< CVImageInRangeModel >( model_regs, duplicate_model_names );
    registerModel< TemplateModel >( model_regs, duplicate_model_names );
    registerModel< Test_SharpenModel >( model_regs, duplicate_model_names );

    registerModel< ExternalCommandModel >( model_regs, duplicate_model_names );
    registerModel< NotSyncDataModel >( model_regs, duplicate_model_names );
    registerModel< MathIntegerSumModel >( model_regs, duplicate_model_names );
    registerModel< CVDrawContourModel >( model_regs, duplicate_model_names );
    registerModel< CVFindContourModel >( model_regs, duplicate_model_names );
    registerModel< CVFindAndDrawContourModel >( model_regs, duplicate_model_names );
    registerModel< CVMatSumModel >( model_regs, duplicate_model_names );
    registerModel< MathConditionModel >( model_regs, duplicate_model_names );
    registerModel< MathConvertToIntModel >( model_regs, duplicate_model_names );
    registerModel< CombineSyncModel >( model_regs, duplicate_model_names );

 //   registerModel< CVCameraCalibrationModel >( model_regs, duplicate_model_names );

    return duplicate_model_names;
}
