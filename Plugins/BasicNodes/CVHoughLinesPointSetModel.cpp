#include "CVHoughLinesPointSetModel.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include "qtvariantproperty_p.h"

const QString CVHoughLinesPointSetModel::_category = QString("Image Processing");
const QString CVHoughLinesPointSetModel::_model_name = QString("CV Hough Lines PointSet");
const std::string CVHoughLinesPointSetModel::color[3] = {"B", "G", "R"};

void CVHoughLinesPointSetWorker::processFrame(cv::Mat input,
                                              CVHoughLinesPointSetParams params,
                                              FrameSharingMode mode,
                                              std::shared_ptr<CVImagePool> pool,
                                              long frameId,
                                              QString producerId)
{
    if (input.empty())
    {
        Q_EMIT frameReady(nullptr, nullptr);
        return;
    }

    // Ensure grayscale for point extraction
    cv::Mat gray;
    if (input.channels() == 1)
        gray = input;
    else if (input.channels() == 3)
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    else if (input.channels() == 4)
        cv::cvtColor(input, gray, cv::COLOR_BGRA2GRAY);
    else
    {
        Q_EMIT frameReady(nullptr, nullptr);
        return;
    }

    FrameMetadata metadata;
    metadata.producerId = producerId;
    metadata.frameId = frameId;

    // Build point set from non-zero pixels
    std::vector<cv::Point2f> points;
    points.reserve(static_cast<size_t>(gray.total() / 4));
    for (int y = 0; y < gray.rows; ++y)
    {
        const uchar *row = gray.ptr<uchar>(y);
        for (int x = 0; x < gray.cols; ++x)
            if (row[x])
                points.emplace_back((float)x, (float)y);
    }

    std::vector<cv::Vec3f> lines; // rho, theta, votes
    if (!points.empty())
    {
        double minTheta = params.mdMinThetaDeg * CV_PI / 180.0;
        double maxTheta = params.mdMaxThetaDeg * CV_PI / 180.0;
        double thetaStep = params.mdThetaStepDeg * CV_PI / 180.0;
        cv::HoughLinesPointSet(points, lines, params.miLinesMax, params.miThreshold, params.mdMinRho, params.mdMaxRho, params.mdRhoStep, minTheta, maxTheta, thetaStep);
        if (params.mbStrongestOnly && !lines.empty())
        {
            std::sort(lines.begin(), lines.end(), [](const cv::Vec3f &a, const cv::Vec3f &b)
                      { return a[2] > b[2]; });
            if (static_cast<int>(lines.size()) > params.miLinesMax)
                lines.resize(params.miLinesMax);
        }
    }

    auto newImageData = std::make_shared<CVImageData>(cv::Mat());
    bool pooled = false;
    if (mode == FrameSharingMode::PoolMode && pool)
    {
        auto handle = pool->acquire(3, metadata); // 3 channel BGR
        if (handle)
        {
            cv::cvtColor(gray, handle.matrix(), cv::COLOR_GRAY2BGR);
            if (params.mbDisplayLines)
            {
                for (auto &lv : lines)
                {
                    float rho = lv[0];
                    float theta = lv[1];
                    double a = cos(theta), b = sin(theta);
                    double x0 = a * rho;
                    double y0 = b * rho;
                    cv::Point pt1(cvRound(x0 + 1000 * (-b)), cvRound(y0 + 1000 * (a)));
                    cv::Point pt2(cvRound(x0 - 1000 * (-b)), cvRound(y0 - 1000 * (a)));
                    cv::line(handle.matrix(), pt1, pt2, cv::Scalar(params.mucLineColor[0], params.mucLineColor[1], params.mucLineColor[2]), params.miLineThickness, params.miLineType);
                }
            }
            if (!handle.matrix().empty() && newImageData->adoptPoolFrame(std::move(handle)))
                pooled = true;
        }
    }
    if (!pooled)
    {
        cv::Mat result;
        cv::cvtColor(gray, result, cv::COLOR_GRAY2BGR);
        if (params.mbDisplayLines)
        {
            for (auto &lv : lines)
            {
                float rho = lv[0];
                float theta = lv[1];
                double a = cos(theta), b = sin(theta);
                double x0 = a * rho;
                double y0 = b * rho;
                cv::Point pt1(cvRound(x0 + 1000 * (-b)), cvRound(y0 + 1000 * (a)));
                cv::Point pt2(cvRound(x0 - 1000 * (-b)), cvRound(y0 - 1000 * (a)));
                cv::line(result, pt1, pt2, cv::Scalar(params.mucLineColor[0], params.mucLineColor[1], params.mucLineColor[2]), params.miLineThickness, params.miLineType);
            }
        }
        if (result.empty())
        {
            Q_EMIT frameReady(nullptr, nullptr);
            return;
        }
        newImageData->updateMove(std::move(result), metadata);
    }
    auto countData = std::make_shared<IntegerData>(static_cast<int>(lines.size()));
    Q_EMIT frameReady(newImageData, countData);
}

