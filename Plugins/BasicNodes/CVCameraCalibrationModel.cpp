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

#include "CVCameraCalibrationModel.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <QDebug>
#include "qtvariantproperty_p.h"

const QString CVCameraCalibrationModel::_category = QString("Image Processing");

const QString CVCameraCalibrationModel::_model_name = QString( "CV Camera Calibration" );

CameraCalibrationThread::CameraCalibrationThread( QObject * parent )
    : QThread(parent)
{
    // Shall set this only after a working variable is ready to process data.
    // Might have a function to do this separately.
    mbThreadReady = true;
}


CameraCalibrationThread::
~CameraCalibrationThread()
{
    mbAbort = true;
    stop_thread();
    wait();
    if( mpCharucoDetector )
        delete mpCharucoDetector;
}

void
CameraCalibrationThread::
    start_thread( )
{
    miThreadState = THREAD_DETECT_CORNERS;
    if( !isRunning() )
        start();
}

void
CameraCalibrationThread::
    stop_thread()
{
    miThreadState = THREAD_STOP;
    mWaitingSemaphore.release();
}

void
CameraCalibrationThread::detect_corners( const cv::Mat & in_image )
{
    if( !in_image.empty() )
    {
        miThreadState = THREAD_DETECT_CORNERS;
        in_image.copyTo( mCVImage );
        mWaitingSemaphore.release();
    }
}

void
CameraCalibrationThread::calibrate()
{
    if( !mvCVImages.empty() )
    {
        miThreadState = THREAD_CALIBRATE;
        mWaitingSemaphore.release();
    }
}

void
CameraCalibrationThread::
run()
{
    while( !mbAbort )
    {
        qDebug() <<"Waiting for Semaphore....";
        mWaitingSemaphore.acquire();
        if( !mbThreadReady )
            continue;
        if( miThreadState == THREAD_STOP )
        {
            miThreadState = THREAD_INIT;
            mbThreadReady = false;
            if( mWaitingSemaphore.available() != 0 )
                mWaitingSemaphore.acquire( mWaitingSemaphore.available() );
            continue;
        }
        else if( miThreadState == THREAD_DETECT_CORNERS )
        {
            cv::Mat gray;
            std::vector<cv::Point2f> pointbuf;
            cv::cvtColor( mCVImage, gray, cv::COLOR_BGR2GRAY );
            bool found = false;
            if( mCameraCalibrationParams.miPattern == CHESSBOARD )
                found = cv::findChessboardCorners( mCVImage, mBoardSize, pointbuf, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);
            else if( mCameraCalibrationParams.miPattern == CIRCLES_GRID )
                found = cv::findCirclesGrid( mCVImage, mBoardSize, pointbuf );
            else if( mCameraCalibrationParams.miPattern == ASYMMETRIC_CIRCLES_GRID )
                found = cv::findCirclesGrid( mCVImage, mBoardSize, pointbuf, cv::CALIB_CB_ASYMMETRIC_GRID );
            else if( mCameraCalibrationParams.miPattern == CHARUCOBOARD )
            {

            }
            if( mCameraCalibrationParams.miPattern == CHESSBOARD && found )
                cv::cornerSubPix(gray, pointbuf, cv::Size(11,11), cv::Size(-1,-1), cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.0001));
            if( found )
            {
                if( mCameraCalibrationParams.miPattern != CHARUCOBOARD )
                    cv::drawChessboardCorners( mCVImage, mBoardSize, cv::Mat(pointbuf), found );
                Q_EMIT result_image_ready( mCVImage );
            }
            else if( 0 ) // If error occurs, send a signal to the calling thread.
            {
                Q_EMIT error_signal( 1 );
            }
        }
        else if( miThreadState == THREAD_CALIBRATE )
        {
            cv::Mat gray;
            for( int i = 0; i < static_cast<int>(mvCVImages.size()); i++ )
            {
//cv
            }
        }
    }
}

