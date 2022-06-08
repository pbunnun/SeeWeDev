include($$PWD/../OpenCV.pri)
include($$PWD/../Plugins.pri)

greaterThan( QT_MAJOR_VERSION, 5 ) {
    QT += openglwidgets
}
equals( QT_MAJOR_VERSION, 5 ) {
    QT += opengl
}

HEADERS		+= \
            BasicNodePlugin.hpp \
            BitwiseOperationEmbeddedWidget.hpp \
            BitwiseOperationModel.hpp \
            BlendImagesEmbeddedWidget.hpp \
            BlendImagesModel.hpp \
            ColorMapModel.hpp \
            ConnectedComponentsModel.hpp \
            ConvertDepthModel.hpp \
            DataGeneratorEmbeddedWidget.hpp \
            DataGeneratorModel.hpp \
            FaceDetectionEmbeddedWidget.hpp \
            CVCameraEmbeddedWidget.hpp \
            CVCameraModel.hpp \
            PBImageDisplayWidget.hpp \
            CVImageDisplayModel.hpp \
            CVImageLoaderModel.hpp \
            CVImagePropertiesModel.hpp \
            CVVDOLoaderModel.hpp \
            CannyEdgeModel.hpp \
            ColorSpaceModel.hpp \
            CreateHistogramModel.hpp \
            DistanceTransformModel.hpp \
            DrawContourModel.hpp \
            ErodeAndDilateEmbeddedWidget.hpp \
            ErodeAndDilateModel.hpp \
            FaceDetectionModel.hpp \
            Filter2DModel.hpp \
            FloodFillEmbeddedWidget.hpp \
            FloodFillModel.hpp \
            GaussianBlurModel.hpp \
            HoughCircleTransfromModel.hpp \
            ImageROIEmbeddedWidget.hpp \
            ImageROIModel.hpp \
            InformationDisplayModel.hpp \
            InvertGrayModel.hpp \
            MakeBorderModel.hpp \
            MatrixOperationModel.hpp \
            MinMaxLocationModel.hpp \
            MorphologicalTransformationModel.hpp \
            NodeDataTimerEmbeddedWidget.hpp \
            NodeDataTimerModel.hpp \
            NormalizationModel.hpp \
            PixelIterationModel.hpp \
            RGBsetValueEmbeddedWidget.hpp \
            RGBsetValueModel.hpp \
            RGBtoGrayModel.hpp \
            ScalarOperationModel.hpp \
            SobelAndScharrEmbeddedWidget.hpp \
            SobelAndScharrModel.hpp \
            SplitImageModel.hpp \
            SyncGateEmbeddedWidget.hpp \
            SyncGateModel.hpp \
            TemplateMatchingModel.hpp \
            TemplateModel.hpp \
            TemplateEmbeddedWidget.hpp \
            Test_SharpenModel.hpp \
            ThresholdingModel.hpp \
            TimerModel.hpp \
            WatershedModel.hpp \
            VideoWriterModel.hpp
SOURCES		+= \
            BasicNodePlugin.cpp \
            BitwiseOperationEmbeddedWidget.cpp \
            BitwiseOperationModel.cpp \
            BlendImagesEmbeddedWidget.cpp \
            BlendImagesModel.cpp \
            ColorMapModel.cpp \
            ConnectedComponentsModel.cpp \
            ConvertDepthModel.cpp \
            DataGeneratorEmbeddedWidget.cpp \
            DataGeneratorModel.cpp \
            FaceDetectionEmbeddedWidget.cpp \
            CVCameraEmbeddedWidget.cpp \
            CVCameraModel.cpp \
            PBImageDisplayWidget.cpp \
            CVImageDisplayModel.cpp \
            CVImageLoaderModel.cpp \
            CVImagePropertiesModel.cpp \
            CVVDOLoaderModel.cpp \
            CannyEdgeModel.cpp \
            ColorSpaceModel.cpp \
            CreateHistogramModel.cpp \
            DistanceTransformModel.cpp \
            DrawContourModel.cpp \
            ErodeAndDilateEmbeddedWidget.cpp \
            ErodeAndDilateModel.cpp \
            FaceDetectionModel.cpp \
            Filter2DModel.cpp \
            FloodFillEmbeddedWidget.cpp \
            FloodFillModel.cpp \
            GaussianBlurModel.cpp \
            HoughCircleTransfromModel.cpp \
            ImageROIEmbeddedWidget.cpp \
            ImageROIModel.cpp \
            InformationDisplayModel.cpp \
            InvertGrayModel.cpp \
            MakeBorderModel.cpp \
            MatrixOperationModel.cpp \
            MinMaxLocationModel.cpp \
            MorphologicalTransformationModel.cpp \
            NodeDataTimerEmbeddedWidget.cpp \
            NodeDataTimerModel.cpp \
            NormalizationModel.cpp \
            PixelIterationModel.cpp \
            RGBsetValueEmbeddedWidget.cpp \
            RGBsetValueModel.cpp \
            RGBtoGrayModel.cpp \
            ScalarOperationModel.cpp \
            SobelAndScharrEmbeddedWidget.cpp \
            SobelAndScharrModel.cpp \
            SplitImageModel.cpp \
            SyncGateEmbeddedWidget.cpp \
            SyncGateModel.cpp \
            TemplateMatchingModel.cpp \
            TemplateModel.cpp \
            TemplateEmbeddedWidget.cpp \
            Test_SharpenModel.cpp \
            ThresholdingModel.cpp \
            TimerModel.cpp \
            WatershedModel.cpp \
            VideoWriterModel.cpp
TARGET		= $$qtLibraryTarget(plugin_Basics)

RESOURCES += \
    resources/basic_resources.qrc

FORMS += \
    BitwiseOperationEmbeddedWidget.ui \
    BlendImagesEmbeddedWidget.ui \
    CVCameraEmbeddedWidget.ui \
    DataGeneratorEmbeddedWidget.ui \
    ErodeAndDilateEmbeddedWidget.ui \
    FaceDetectionEmbeddedWidget.ui \
    FloodFillEmbeddedWidget.ui \
    ImageROIEmbeddedWidget.ui \
    NodeDataTimerEmbeddedWidget.ui \
    RGBsetValueEmbeddedWidget.ui \
    SobelAndScharrEmbeddedWidget.ui \
    SyncGateEmbeddedWidget.ui \
    TemplateEmbeddedWidget.ui

unix{
        LIBS += -lopencv_face -lopencv_objdetect
    }
