//Copyright © 2026, NECTEC, all rights reserved

#include "ViewSettingsCommand.hpp"
#include "MainWindow.hpp"

ViewSettingsCommand::ViewSettingsCommand(MainWindow* mainWindow, ViewSettingType type, bool oldValue, bool newValue, QUndoCommand* parent)
    : QUndoCommand(parent)
    , mMainWindow(mainWindow)
    , mType(type)
    , mOldValue(oldValue)
    , mNewValue(newValue)
{
    switch (mType) {
        case CategoryVisible:
            setText(QObject::tr("Toggle Category Panel"));
            break;
        case WorkspaceVisible:
            setText(QObject::tr("Toggle Workspace Panel"));
            break;
        case PropertiesVisible:
            setText(QObject::tr("Toggle Properties Panel"));
            break;
        case FocusView:
            setText(QObject::tr("Toggle Focus View"));
            break;
        case FullScreen:
            setText(QObject::tr("Toggle Full Screen"));
            break;
        case SnapToGrid:
            setText(QObject::tr("Toggle Snap to Grid"));
            break;
    }
}

void ViewSettingsCommand::undo()
{
    if (mMainWindow) {
        mMainWindow->applyViewSetting(mType, mOldValue);
    }
}

void ViewSettingsCommand::redo()
{
    if (mMainWindow) {
        mMainWindow->applyViewSetting(mType, mNewValue);
    }
}