void
CameraCalibrationThread::
set_params( CameraCalibrationParams & params )
{
    mCameraCalibrationParams = params;
    if( mCameraCalibrationParams.miPattern == CHARUCOBOARD )
    {
        int arucoDict;
        if( mCameraCalibrationParams.miArucoDict == DICT4x4_50 )              arucoDict = cv::aruco::DICT_4X4_50;
        else if( mCameraCalibrationParams.miArucoDict == DICT4x4_100 )        arucoDict = cv::aruco::DICT_4X4_100;
        else if( mCameraCalibrationParams.miArucoDict == DICT4x4_250 )        arucoDict = cv::aruco::DICT_4X4_250;
        else if( mCameraCalibrationParams.miArucoDict == DICT4x4_1000 )       arucoDict = cv::aruco::DICT_4X4_1000;
        else if( mCameraCalibrationParams.miArucoDict == DICT5x5_50 )         arucoDict = cv::aruco::DICT_5X5_50;
        else if( mCameraCalibrationParams.miArucoDict == DICT5x5_100 )        arucoDict = cv::aruco::DICT_5X5_100;
        else if( mCameraCalibrationParams.miArucoDict == DICT5x5_250 )        arucoDict = cv::aruco::DICT_5X5_250;
        else if( mCameraCalibrationParams.miArucoDict == DICT5x5_1000 )       arucoDict = cv::aruco::DICT_5X5_1000;
        else if( mCameraCalibrationParams.miArucoDict == DICT6x6_50 )         arucoDict = cv::aruco::DICT_6X6_50;
        else if( mCameraCalibrationParams.miArucoDict == DICT6x6_100 )        arucoDict = cv::aruco::DICT_6X6_100;
        else if( mCameraCalibrationParams.miArucoDict == DICT6x6_250 )        arucoDict = cv::aruco::DICT_6X6_250;
        else if( mCameraCalibrationParams.miArucoDict == DICT6x6_1000 )       arucoDict = cv::aruco::DICT_6X6_1000;
        else if( mCameraCalibrationParams.miArucoDict == DICT7x7_50 )         arucoDict = cv::aruco::DICT_7X7_50;
        else if( mCameraCalibrationParams.miArucoDict == DICT7x7_100 )        arucoDict = cv::aruco::DICT_7X7_100;
        else if( mCameraCalibrationParams.miArucoDict == DICT7x7_250 )        arucoDict = cv::aruco::DICT_7X7_250;
        else if( mCameraCalibrationParams.miArucoDict == DICT7x7_1000 )       arucoDict = cv::aruco::DICT_7X7_1000;
        else if( mCameraCalibrationParams.miArucoDict == DICTOriginal )       arucoDict = cv::aruco::DICT_ARUCO_ORIGINAL;
        else if( mCameraCalibrationParams.miArucoDict == DICTApriltag_16h5 )  arucoDict = cv::aruco::DICT_7X7_100;
        else if( mCameraCalibrationParams.miArucoDict == DICTApriltag_25h9 )  arucoDict = cv::aruco::DICT_7X7_250;
        else if( mCameraCalibrationParams.miArucoDict == DICTApriltag_36h10 ) arucoDict = cv::aruco::DICT_7X7_1000;
        else if( mCameraCalibrationParams.miArucoDict == DICTApriltag_36h11 ) arucoDict = cv::aruco::DICT_ARUCO_ORIGINAL;

        cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary( arucoDict );
        if( mpCharucoDetector )
            delete mpCharucoDetector;
        cv::aruco::CharucoBoard ch_board(mBoardSize, mCameraCalibrationParams.mfSquare_Size, mCameraCalibrationParams.mfMarker_Size, dictionary );
        mpCharucoDetector = new cv::aruco::CharucoDetector(ch_board);
    }
    mBoardSize.width = mCameraCalibrationParams.miChessBoard_Cols;
    mBoardSize.height = mCameraCalibrationParams.miChessBoard_Rows;
}

void
CameraCalibrationThread::
calcChessboardCorners(cv::Size boardSize, float squareSize, std::vector< cv::Point3f >& corners, CameraCalibPattern patternType)
{
    corners.resize(0);

    switch(patternType)
    {
    case CHESSBOARD:
    case CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; i++ )
            for( int j = 0; j < boardSize.width; j++ )
                corners.push_back(cv::Point3f(float(j*squareSize),
                                          float(i*squareSize), 0));
        break;

    case ASYMMETRIC_CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; i++ )
            for( int j = 0; j < boardSize.width; j++ )
                corners.push_back(cv::Point3f(float((2*j + i % 2)*squareSize),
                                          float(i*squareSize), 0));
        break;

    case CHARUCOBOARD:
        for( int i = 0; i < boardSize.height-1; i++ )
            for( int j = 0; j < boardSize.width-1; j++ )
                corners.push_back(cv::Point3f(float(j*squareSize),
                                          float(i*squareSize), 0));
        break;
    default:
        CV_Error(cv::Error::StsBadArg, "Unknown pattern type\n");
    }
}

double CameraCalibrationThread::
computeReprojectionErrors(
    const std::vector< std::vector<cv::Point3f> >& objectPoints,
    const std::vector< std::vector<cv::Point2f> >& imagePoints,
    const std::vector< cv::Mat >& rvecs, const std::vector< cv::Mat >& tvecs,
    const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs,
    std::vector< float >& perViewErrors )
{
    std::vector< cv::Point2f > imagePoints2;
    int i, totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for( i = 0; i < (int)objectPoints.size(); i++ )
    {
        projectPoints( cv::Mat(objectPoints[i]), rvecs[i], tvecs[i],
                      cameraMatrix, distCoeffs, imagePoints2);
        err = norm( cv::Mat(imagePoints[i]), cv::Mat(imagePoints2), cv::NORM_L2);
        int n = (int)objectPoints[i].size();
        perViewErrors[i] = (float)std::sqrt(err*err/n);
        totalErr += err*err;
        totalPoints += n;
    }

    return std::sqrt(totalErr/totalPoints);
}