CVHoughLinesPointSetModel::CVHoughLinesPointSetModel() : PBAsyncDataModel(_model_name), _minPixmap(":CVHoughLinesPointSet.png")
{
    mpIntegerData = std::make_shared<IntegerData>(int());

    IntPropertyType intPropertyType;
    DoublePropertyType doublePropertyType;
    UcharPropertyType ucharPropertyType;
    EnumPropertyType enumPropertyType;

    // Lines Max
    intPropertyType.miValue = mParams.miLinesMax;
    intPropertyType.miMin = 1;
    intPropertyType.miMax = 1000;
    QString propId = "lines_max";
    auto propLinesMax = std::make_shared<TypedProperty<IntPropertyType>>("Lines Max", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propLinesMax);
    mMapIdToProperty[propId] = propLinesMax;

    // Threshold
    intPropertyType.miValue = mParams.miThreshold;
    intPropertyType.miMin = 1;
    intPropertyType.miMax = 10000;
    propId = "threshold";
    auto propThreshold = std::make_shared<TypedProperty<IntPropertyType>>("Threshold", propId, QMetaType::Int, intPropertyType, "Operation");
    mvProperty.push_back(propThreshold);
    mMapIdToProperty[propId] = propThreshold;

    // Rho min/max/step
    doublePropertyType.mdValue = mParams.mdMinRho;
    doublePropertyType.mdMin = -5000.0;
    doublePropertyType.mdMax = 5000.0;
    propId = "min_rho";
    auto propMinRho = std::make_shared<TypedProperty<DoublePropertyType>>("Min Rho", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propMinRho);
    mMapIdToProperty[propId] = propMinRho;

    doublePropertyType.mdValue = mParams.mdMaxRho;
    doublePropertyType.mdMin = -5000.0;
    doublePropertyType.mdMax = 5000.0;
    propId = "max_rho";
    auto propMaxRho = std::make_shared<TypedProperty<DoublePropertyType>>("Max Rho", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propMaxRho);
    mMapIdToProperty[propId] = propMaxRho;

    doublePropertyType.mdValue = mParams.mdRhoStep;
    doublePropertyType.mdMin = 0.01;
    doublePropertyType.mdMax = 1000.0;
    propId = "rho_step";
    auto propRhoStep = std::make_shared<TypedProperty<DoublePropertyType>>("Rho Step", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propRhoStep);
    mMapIdToProperty[propId] = propRhoStep;

    // Theta range (degrees)
    doublePropertyType.mdValue = mParams.mdMinThetaDeg;
    doublePropertyType.mdMin = 0.0;
    doublePropertyType.mdMax = 180.0;
    propId = "min_theta";
    auto propMinTheta = std::make_shared<TypedProperty<DoublePropertyType>>("Min Theta (deg)", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propMinTheta);
    mMapIdToProperty[propId] = propMinTheta;

    doublePropertyType.mdValue = mParams.mdMaxThetaDeg;
    doublePropertyType.mdMin = 0.0;
    doublePropertyType.mdMax = 180.0;
    propId = "max_theta";
    auto propMaxTheta = std::make_shared<TypedProperty<DoublePropertyType>>("Max Theta (deg)", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propMaxTheta);
    mMapIdToProperty[propId] = propMaxTheta;

    doublePropertyType.mdValue = mParams.mdThetaStepDeg;
    doublePropertyType.mdMin = 0.01;
    doublePropertyType.mdMax = 180.0;
    propId = "theta_step";
    auto propThetaStep = std::make_shared<TypedProperty<DoublePropertyType>>("Theta Step (deg)", propId, QMetaType::Double, doublePropertyType, "Operation");
    mvProperty.push_back(propThetaStep);
    mMapIdToProperty[propId] = propThetaStep;

    // Display lines toggle
    propId = "display_lines";
    auto propDisplayLines = std::make_shared<TypedProperty<bool>>("Display Lines", propId, QMetaType::Bool, mParams.mbDisplayLines, "Display");
    mvProperty.push_back(propDisplayLines);
    mMapIdToProperty[propId] = propDisplayLines;

    // Color B,G,R
    for (int i = 0; i < 3; i++)
    {
        ucharPropertyType.mucValue = mParams.mucLineColor[i];
        propId = QString("line_color_%1").arg(i);
        auto propLineColor = std::make_shared<TypedProperty<UcharPropertyType>>(QString::fromStdString("Line Color ") + QString::fromStdString(color[i]), propId, QMetaType::Int, ucharPropertyType, "Display");
        mvProperty.push_back(propLineColor);
        mMapIdToProperty[propId] = propLineColor;
    }

    // Thickness
    intPropertyType.miValue = mParams.miLineThickness;
    intPropertyType.miMin = 1;
    intPropertyType.miMax = 32;
    propId = "line_thickness";
    auto propLineThickness = std::make_shared<TypedProperty<IntPropertyType>>("Line Thickness", propId, QMetaType::Int, intPropertyType, "Display");
    mvProperty.push_back(propLineThickness);
    mMapIdToProperty[propId] = propLineThickness;

    // Line type
    EnumPropertyType enumType;
    enumType.mslEnumNames = QStringList({"LINE_8", "LINE_4", "LINE_AA"});
    enumType.miCurrentIndex = (mParams.miLineType == cv::LINE_AA ? 2 : (mParams.miLineType == cv::LINE_4 ? 1 : 0));
    propId = "line_type";
    auto propLineType = std::make_shared<TypedProperty<EnumPropertyType>>("Line Type", propId, QtVariantPropertyManager::enumTypeId(), enumType, "Display");
    mvProperty.push_back(propLineType);
    mMapIdToProperty[propId] = propLineType;

    // Strongest N Only (cap display to Lines Max)
    propId = "strongest_only";
    auto propStrongest = std::make_shared<TypedProperty<bool>>("Strongest N Only", propId, QMetaType::Bool, mParams.mbStrongestOnly, "Display");
    mvProperty.push_back(propStrongest);
    mMapIdToProperty[propId] = propStrongest;

    qRegisterMetaType<CVHoughLinesPointSetParams>("CVHoughLinesPointSetParams");
}

