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

#include "CVFaceDetectionModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>

#include <QtWidgets/QFileDialog>

#include "CVImageData.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include "qtvariantproperty_p.h"

const QString CVFaceDetectionModel::_category = QString("Image Processing");

const QString CVFaceDetectionModel::_model_name = QString( "CV Face Detection" );

using namespace std;
using namespace cv;

CascadeClassifier cascade;
int boxSize;

CVFaceDetectionModel::CVFaceDetectionModel() : PBNodeDelegateModel( _model_name ),
    mpEmbeddedWidget( new CVFaceDetectionEmbeddedWidget() ),
    _minPixmap( ":FaceDetection.png" ) 
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&" );
    connect( mpEmbeddedWidget, &CVFaceDetectionEmbeddedWidget::button_clicked_signal, this, &CVFaceDetectionModel::em_button_clicked );
    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );
        
    cascade.load(samples::findFile("haarcascades/haarcascade_frontalface_default.xml"));
        
    EnumPropertyType enumPropertyType;
    enumPropertyType.mslEnumNames = mpEmbeddedWidget->get_combobox_string_list();
    enumPropertyType.miCurrentIndex = 0;
    QString propId = "combobox_id";
    auto propComboBox = std::make_shared< TypedProperty< EnumPropertyType > >("ComboBox", propId, QtVariantPropertyManager::enumTypeId(), enumPropertyType);
    mvProperty.push_back( propComboBox );
    mMapIdToProperty[ propId ] = propComboBox;
}

unsigned int
CVFaceDetectionModel::nPorts(PortType portType) const {
    unsigned int result = 1;

    switch (portType)
    {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 1;
        break;

    default:
        break;
    }

    return result;
}

NodeDataType
CVFaceDetectionModel::dataType(PortType, PortIndex) const {
    return CVImageData().type();
}

std::shared_ptr<NodeData>
CVFaceDetectionModel::outData(PortIndex) {
    if( isEnable() )
        return mpCVImageData;
    else
        return nullptr;
}

void CVFaceDetectionModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex) {

    if( !isEnable() )
        return;

    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );

        if( d )
        {
            cv::Mat faceDetectedImage = processData(d);
            // Move the temporary Mat into CVImageData to avoid an extra deep clone
            mpCVImageData->set_image( std::move(faceDetectedImage) );
        }
    }

    Q_EMIT dataUpdated(0);
}

cv::Mat CVFaceDetectionModel::processData(const std::shared_ptr<CVImageData> &p)
{
    // Avoid deep-copying the source image unless we need to draw on it.
    const cv::Mat &src = p->data();
    cv::Mat grayScaled, reducedSize;
    vector<Rect> objects;

    // Use the source image for detection (no modification of src here)
    cvtColor(src, grayScaled, COLOR_BGR2GRAY );
    resize(grayScaled, reducedSize, Size(), 1, 1, INTER_LINEAR );
    equalizeHist(reducedSize, reducedSize);

    cascade.detectMultiScale(reducedSize, objects, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(30, 30) );

    // If no faces were detected, return a shallow header to avoid an unnecessary deep copy.
    if (objects.empty())
        return src;

    // We need to draw rectangles — clone now to produce an independent image.
    cv::Mat img = src.clone();
    for ( size_t i = 0; i < objects.size(); i++ ) {
        Rect r = objects[i];
        Point topLeft(cvRound(r.x)-boxSize, cvRound(r.y)-boxSize);
        Point bottomRight(cvRound(r.x+r.width)+boxSize, cvRound(r.y+r.height)+boxSize);
        rectangle(img, topLeft, bottomRight, Scalar(255, 0, 0), 8, 8, 0);
    }

    return img;
}

QJsonObject CVFaceDetectionModel::save() const {

    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "combobox_text" ] = mpEmbeddedWidget->get_combobox_text();
    modelJson[ "cParams" ] = cParams;

    return modelJson;
}

void CVFaceDetectionModel::setModelProperty( QString & id, const QVariant & value ) {
    PBNodeDelegateModel::setModelProperty( id, value );

    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >(mMapIdToProperty[ id ]);
    typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( value.toString() );

    boxSize = 25;
    switch( value.toInt() ) {
        case 0:
            cascade.load(samples::findFile("haarcascades/haarcascade_frontalface_default.xml"));
            break;
        case 1:
            cascade.load(samples::findFile("haarcascades/haarcascade_frontalface_alt2.xml"));
            break;
        case 2:
            cascade.load(samples::findFile("haarcascades/haarcascade_frontalface_alt.xml"));
            break;
        case 3:
            cascade.load(samples::findFile("haarcascades/haarcascade_eye_tree_eyeglasses.xml"));
            boxSize = 5;
            break;
    }
    mpEmbeddedWidget->set_combobox_value( value.toString() );
}

void CVFaceDetectionModel::em_button_clicked( int button ) {
    if( button == 3 ) {
        auto prop = mMapIdToProperty[ "combobox_id" ];
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );
        typedProp->getData().miCurrentIndex = typedProp->getData().mslEnumNames.indexOf( mpEmbeddedWidget->get_combobox_text() );
        Q_EMIT property_changed_signal( prop );
    }
}