bool
CameraCalibrationThread::
runCalibration( std::vector< std::vector<cv::Point2f> > imagePoints, cv::Size imageSize, cv::Size boardSize, CameraCalibPattern patternType,
               float squareSize, float aspectRatio, float grid_width, bool release_object, int flags, cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
               std::vector< cv::Mat >& rvecs, std::vector< cv::Mat >& tvecs, std::vector< float >& reprojErrs, std::vector< cv::Point3f >& newObjPoints,
               double& totalAvgErr )
{
    if( flags & cv::CALIB_FIX_ASPECT_RATIO )
        cameraMatrix.at<double>(0,0) = aspectRatio;

    distCoeffs = cv::Mat::zeros(8, 1, CV_64F);

    std::vector< std::vector<cv::Point3f> > objectPoints(1);
    calcChessboardCorners(boardSize, squareSize, objectPoints[0], patternType);
    int offset = patternType != CHARUCOBOARD ? boardSize.width - 1: boardSize.width - 2;
    objectPoints[0][offset].x = objectPoints[0][0].x + grid_width;
    newObjPoints = objectPoints[0];

    objectPoints.resize(imagePoints.size(),objectPoints[0]);

    double rms;
    int iFixedPoint = -1;
    if (release_object)
        iFixedPoint = boardSize.width - 1;
    rms = calibrateCameraRO(objectPoints, imagePoints, imageSize, iFixedPoint,
                            cameraMatrix, distCoeffs, rvecs, tvecs, newObjPoints,
                            flags | cv::CALIB_USE_LU);
    qDebug() << "RMS error reported by calibrateCamera: " << rms;

    bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);

    if (release_object) {
        std::cout << "New board corners: " << std::endl;
        std::cout << newObjPoints[0] << std::endl;
        std::cout << newObjPoints[boardSize.width - 1] << std::endl;
        std::cout << newObjPoints[boardSize.width * (boardSize.height - 1)] << std::endl;
        std::cout << newObjPoints.back() << std::endl;
    }

    objectPoints.clear();
    objectPoints.resize(imagePoints.size(), newObjPoints);
    totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints,
                                            rvecs, tvecs, cameraMatrix, distCoeffs, reprojErrs);

    return ok;
}


///////////////////////////////////////////////////////////////////

