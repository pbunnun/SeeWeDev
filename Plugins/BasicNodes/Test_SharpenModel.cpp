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

#include "Test_SharpenModel.hpp"

#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QDebug>

#include <QtWidgets/QFileDialog>

#include <nodes/DataModelRegistry>

#include "CVImageData.hpp"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

Test_SharpenModel::Test_SharpenModel()
    : PBNodeDataModel(_model_name),
      _minPixmap(":Test_Sharpen.png")
{
    mpCVImageData = std::make_shared<CVImageData>(cv::Mat());
}

Test_SharpenModel::~Test_SharpenModel()
{}

unsigned int Test_SharpenModel::nPorts(PortType PortType) const
{
    unsigned int result = 1;
    switch(PortType)
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

NodeDataType Test_SharpenModel::dataType(PortType , PortIndex) const
{
    return CVImageData().type();
}

std::shared_ptr<NodeData> Test_SharpenModel::outData(PortIndex)
{
    if( isEnable() && mpCVImageData->image().data != nullptr )
    {
        return mpCVImageData;
    }
    return nullptr;
}

void Test_SharpenModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex)
{
    if( !isEnable() )
    {
        return;
    }
    if( nodeData )
    {
        auto d= std::dynamic_pointer_cast<CVImageData>(nodeData);
        if(d && d->image().type()==CV_8UC3)
        {
            //mpCVImageInData = d;
            cv::Mat CVTestSharpenImage = d->image().clone();
            uint row = CVTestSharpenImage.rows;
            uint col = CVTestSharpenImage.cols*CVTestSharpenImage.channels();
            for(uint i=1; i<row-1; i++)
            {
                uchar *pu = CVTestSharpenImage.ptr<uchar>(i-1);
                uchar *pm = CVTestSharpenImage.ptr<uchar>(i);
                uchar *pl = CVTestSharpenImage.ptr<uchar>(i+1);
                for(uint j=1; j<col-1; j++)
                {
                    pm[j] = cv::saturate_cast<uchar>(-1*pu[j-1] -1*pu[j] -1*pu[j+1]
                                                     -1*pm[j-1] +9*pm[j] -1*pm[j+1]
                                                     -1*pl[j-1] -1*pl[j] -1*pl[j+1]
                                                     );
                }
            }
            //code for borders
            mpCVImageData->set_image(CVTestSharpenImage);
        }
        else
        {
            mpCVImageData->set_image(d->image());
        }
    }
    Q_EMIT dataUpdated(0);
}

const QString Test_SharpenModel::_category = QString("Template Category");
const QString Test_SharpenModel::_model_name = QString("Test_Sharpen");
