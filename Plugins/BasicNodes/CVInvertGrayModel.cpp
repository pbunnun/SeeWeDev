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

#include "CVInvertGrayModel.hpp"
#include "CVImageData.hpp"
#include <opencv2/imgproc.hpp>
#include <QMetaObject>
#include <QTimer>

const QString CVInvertGrayModel::_category = QString("Image Conversion");
const QString CVInvertGrayModel::_model_name = QString("CV Invert Grayscale");

CVInvertGrayModel::CVInvertGrayModel()
    : PBAsyncDataModel(_model_name),
      _minPixmap(":CVInvertGray.png")
{
}

QObject* CVInvertGrayModel::createWorker()
{
    return new CVInvertGrayWorker();
}

void CVInvertGrayModel::connectWorker(QObject* worker)
{
    auto w = qobject_cast<CVInvertGrayWorker*>(worker);
    if (w)
    {
        connect(w, &CVInvertGrayWorker::frameReady,
                this, &CVInvertGrayModel::handleFrameReady);
    }
}

void CVInvertGrayModel::dispatchPendingWork()
{
    if (!hasPendingWork() || isShuttingDown() || mPendingFrame.empty())
        return;

    cv::Mat input = mPendingFrame;
    setPendingWork(false);

    setWorkerBusy(true);
    QMetaObject::invokeMethod(mpWorker, "processFrame",
                            Qt::QueuedConnection,
                            Q_ARG(cv::Mat, input.clone()));
}

void CVInvertGrayModel::process_cached_input()
{
    if (!mpCVImageInData || mpCVImageInData->data().empty())
        return;

    cv::Mat input = mpCVImageInData->data();
    
    if (input.channels() != 1)
        return;
    
    // Emit sync "false" signal in next event loop
    QTimer::singleShot(0, this, [this]() {
        mpSyncData->data() = false;
        Q_EMIT dataUpdated(1);
    });

    if (isWorkerBusy())
    {
        // Store as pending - will be processed when worker finishes
        mPendingFrame = input.clone();
        setPendingWork(true);
    }
    else
    {
        setWorkerBusy(true);
        QMetaObject::invokeMethod(mpWorker, "processFrame",
                                Qt::QueuedConnection,
                                Q_ARG(cv::Mat, input.clone()));
    }
}