CVCameraCalibrationModel::
CVCameraCalibrationModel()
    : PBNodeDelegateModel( _model_name ),
    mpEmbeddedWidget( new CVCameraCalibrationEmbeddedWidget( qobject_cast<QWidget *>(this) ) )
{
    qRegisterMetaType<cv::Mat>( "cv::Mat&");
    connect( mpEmbeddedWidget, &CVCameraCalibrationEmbeddedWidget::button_clicked_signal, this, &CVCameraCalibrationModel::em_button_clicked );

    mpCVImageData = std::make_shared< CVImageData >( cv::Mat() );

    PathPropertyType pathPropertyType;
    pathPropertyType.msPath = msWorkingDirname;
    QString propId = "working_dirname";
    auto propWorkingDirname = std::make_shared< TypedProperty< PathPropertyType > >("Working Directory", propId, QtVariantPropertyManager::pathTypeId(), pathPropertyType);
    mvProperty.push_back( propWorkingDirname );
    mMapIdToProperty[ propId ] = propWorkingDirname;

//    mEnumPattern.mslEnumNames = {"Chessboard","Charuco", "Circles", "Acircles"};
    mEnumPattern.miCurrentIndex = mCameraCalibrationParams.miPattern;
    propId = "pattern";
    auto propPattern = std::make_shared< TypedProperty < EnumPropertyType > >("Pattern", propId, QtVariantPropertyManager::enumTypeId(), mEnumPattern );
    mvProperty.push_back( propPattern );
    mMapIdToProperty[ propId ] = propPattern;
/*
    mEnumArucoDict.mslEnumNames = {"4x4 50", "4X4 100", "4X4 250", "4x4 1000",
                                     "5x5 50", "5X5 100", "5X5 250", "5x5 1000",
                                     "6x6 50", "6X6 100", "6X6 250", "6x6 1000",
                                     "7x7 50", "7X7 100", "7X7 250", "7x7 1000",
                                     "Original", "Apriltag 16h5", "Apriltag 25h9", "Apriltag 36h10", "Apriltag 36h11"
                                    };
*/
    mEnumArucoDict.miCurrentIndex = mCameraCalibrationParams.miArucoDict;
    propId = "aruco_dict";
    auto propArucoDict = std::make_shared< TypedProperty < EnumPropertyType > >("Aruco DICT", propId, QtVariantPropertyManager::enumTypeId(), mEnumArucoDict );
    mvProperty.push_back( propArucoDict );
    mMapIdToProperty[ propId ] = propArucoDict;

    SizePropertyType sizePropertyType;
    sizePropertyType.miWidth = mCameraCalibrationParams.miChessBoard_Cols;
    sizePropertyType.miHeight = mCameraCalibrationParams.miChessBoard_Rows;
    propId = "inner_corners";
    auto propChessBoard = std::make_shared< TypedProperty< SizePropertyType > >("No Inner Corners", propId, QMetaType::QSize, sizePropertyType);
    mvProperty.push_back( propChessBoard );
    mMapIdToProperty[ propId ] = propChessBoard;

    DoublePropertyType doublePropertyType;
    doublePropertyType.mdValue = mCameraCalibrationParams.mfSquare_Size;
    doublePropertyType.mdMax = 7777;
    doublePropertyType.mdMin = 0.0007;
    propId = "square_size";
    auto squareSize = std::make_shared< TypedProperty< DoublePropertyType > >("Square Size", propId, QMetaType::Double, doublePropertyType);
    mvProperty.push_back( squareSize );
    mMapIdToProperty[ propId ] = squareSize;

    doublePropertyType.mdValue = mCameraCalibrationParams.mfMarker_Size;
    propId = "marker_size";
    auto markerSize = std::make_shared< TypedProperty< DoublePropertyType > >("Marker Size", propId, QMetaType::Double, doublePropertyType);
    mvProperty.push_back( markerSize );
    mMapIdToProperty[ propId ] = markerSize;

    doublePropertyType.mdValue = mCameraCalibrationParams.mfActualDistanceTopLeftRightCorner;
    doublePropertyType.mdMax = 7777;
    doublePropertyType.mdMin = 0.0007;
    propId = "top_left2right_distance";
    auto left2rightDistance = std::make_shared< TypedProperty< DoublePropertyType > >("Top Left to Right Distance", propId, QMetaType::Double, doublePropertyType);
    mvProperty.push_back( left2rightDistance );
    mMapIdToProperty[ propId ] = left2rightDistance;

    doublePropertyType.mdValue = mCameraCalibrationParams.mfFixAspectRation;
    doublePropertyType.mdMax = 7777;
    doublePropertyType.mdMin = 0.0007;
    propId = "aspect_ratio";
    auto aspectRatio = std::make_shared< TypedProperty< DoublePropertyType > >("Aspect Ratio", propId, QMetaType::Double, doublePropertyType);
    mvProperty.push_back( aspectRatio );
    mMapIdToProperty[ propId ] = aspectRatio;

    sizePropertyType.miWidth = mCameraCalibrationParams.miSearchWindow_Width;
    sizePropertyType.miHeight = mCameraCalibrationParams.miSearchWindow_Height;
    propId = "search_window";
    auto propSearchWindow = std::make_shared< TypedProperty< SizePropertyType > >("Search Window for Sub Pixel Accuracy", propId, QMetaType::QSize, sizePropertyType);
    mvProperty.push_back( propSearchWindow );
    mMapIdToProperty[ propId ] = propSearchWindow;

    propId = "enable_k3";
    auto propEnableK3 = std::make_shared< TypedProperty< bool > >("K3 Coeff.", propId, QMetaType::Bool, mCameraCalibrationParams.mbEnableK3, "Options" );
    mvProperty.push_back( propEnableK3 );
    mMapIdToProperty[ propId ] = propEnableK3;

    propId = "write_detected";
    auto propWriteDetectedFeatures = std::make_shared< TypedProperty< bool > >("Write Detected Features", propId, QMetaType::Bool, mCameraCalibrationParams.mbWriteDetectedFeatures, "Options" );
    mvProperty.push_back( propWriteDetectedFeatures );
    mMapIdToProperty[ propId ] = propWriteDetectedFeatures;

    propId = "write_extrinsic_params";
    auto propWriteExtrinsicParams = std::make_shared< TypedProperty< bool > >("Write Extrinsic Params", propId, QMetaType::Bool, mCameraCalibrationParams.mbWriteExtrinsicParams, "Options" );
    mvProperty.push_back( propWriteExtrinsicParams );
    mMapIdToProperty[ propId ] = propWriteExtrinsicParams;

    propId = "write_refined_3d_points";
    auto propWriteRefined3DPoints = std::make_shared< TypedProperty< bool > >("Write Refined 3D Points", propId, QMetaType::Bool, mCameraCalibrationParams.mbWriteRefined3DPoints, "Options" );
    mvProperty.push_back( propWriteRefined3DPoints );
    mMapIdToProperty[ propId ] = propWriteRefined3DPoints;

    propId = "assume_zero_tangential_dist";
    auto propAssume0TangentialDist = std::make_shared< TypedProperty< bool > >("Assume 0 Tangential Dist", propId, QMetaType::Bool, mCameraCalibrationParams.mbAssumeZeroTangentialDistortion, "Options" );
    mvProperty.push_back( propAssume0TangentialDist );
    mMapIdToProperty[ propId ] = propAssume0TangentialDist;

    propId = "fix_prn_pnt_at_ctr";
    auto propFixPrnPntAtCtr = std::make_shared< TypedProperty< bool > >("Fix Principal Point at Center", propId, QMetaType::Bool, mCameraCalibrationParams.mbFixPrincipalPointAtCenter, "Options" );
    mvProperty.push_back( propFixPrnPntAtCtr );
    mMapIdToProperty[ propId ] = propFixPrnPntAtCtr;

    propId = "flip_images";
    auto propFlipImages = std::make_shared< TypedProperty< bool > >("Flip Images", propId, QMetaType::Bool, mCameraCalibrationParams.mbFlipImages, "Options" );
    mvProperty.push_back( propFlipImages );
    mMapIdToProperty[ propId ] = propFlipImages;

    propId = "save_undist_images";
    auto propSaveUndistImages = std::make_shared< TypedProperty< bool > >("Save Undistorted Images", propId, QMetaType::Bool, mCameraCalibrationParams.mbSaveUndistortedImages, "Options" );
    mvProperty.push_back( propSaveUndistImages );
    mMapIdToProperty[ propId ] = propSaveUndistImages;

    set_flags();
 /*
 * int cols x int rows of the chessboard (no corners)
 * float square of the chessboard size, must bigger than marker size.
 * float marker size, normally half the square of the chessboard size.
 * if using aruco chessboard, a drop list of arucoDict.
 * string output camera parameter filename
 * bool write detected points
 * bool write extrinsic params
 * bool write refined 3d points
 * bool assume zero tangential distortion
 * double fix aspect ratio (fx/fy)
 * bool fix the principal point at the center
 * bool flip the captured images around the herizontal axis
 * bool save undistorted images
 * string directory to save undistorted images
 * bool enable or disable K3 coefficient for the distortion model
 * dobule actual distance between top-left and top-right corner
 */
}

