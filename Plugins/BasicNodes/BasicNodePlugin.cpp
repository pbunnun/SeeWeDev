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

#include "BasicNodePlugin.hpp"
#include "CannyEdgeModel.hpp"
#include "RGBsetValueModel.hpp"
#include "CVImageDisplayModel.hpp"
#include "InformationDisplayModel.hpp"
#include "CVImageLoaderModel.hpp"
#include "CVVDOLoaderModel.hpp"
#include "RGBtoGrayModel.hpp"
#include "ColorSpaceModel.hpp"
#include "Test_SharpenModel.hpp"
#include "CVCameraModel.hpp"
#include "GaussianBlurModel.hpp"
#include "TemplateModel.hpp"
#include "SobelAndScharrModel.hpp"
#include "CreateHistogramModel.hpp"
#include "ErodeAndDilateModel.hpp"
#include "InvertGrayModel.hpp"
#include "ThresholdingModel.hpp"
#include "BlendImagesModel.hpp"
#include "FloodFillModel.hpp"
#include "MakeBorderModel.hpp"
#include "BitwiseOperationModel.hpp"
#include "ImageROIModel.hpp"
#include "CVImageROIModel.hpp"
#include "CVImagePropertiesModel.hpp"
#include "MorphologicalTransformationModel.hpp"
#include "HoughCircleTransfromModel.hpp"
#include "DistanceTransformModel.hpp"
#include "Filter2DModel.hpp"
#include "SplitImageModel.hpp"
#include "TemplateMatchingModel.hpp"
#include "MatrixOperationModel.hpp"
#include "MinMaxLocationModel.hpp"
#include "ConnectedComponentsModel.hpp"
#include "ConvertDepthModel.hpp"
#include "PixelIterationModel.hpp"
#include "ScalarOperationModel.hpp"
#include "DataGeneratorModel.hpp"
#include "SyncGateModel.hpp"
#include "NormalizationModel.hpp"
#include "WatershedModel.hpp"
#include "ColorMapModel.hpp"
#include "TimerModel.hpp"
#include "NodeDataTimerModel.hpp"
#include "VideoWriterModel.hpp"
#include "CVRotateImageModel.hpp"
#include "CVImageResizeModel.hpp"
#include "InfoConcatenateModel.hpp"
#include "SaveImageModel.hpp"
#include "CVImageInRangeModel.hpp"
#include "ExternalCommandModel.hpp"
#include "NotSyncDataModel.hpp"
#include "MathIntegerSumModel.hpp"
#include "FindContourModel.hpp"
#include "DrawContourModel.hpp"
#include "FindAndDrawContourModel.hpp"
#include "CVMatSumModel.hpp"
//#include "CVCameraCalibrationModel.hpp"
#include "MathConditionModel.hpp"
#include "MathConvertToIntModel.hpp"
#include "CombineSyncModel.hpp"

//#include "FaceDetectionModel.hpp"