QJsonObject CVHoughLinesPointSetModel::save() const
{
    QJsonObject modelJson = PBAsyncDataModel::save();

    QJsonObject cParams;
    cParams["linesMax"] = mParams.miLinesMax;
    cParams["threshold"] = mParams.miThreshold;
    cParams["minRho"] = mParams.mdMinRho;
    cParams["maxRho"] = mParams.mdMaxRho;
    cParams["rhoStep"] = mParams.mdRhoStep;
    cParams["minThetaDeg"] = mParams.mdMinThetaDeg;
    cParams["maxThetaDeg"] = mParams.mdMaxThetaDeg;
    cParams["thetaStepDeg"] = mParams.mdThetaStepDeg;
    cParams["displayLines"] = mParams.mbDisplayLines;
    cParams["strongestOnly"] = mParams.mbStrongestOnly;
    for (int i = 0; i < 3; ++i)
        cParams[QString("lineColor%1").arg(i)] = mParams.mucLineColor[i];
    cParams["lineThickness"] = mParams.miLineThickness;
    cParams["lineType"] = mParams.miLineType;

    modelJson["cParams"] = cParams;
    return modelJson;
}

void CVHoughLinesPointSetModel::load(const QJsonObject &p)
{
    PBAsyncDataModel::load(p);

    QJsonObject paramsObj = p["cParams"].toObject();
    if (paramsObj.isEmpty())
        return;

    QJsonValue v;
    v = paramsObj["linesMax"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["lines_max"];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = v.toInt();
        mParams.miLinesMax = v.toInt();
    }
    v = paramsObj["threshold"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["threshold"];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = v.toInt();
        mParams.miThreshold = v.toInt();
    }
    v = paramsObj["minRho"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["min_rho"];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = v.toDouble();
        mParams.mdMinRho = v.toDouble();
    }
    v = paramsObj["maxRho"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["max_rho"];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = v.toDouble();
        mParams.mdMaxRho = v.toDouble();
    }
    v = paramsObj["rhoStep"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["rho_step"];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = v.toDouble();
        mParams.mdRhoStep = v.toDouble();
    }
    v = paramsObj["minThetaDeg"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["min_theta"];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = v.toDouble();
        mParams.mdMinThetaDeg = v.toDouble();
    }
    v = paramsObj["maxThetaDeg"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["max_theta"];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = v.toDouble();
        mParams.mdMaxThetaDeg = v.toDouble();
    }
    v = paramsObj["thetaStepDeg"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["theta_step"];
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = v.toDouble();
        mParams.mdThetaStepDeg = v.toDouble();
    }
    v = paramsObj["displayLines"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["display_lines"];
        auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typed->getData() = v.toBool();
        mParams.mbDisplayLines = v.toBool();
    }
    v = paramsObj["strongestOnly"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["strongest_only"];
        auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typed->getData() = v.toBool();
        mParams.mbStrongestOnly = v.toBool();
    }
    for (int i = 0; i < 3; ++i)
    {
        v = paramsObj[QString("lineColor%1").arg(i)];
        if (!v.isNull())
        {
            auto prop = mMapIdToProperty[QString("line_color_%1").arg(i)];
            auto typed = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
            typed->getData().mucValue = v.toInt();
            mParams.mucLineColor[i] = v.toInt();
        }
    }
    v = paramsObj["lineThickness"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["line_thickness"];
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = v.toInt();
        mParams.miLineThickness = v.toInt();
    }
    v = paramsObj["lineType"];
    if (!v.isNull())
    {
        auto prop = mMapIdToProperty["line_type"];
        auto typed = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typed->getData().miCurrentIndex = v.toInt();
        mParams.miLineType = v.toInt();
    }
}

