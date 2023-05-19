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

#include "MainWindow.hpp"

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0) )
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/cvdev-64.png"));

    MainWindow window;
    window.show();
    return app.exec();
}