void
CVCameraCalibrationModel::
set_flags()
{
    mCameraCalibrationParams.miFlags = 0;
    if( mCameraCalibrationParams.mfFixAspectRation != 0 )
        mCameraCalibrationParams.miFlags |= cv::CALIB_FIX_ASPECT_RATIO;
    if( mCameraCalibrationParams.mbAssumeZeroTangentialDistortion )
        mCameraCalibrationParams.miFlags |= cv::CALIB_ZERO_TANGENT_DIST;
    if( mCameraCalibrationParams.mbFixPrincipalPointAtCenter )
        mCameraCalibrationParams.miFlags |= cv::fisheye::CALIB_FIX_PRINCIPAL_POINT;
    if( !mCameraCalibrationParams.mbEnableK3 )
        mCameraCalibrationParams.miFlags |= cv::CALIB_FIX_K3;
}

unsigned int
CVCameraCalibrationModel::
nPorts(PortType portType) const
{
    unsigned int result = 0;

    if ( portType == PortType::In )
        result = 1;
    else if( portType == PortType::Out )
        result = 1;
    return result;
}

NodeDataType
CVCameraCalibrationModel::
dataType(PortType portType, PortIndex portIndex) const
{
    if( portType == PortType::In && portIndex == 0 )
        return CVImageData().type();
    else if( portType == PortType::Out && portIndex == 0 )
        return CVImageData().type();
    return NodeDataType();
}

std::shared_ptr<NodeData>
CVCameraCalibrationModel::
outData( PortIndex port )
{
    if( isEnable() )
    {
        if( port == 0 )
            return mpCVImageData;
    }
    return nullptr;
}

void
CVCameraCalibrationModel::
setInData( std::shared_ptr< NodeData > nodeData, PortIndex )
{
    if( !isEnable() )
        return;
    if( nodeData )
    {
        auto d = std::dynamic_pointer_cast< CVImageData >( nodeData );
        if( d )
            processData( d );
    }
}


