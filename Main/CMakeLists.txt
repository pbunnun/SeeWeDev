cmake_minimum_required(VERSION 3.10)

project(CVDev LANGUAGES CXX)

if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

file(GLOB CPP_FILES *.cpp)

if( WIN32 )
    set(APP_ICON_RESOURCE_WINDOWS "resources/cvdev_resources.rc")
    add_executable(${PROJECT_NAME}
        WIN32
        ${CPP_FILES}
        ${APP_ICON_RESOURCE_WINDOWS}
        resources/cvdev_resources.qrc
        )
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    add_executable(${PROJECT_NAME}
        ${CPP_FILES}
        resources/cvdev_resources.qrc
    )
endif()

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED)

include_directories(${CVDevLibrary_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} CVDevLibrary)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins)
# Place the built executable into the working application folder
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/
)

if(WIN32)
    # Try to locate windeployqt. Prefer PATH but also check common Qt layout near Qt6_DIR.
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt.exe windeployqt
        HINTS "$ENV{QT_HOME}/bin"
                    "${Qt6_DIR}/../bin"
                    "${Qt6_DIR}/../../bin"
        PATHS $ENV{PATH}
    )

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME}
                                             POST_BUILD
                                             COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:${PROJECT_NAME}>"
                                             WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                                             COMMENT "Running windeployqt on $<TARGET_FILE:${PROJECT_NAME}>"
                                             VERBATIM)
    else()
        message(WARNING "windeployqt not found. If you want automatic Qt deployment, make sure windeployqt is in PATH or set QT_HOME.")
    endif()
else()
    # ---- ADD THIS SECTION FOR CPACK ----
    # Install the executable target. This tells CPack to include the CVDev
    # binary in the package.
    # 'RUNTIME DESTINATION bin' installs the executable to the standard '/usr/local/bin'
    # directory on the target system.
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        COMPONENT Runtime
    )
    # Install the desktop entry file for application menus
    install(FILES CVDev.desktop
        DESTINATION share/applications
        COMPONENT Runtime
    )
    # Install the application icon
    install(FILES resources/cvdev-64.png
        DESTINATION share/icons/hicolor/scalable/apps
        COMPONENT Runtime
    )
# ------------------------------------
endif()

