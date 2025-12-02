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

#pragma once

#include <QtCore/QObject>

#include "PBAsyncDataModel.hpp"
#include "CVImageData.hpp"
#include "CVImagePool.hpp"
#include "SyncData.hpp"

using CVDevLibrary::FrameSharingMode;
using CVDevLibrary::CVImagePool;

// Parameters for global histogram equalization
typedef struct CVHistogramEqualizationParameters {
    bool mbApplyColorLuma { true }; // If true and image is color, equalize luminance channel
    int  miColorSpaceIndex { 0 };    // 0=YCrCb, 1=Lab
    bool mbConvertTo8Bit { false };  // Normalize & convert non 8U before equalization
} CVHistogramEqualizationParameters;

class CVHistogramEqualizationWorker : public QObject
{
    Q_OBJECT
public:
    CVHistogramEqualizationWorker() {}

public Q_SLOTS:
    void processFrame(cv::Mat input,
                      bool applyColorLuma,
                      int colorSpaceIndex,
                      bool convertTo8Bit,
                      FrameSharingMode mode,
                      std::shared_ptr<CVImagePool> pool,
                      long frameId,
                      QString producerId);

Q_SIGNALS:
    void frameReady(std::shared_ptr<CVImageData> img);
};

class CVHistogramEqualizationModel : public PBAsyncDataModel
{
    Q_OBJECT
public:
    CVHistogramEqualizationModel();
    ~CVHistogramEqualizationModel() override = default;

    QJsonObject save() const override;
    void load(const QJsonObject &p) override;

    QWidget * embeddedWidget() override { return nullptr; }
    void setModelProperty(QString & id, const QVariant & value) override;
    QPixmap minPixmap() const override { return _minPixmap; }

    static const QString _category;
    static const QString _model_name;

protected:
    QObject* createWorker() override;
    void connectWorker(QObject* worker) override;
    void dispatchPendingWork() override;

private:
    void process_cached_input() override;

    CVHistogramEqualizationParameters mParams;
    QPixmap _minPixmap;

    cv::Mat mPendingFrame;
    CVHistogramEqualizationParameters mPendingParams;
};
