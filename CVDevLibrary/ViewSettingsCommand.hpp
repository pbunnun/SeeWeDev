//Copyright © 2026, NECTEC, all rights reserved

#pragma once

#include "CVDevLibrary.hpp"
#include <QUndoCommand>

class MainWindow;

enum ViewSettingType {
    CategoryVisible,
    WorkspaceVisible,
    PropertiesVisible,
    FocusView,
    FullScreen,
    SnapToGrid
};

class CVDEVSHAREDLIB_EXPORT ViewSettingsCommand : public QUndoCommand
{
public:
    ViewSettingsCommand(MainWindow* mainWindow, ViewSettingType type, bool oldValue, bool newValue, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    MainWindow* mMainWindow;
    ViewSettingType mType;
    bool mOldValue;
    bool mNewValue;
};