void CVHoughLinesPointSetModel::setModelProperty(QString &id, const QVariant &value)
{
    if (!mMapIdToProperty.contains(id))
    {
        PBAsyncDataModel::setModelProperty(id, value);
        return;
    }

    auto prop = mMapIdToProperty[id];

    if (id == "lines_max")
    {
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miLinesMax = value.toInt();
    }
    else if (id == "threshold")
    {
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miThreshold = value.toInt();
    }
    else if (id == "min_rho")
    {
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdMinRho = value.toDouble();
    }
    else if (id == "max_rho")
    {
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdMaxRho = value.toDouble();
    }
    else if (id == "rho_step")
    {
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdRhoStep = value.toDouble();
    }
    else if (id == "min_theta")
    {
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdMinThetaDeg = value.toDouble();
    }
    else if (id == "max_theta")
    {
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdMaxThetaDeg = value.toDouble();
    }
    else if (id == "theta_step")
    {
        auto typed = std::static_pointer_cast<TypedProperty<DoublePropertyType>>(prop);
        typed->getData().mdValue = value.toDouble();
        mParams.mdThetaStepDeg = value.toDouble();
    }
    else if (id == "display_lines")
    {
        auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typed->getData() = value.toBool();
        mParams.mbDisplayLines = value.toBool();
    }
    else if (id == "strongest_only")
    {
        auto typed = std::static_pointer_cast<TypedProperty<bool>>(prop);
        typed->getData() = value.toBool();
        mParams.mbStrongestOnly = value.toBool();
    }
    else if (id.startsWith("line_color_"))
    {
        auto typed = std::static_pointer_cast<TypedProperty<UcharPropertyType>>(prop);
        typed->getData().mucValue = value.toInt();
        int idx = id.mid(11).toInt();
        if (idx >= 0 && idx < 3)
            mParams.mucLineColor[idx] = value.toInt();
    }
    else if (id == "line_thickness")
    {
        auto typed = std::static_pointer_cast<TypedProperty<IntPropertyType>>(prop);
        typed->getData().miValue = value.toInt();
        mParams.miLineThickness = value.toInt();
    }
    else if (id == "line_type")
    {
        auto typed = std::static_pointer_cast<TypedProperty<EnumPropertyType>>(prop);
        typed->getData().miCurrentIndex = value.toInt();
        switch (value.toInt())
        {
        case 0:
            mParams.miLineType = cv::LINE_8;
            break;
        case 1:
            mParams.miLineType = cv::LINE_4;
            break;
        case 2:
            mParams.miLineType = cv::LINE_AA;
            break;
        }
    }
    else
    {
        PBAsyncDataModel::setModelProperty(id, value);
        return;
    }

    if (mpCVImageInData && !isShuttingDown())
        process_cached_input();
}