QStringList BasicNodePlugin::registerDataModel( std::shared_ptr< DataModelRegistry > model_regs )
{
    QStringList duplicate_model_names;

    registerModel< CVImageDisplayModel >( model_regs, duplicate_model_names );
    registerModel< CVImagePropertiesModel >( model_regs, duplicate_model_names );
    registerModel< InformationDisplayModel > ( model_regs, duplicate_model_names );
    registerModel< NodeDataTimerModel > ( model_regs, duplicate_model_names );

    registerModel< CVCameraModel >( model_regs, duplicate_model_names );
    registerModel< CVImageLoaderModel >( model_regs, duplicate_model_names );
    registerModel< CVVDOLoaderModel >( model_regs, duplicate_model_names );

    registerModel< BitwiseOperationModel >( model_regs, duplicate_model_names );
    registerModel< BlendImagesModel >( model_regs, duplicate_model_names );
    registerModel< CannyEdgeModel >( model_regs, duplicate_model_names );
    registerModel< ColorMapModel >( model_regs, duplicate_model_names );
    registerModel< ColorSpaceModel >( model_regs, duplicate_model_names );
    registerModel< ConnectedComponentsModel >( model_regs, duplicate_model_names );
    registerModel< ConvertDepthModel >( model_regs, duplicate_model_names );
    registerModel< CreateHistogramModel >( model_regs, duplicate_model_names );
    registerModel< DataGeneratorModel >( model_regs, duplicate_model_names );
    registerModel< DistanceTransformModel >( model_regs, duplicate_model_names );
    registerModel< ErodeAndDilateModel >( model_regs, duplicate_model_names );
//    registerModel< FaceDetectionModel >( model_regs, duplicate_model_names );
    registerModel< Filter2DModel >( model_regs, duplicate_model_names );
    registerModel< FloodFillModel >( model_regs, duplicate_model_names );
    registerModel< GaussianBlurModel >( model_regs, duplicate_model_names );
    registerModel< HoughCircleTransformModel >( model_regs, duplicate_model_names );
    registerModel< ImageROIModel >( model_regs, duplicate_model_names );
    registerModel< CVImageROIModel > ( model_regs, duplicate_model_names );
    registerModel< CVImageResizeModel > ( model_regs, duplicate_model_names );
    registerModel< InvertGrayModel >( model_regs, duplicate_model_names );
    registerModel< MakeBorderModel >( model_regs, duplicate_model_names );
    registerModel< MatrixOperationModel >( model_regs, duplicate_model_names );
    registerModel< MinMaxLocationModel >( model_regs, duplicate_model_names );
    registerModel< MorphologicalTransformationModel >( model_regs, duplicate_model_names );
    registerModel< NormalizationModel >(model_regs, duplicate_model_names);
    registerModel< PixelIterationModel >(model_regs, duplicate_model_names);
    registerModel< RGBsetValueModel >(model_regs, duplicate_model_names);
    registerModel< RGBtoGrayModel >( model_regs, duplicate_model_names );
    registerModel< ScalarOperationModel >( model_regs, duplicate_model_names );
    registerModel< SobelAndScharrModel >( model_regs, duplicate_model_names );
    registerModel< SplitImageModel >( model_regs, duplicate_model_names );
    registerModel< SyncGateModel >( model_regs, duplicate_model_names );
    registerModel< TemplateMatchingModel >( model_regs, duplicate_model_names );
    registerModel< ThresholdingModel >( model_regs, duplicate_model_names );
    registerModel< WatershedModel >( model_regs, duplicate_model_names );
    registerModel< TimerModel >( model_regs, duplicate_model_names );
    registerModel< VideoWriterModel >( model_regs, duplicate_model_names );
    registerModel< CVRotateImageModel >( model_regs, duplicate_model_names );
    registerModel< InfoConcatenateModel >( model_regs, duplicate_model_names );
    registerModel< SaveImageModel >( model_regs, duplicate_model_names );
    registerModel< CVImageInRangeModel >( model_regs, duplicate_model_names );
    registerModel< TemplateModel >( model_regs, duplicate_model_names );
    registerModel< Test_SharpenModel >( model_regs, duplicate_model_names );

    registerModel< ExternalCommandModel >( model_regs, duplicate_model_names );
    registerModel< NotSyncDataModel >( model_regs, duplicate_model_names );
    registerModel< MathIntegerSumModel >( model_regs, duplicate_model_names );
    registerModel< DrawContourModel >( model_regs, duplicate_model_names );
    registerModel< FindContourModel >( model_regs, duplicate_model_names );
    registerModel< FindAndDrawContourModel >( model_regs, duplicate_model_names );
    registerModel< CVMatSumModel >( model_regs, duplicate_model_names );
    registerModel< MathConditionModel >( model_regs, duplicate_model_names );
    registerModel< MathConvertToIntModel >( model_regs, duplicate_model_names );
    registerModel< CombineSyncModel >( model_regs, duplicate_model_names );

 //   registerModel< CVCameraCalibrationModel >( model_regs, duplicate_model_names );

    return duplicate_model_names;
}