QJsonObject
CVCameraCalibrationModel::
save() const
{
    QJsonObject modelJson = PBNodeDelegateModel::save();

    QJsonObject cParams;
    cParams[ "working_dirname" ] = msWorkingDirname;
    if( mCameraCalibrationParams.miPattern >= 0 && mCameraCalibrationParams.miPattern < mEnumPattern.mslEnumNames.size() )
        cParams[ "pattern" ] = mEnumPattern.mslEnumNames[mCameraCalibrationParams.miPattern];
    else
        cParams[ "pattern" ] = "Chessboard";
    if( mCameraCalibrationParams.miArucoDict >= 0 && mCameraCalibrationParams.miArucoDict < mEnumArucoDict.mslEnumNames.size() )
        cParams[ "aruco_dict" ] = mEnumArucoDict.mslEnumNames[mCameraCalibrationParams.miArucoDict];
    else
        cParams[ "aruco_dict" ] = "Original";
    cParams[ "inner_corners_cols" ] = mCameraCalibrationParams.miChessBoard_Cols;
    cParams[ "inner_corners_rows" ] = mCameraCalibrationParams.miChessBoard_Rows;
    cParams[ "square_size" ] = mCameraCalibrationParams.mfSquare_Size;
    cParams[ "marker_size" ] = mCameraCalibrationParams.mfMarker_Size;
    cParams[ "top_left2right_distance" ] = mCameraCalibrationParams.mfActualDistanceTopLeftRightCorner;
    cParams[ "aspect_ratio" ] = mCameraCalibrationParams.mfFixAspectRation;
    cParams[ "search_window_width" ] = mCameraCalibrationParams.miSearchWindow_Width;
    cParams[ "search_window_height" ] = mCameraCalibrationParams.miSearchWindow_Height;

    cParams[ "enable_k3" ] = mCameraCalibrationParams.mbEnableK3;
    cParams[ "write_detected" ] = mCameraCalibrationParams.mbWriteDetectedFeatures;
    cParams[ "write_extrinsic_params" ] = mCameraCalibrationParams.mbWriteExtrinsicParams;
    cParams[ "write_refined_3d_points" ] = mCameraCalibrationParams.mbWriteRefined3DPoints;
    cParams[ "assume_zero_tangential_dist" ] = mCameraCalibrationParams.mbAssumeZeroTangentialDistortion;
    cParams[ "fix_prn_pnt_at_ctr" ] = mCameraCalibrationParams.mbFixPrincipalPointAtCenter;
    cParams[ "flip_images" ] = mCameraCalibrationParams.mbFlipImages;
    cParams[ "save_undist_images" ] = mCameraCalibrationParams.mbSaveUndistortedImages;

    cParams[ "auto_capture" ] = mbAutoCapture;

    modelJson["cParams"] = cParams;

    return modelJson;
}


void
CVCameraCalibrationModel::
load(QJsonObject const &p)
{
    PBNodeDelegateModel::load(p);

    QJsonObject paramsObj = p["cParams"].toObject();
    if( !paramsObj.isEmpty() )
    {
        QJsonValue v = paramsObj[ "working_dirname" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "working_dirname" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType > >( prop );
            typedProp->getData().msPath = v.toString();

            msWorkingDirname = v.toString();
        }
        v = paramsObj[ "pattern" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "pattern" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );

            mCameraCalibrationParams.miPattern  = typedProp->getData().mslEnumNames.indexOf( v.toString() );
            typedProp->getData().miCurrentIndex = mCameraCalibrationParams.miPattern;
        }
        v = paramsObj[ "aruco_dict" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "aruco_dict" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > >( prop );

            mCameraCalibrationParams.miArucoDict  = typedProp->getData().mslEnumNames.indexOf( v.toString() );
            typedProp->getData().miCurrentIndex = mCameraCalibrationParams.miArucoDict;
        }
        v = paramsObj[ "inner_corners_cols" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "inner_corners" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = v.toInt();

            mCameraCalibrationParams.miChessBoard_Cols = v.toInt();
        }
        v = paramsObj[ "inner_corners_rows" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "inner_corners" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miHeight = v.toInt();

            mCameraCalibrationParams.miChessBoard_Rows = v.toInt();
        }
        v = paramsObj[ "square_size" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "square_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mCameraCalibrationParams.mfSquare_Size = v.toDouble();
        }
        v = paramsObj[ "marker_size" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "marker_size" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mCameraCalibrationParams.mfMarker_Size = v.toDouble();
        }
        v = paramsObj[ "top_left2right_distance" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "top_left2right_distance" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mCameraCalibrationParams.mfActualDistanceTopLeftRightCorner = v.toDouble();
        }
        v = paramsObj[ "aspect_ratio" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "aspect_ratio" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
            typedProp->getData().mdValue = v.toDouble();

            mCameraCalibrationParams.mfFixAspectRation = v.toDouble();
        }
        v = paramsObj[ "search_window_width" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "search_window" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miWidth = v.toInt();

            mCameraCalibrationParams.miSearchWindow_Width = v.toInt();
        }
        v = paramsObj[ "search_window_height" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "search_window" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
            typedProp->getData().miHeight = v.toInt();

            mCameraCalibrationParams.miSearchWindow_Height = v.toInt();
        }
        v = paramsObj[ "enable_k3" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "enable_k3" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mCameraCalibrationParams.mbEnableK3 = v.toBool();
        }
        v = paramsObj[ "write_detected" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "write_detected" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mCameraCalibrationParams.mbWriteDetectedFeatures = v.toBool();
        }
        v = paramsObj[ "write_extrinsic_params" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "write_extrinsic_params" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mCameraCalibrationParams.mbWriteExtrinsicParams = v.toBool();
        }
         v = paramsObj[ "write_refined_3d_points" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "write_refined_3d_points" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mCameraCalibrationParams.mbWriteRefined3DPoints = v.toBool();
        }
         v = paramsObj[ "assume_zero_tangential_dist" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "assume_zero_tangential_dist" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mCameraCalibrationParams.mbAssumeZeroTangentialDistortion = v.toBool();
        }
         v = paramsObj[ "fix_prn_pnt_at_ctr" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "fix_prn_pnt_at_ctr" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mCameraCalibrationParams.mbFixPrincipalPointAtCenter = v.toBool();
        }
         v = paramsObj[ "flip_images" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "flip_images" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mCameraCalibrationParams.mbFlipImages = v.toBool();
        }
        v = paramsObj[ "save_undist_images" ];
        if( !v.isNull() )
        {
            auto prop = mMapIdToProperty[ "save_undist_images" ];
            auto typedProp = std::static_pointer_cast< TypedProperty< bool > >( prop );
            typedProp->getData() = v.toBool();

            mCameraCalibrationParams.mbSaveUndistortedImages = v.toBool();
        }
        v = paramsObj[ "auto_capture" ];
        if( !v.isNull() )
        {
            mbAutoCapture = v.toBool();
            mpEmbeddedWidget->set_auto_capture_flag( mbAutoCapture );
        }
    }
    set_flags();
    late_constructor();
}