unsigned int CVHoughLinesPointSetModel::nPorts(PortType portType) const
{
    if (portType == PortType::In)
        return 2; // image + sync
    if (portType == PortType::Out)
        return 3; // image + count + sync
    return 0;
}

NodeDataType CVHoughLinesPointSetModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out)
    {
        if (portIndex == 0)
            return CVImageData().type();
        if (portIndex == 1)
            return IntegerData().type();
        if (portIndex == 2)
            return SyncData().type();
    }
    else if (portType == PortType::In)
    {
        if (portIndex == 0)
            return CVImageData().type();
        if (portIndex == 1)
            return SyncData().type();
    }
    return NodeDataType();
}

std::shared_ptr<NodeData> CVHoughLinesPointSetModel::outData(PortIndex port)
{
    if (port == 0)
        return mpCVImageData;
    if (port == 1)
        return mpIntegerData;
    if (port == 2)
        return mpSyncData;
    return nullptr;
}

QObject *CVHoughLinesPointSetModel::createWorker() { return new CVHoughLinesPointSetWorker(); }

void CVHoughLinesPointSetModel::connectWorker(QObject *worker)
{
    auto *w = qobject_cast<CVHoughLinesPointSetWorker *>(worker);
    if (w)
    {
        connect(w, &CVHoughLinesPointSetWorker::frameReady, this, [this](std::shared_ptr<CVImageData> img, std::shared_ptr<IntegerData> count)
                { mpCVImageData=img; mpIntegerData=count; Q_EMIT dataUpdated(0); Q_EMIT dataUpdated(1); mpSyncData->data()=true; Q_EMIT dataUpdated(2); setWorkerBusy(false); dispatchPendingWork(); }, Qt::QueuedConnection);
    }
}

void CVHoughLinesPointSetModel::dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown())
        return;
    cv::Mat input = mPendingFrame;
    CVHoughLinesPointSetParams params = mPendingParams;
    setPendingWork(false);
    ensure_frame_pool(input.cols, input.rows, CV_8UC3);
    long frameId = getNextFrameId();
    QString producerId = getNodeId();
    double minTheta = params.mdMinThetaDeg * CV_PI / 180.0;
    double maxTheta = params.mdMaxThetaDeg * CV_PI / 180.0;
    double thetaStep = params.mdThetaStepDeg * CV_PI / 180.0;
    QMetaObject::invokeMethod(mpWorker, "processFrame", Qt::QueuedConnection,
                              Q_ARG(cv::Mat, input),
                              Q_ARG(CVHoughLinesPointSetParams, params),
                              Q_ARG(FrameSharingMode, getSharingMode()),
                              Q_ARG(std::shared_ptr<CVImagePool>, getFramePool()),
                              Q_ARG(long, frameId),
                              Q_ARG(QString, producerId));
    setWorkerBusy(true);
}

void CVHoughLinesPointSetModel::process_cached_input()
{
    if (!mpCVImageInData)
        return;
    mPendingFrame = mpCVImageInData->data().clone();
    mPendingParams = mParams;
    if (!mpWorker)
    {
        mpWorker = createWorker();
        mpWorker->moveToThread(&mWorkerThread);
        mWorkerThread.start();
        connectWorker(mpWorker);
    }
    if (hasPendingWork() || isWorkerBusy())
    {
        setPendingWork(true);
        return;
    }
    setPendingWork(true);
    dispatchPendingWork();
}