void
CVCameraCalibrationModel::
setModelProperty( QString & id, const QVariant & value )
{
    PBNodeDelegateModel::setModelProperty( id, value );
    if( !mMapIdToProperty.contains( id ) )
        return;

    auto prop = mMapIdToProperty[ id ];
    if( id == "working_dirname" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< PathPropertyType> > (prop);
        msWorkingDirname = value.toString();

        typedProp->getData().msPath = msWorkingDirname;
    }
    else if( id == "pattern" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > > (prop);
        mCameraCalibrationParams.miPattern = value.toInt();

        typedProp->getData().miCurrentIndex = mCameraCalibrationParams.miPattern;
    }
    else if( id == "aruco_dict" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< EnumPropertyType > > (prop);
        mCameraCalibrationParams.miArucoDict = value.toInt();

        typedProp->getData().miCurrentIndex = mCameraCalibrationParams.miArucoDict;
    }
    else if( id == "inner_corners" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
        mCameraCalibrationParams.miChessBoard_Cols = value.toSize().width();
        mCameraCalibrationParams.miChessBoard_Rows = value.toSize().height();

        typedProp->getData().miWidth = mCameraCalibrationParams.miChessBoard_Cols;
        typedProp->getData().miHeight = mCameraCalibrationParams.miChessBoard_Rows;
    }
    else if( id == "square_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mCameraCalibrationParams.mfSquare_Size = value.toDouble();
    }
    else if( id == "marker_size" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType > >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mCameraCalibrationParams.mfMarker_Size = value.toDouble();
    }
    else if( id == "top_left2right_distance" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mCameraCalibrationParams.mfActualDistanceTopLeftRightCorner = value.toDouble();
    }
    else if( id == "aspect_ratio" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< DoublePropertyType> >( prop );
        typedProp->getData().mdValue = value.toDouble();

        mCameraCalibrationParams.mfFixAspectRation = value.toDouble();
    }
    else if( id == "search_window" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty< SizePropertyType > >( prop );
        mCameraCalibrationParams.miSearchWindow_Width = value.toSize().width();
        mCameraCalibrationParams.miSearchWindow_Height = value.toSize().height();

        typedProp->getData().miWidth = mCameraCalibrationParams.miSearchWindow_Width;
        typedProp->getData().miHeight = mCameraCalibrationParams.miSearchWindow_Height;
    }
    else if( id == "enable_k3" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop );
        typedProp->getData() = value.toBool();

        mCameraCalibrationParams.mbEnableK3 = value.toBool();
    }
    else if( id == "write_detected" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop );
        typedProp->getData() = value.toBool();

        mCameraCalibrationParams.mbWriteDetectedFeatures = value.toBool();
    }
    else if( id == "write_extrinsic_params" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop );
        typedProp->getData() = value.toBool();

        mCameraCalibrationParams.mbWriteExtrinsicParams = value.toBool();
    }
    else if( id == "write_refined_3d_points" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop );
        typedProp->getData() = value.toBool();

        mCameraCalibrationParams.mbWriteRefined3DPoints = value.toBool();
    }
    else if( id == "assume_zero_tangential_dist" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop );
        typedProp->getData() = value.toBool();

        mCameraCalibrationParams.mbAssumeZeroTangentialDistortion = value.toBool();
    }
    else if( id == "fix_prn_pnt_at_ctr" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop );
        typedProp->getData() = value.toBool();

        mCameraCalibrationParams.mbFixPrincipalPointAtCenter = value.toBool();
    }
    else if( id == "flip_images" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop );
        typedProp->getData() = value.toBool();

        mCameraCalibrationParams.mbFlipImages = value.toBool();
    }
    else if( id == "save_undist_images" )
    {
        auto typedProp = std::static_pointer_cast< TypedProperty < bool > >( prop );
        typedProp->getData() = value.toBool();

        mCameraCalibrationParams.mbSaveUndistortedImages = value.toBool();
    }
    set_flags();
    mpCameraCalibrationThread->set_params( mCameraCalibrationParams );
}


void
CVCameraCalibrationModel::
late_constructor()
{
    if( !mpCameraCalibrationThread )
    {
        mpCameraCalibrationThread = new CameraCalibrationThread(this);
        mpCameraCalibrationThread->set_params( mCameraCalibrationParams );

        connect( mpCameraCalibrationThread, &CameraCalibrationThread::error_signal, this, &CVCameraCalibrationModel::thread_error_occured );
        connect( mpCameraCalibrationThread, &CameraCalibrationThread::result_image_ready, this, &CVCameraCalibrationModel::received_result );
        mpCameraCalibrationThread->start_thread();
    }
}


void
CVCameraCalibrationModel::
processData(const std::shared_ptr<CVImageData> &in )
{
    cv::Mat & in_image = in->data();
    mOrgCVImage = in->data();
    if( !in_image.empty() )
    {
        mbInMemoryImage = false;
        mpCameraCalibrationThread->detect_corners( in_image );
    }
}


void
CVCameraCalibrationModel::
thread_error_occured( int )
{

}

void
CVCameraCalibrationModel::
received_result( cv::Mat & image )
{
    mpCVImageData->data() = image;
    //mpCVImageData->set_image( image );
    //qDebug() << "Got result!";
    if( mbAutoCapture && !mbInMemoryImage )
    {
        mpCameraCalibrationThread->get_images().push_back( mOrgCVImage.clone() );
        mpEmbeddedWidget->update_total_images( mpCameraCalibrationThread->get_images().size() );
        miCurrentDisplayImage = mpCameraCalibrationThread->get_images().size() - 1;
        mpEmbeddedWidget->set_image_number( miCurrentDisplayImage );
    }
    updateAllOutputPorts();
}

void
CVCameraCalibrationModel::
em_button_clicked( int button )
{
    DEBUG_LOG_INFO() << "[em_button_clicked] button:" << button << "isSelected:" << isSelected();
    
    // If node is not selected, select it first and block the interaction
    // User needs to click again when node is selected to perform the action
    if (!isSelected())
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Node not selected, requesting selection";
        Q_EMIT selection_request_signal();
        return;
    }
    
    if( button == 0 && mpCameraCalibrationThread->get_images().size() > 0 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Forward button";
        if( miCurrentDisplayImage < static_cast<int>(mpCameraCalibrationThread->get_images().size()) - 1 )
        {
            miCurrentDisplayImage += 1;
            mpEmbeddedWidget->set_image_number( miCurrentDisplayImage );
            mbInMemoryImage = true;
            mpCameraCalibrationThread->detect_corners( mpCameraCalibrationThread->get_images()[miCurrentDisplayImage] );
        }
        // forward
    }
    else if( button == 1 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Backward button";
        if( miCurrentDisplayImage > 0 )
        {
            miCurrentDisplayImage -= 1;
            mpEmbeddedWidget->set_image_number( miCurrentDisplayImage );
            mbInMemoryImage = true;
            mpCameraCalibrationThread->detect_corners( mpCameraCalibrationThread->get_images()[miCurrentDisplayImage] );
        }
        // backward
    }
    else if( button == 2 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Export button";
        // export
    }
    else if( button == 3 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Capture button";
        // capture
        if( !mOrgCVImage.empty() )
        {
            mpCameraCalibrationThread->get_images().push_back( mOrgCVImage.clone() );
            mpEmbeddedWidget->update_total_images( mpCameraCalibrationThread->get_images().size() );
            miCurrentDisplayImage = mpCameraCalibrationThread->get_images().size() - 1;
            mpEmbeddedWidget->set_image_number( miCurrentDisplayImage );
        }
    }
    else if( button == 4 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Calibrate button";
        // calibrate
    }
    else if( button == 5 )
    {
        DEBUG_LOG_INFO() << "[em_button_clicked] Delete button";
        if( miCurrentDisplayImage != -1 )
        {
            mpCameraCalibrationThread->get_images().erase( mpCameraCalibrationThread->get_images().begin() + miCurrentDisplayImage );
            mpEmbeddedWidget->update_total_images( mpCameraCalibrationThread->get_images().size() );
            miCurrentDisplayImage -= 1;
            if( mpCameraCalibrationThread->get_images().size() != 0 )
            {
                if( miCurrentDisplayImage == -1 )
                    miCurrentDisplayImage = 0;
                mbInMemoryImage = true;
                mpCameraCalibrationThread->detect_corners( mpCameraCalibrationThread->get_images()[miCurrentDisplayImage] );
            }
            mpEmbeddedWidget->set_image_number( miCurrentDisplayImage );
        }
        // remove
    }
    else if( button == 10 )
    {
        mbAutoCapture = false;
        mpEmbeddedWidget->set_auto_capture_flag(false);
    }
    else if( button == 12 )
    {
        mbAutoCapture = true;
        mpEmbeddedWidget->set_auto_capture_flag(true);
    }
}


