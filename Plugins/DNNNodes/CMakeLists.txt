cmake_minimum_required(VERSION 3.10)

project(DNNNodes LANGUAGES CXX)

if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE Release)
endif()

if( NOT WIN32 )
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17) # แนะนำให้ใช้ 17 ตามมาตรฐานโปรเจกต์เดิม
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

# =========================================================
# 📌 ตั้งค่า NCNN และ MNN แบบ Portable (อ้างอิงภายในโปรเจกต์)
# =========================================================

# กำหนด Root ของ Library (ถอยกลับไป 2 ชั้นจาก Plugins/DNNNodes)
set(NCNN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdpartyncnn")
set(MNN_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdpartymnn")

# 1. ตั้งค่า Include Paths
set(NCNN_INC      "${NCNN_DIR}/include")
set(NCNN_INC_NCNN "${NCNN_DIR}/include/ncnn")
set(MNN_INC       "${MNN_DIR}/include")

include_directories(
    ${NCNN_INC} 
    ${NCNN_INC_NCNN} 
    ${MNN_INC}
    ${MNN_INC}/MNN # เผื่อกรณี #include <MNN/Interpreter.hpp>
)

# 2. ตั้งค่า Library Paths (.so files)
# สมมติว่าเก็บไฟล์ .so ไว้ในโฟลเดอร์ lib/ ภายในของแต่ละตัว
set(NCNN_LIBRARY "${NCNN_DIR}/lib/libncnn.so.1") # หรือ .so
set(MNN_LIBRARY  "${MNN_DIR}/lib/libMNN.so")

# =========================================================

file(GLOB CPP_FILES *.cpp)
file(GLOB HPP_FILES *.hpp)
file(GLOB UI_FILES *.ui)

add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}
    ${UI_FILES}
    resources/dnn_resources.qrc
)

# บังคับ MOC ให้รู้จัก Include Paths
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${NCNN_INC} 
    ${NCNN_INC_NCNN} 
    ${MNN_INC}
)

set_target_properties(${PROJECT_NAME}
                      PROPERTIES
                      POSITION_INDEPENDENT_CODE ON
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      )

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED -DQT_NO_KEYWORDS)

include_directories(${CVDevLibrary_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/QtPropertyBrowserLibrary/qtpropertybrowser)
include_directories(${CMAKE_SOURCE_DIR}/QtPropertyBrowserLibrary)

# =========================================================
# 📌 ลิงก์ไลบรารีทั้งหมด
# =========================================================
target_link_libraries(${PROJECT_NAME}
    CVDevLibrary
    ${OpenCV_LIBS}
    ${NCNN_LIBRARY} # ลิงก์ตรงไปที่ไฟล์ใน 3rdpartyncnn
    ${MNN_LIBRARY}  # ลิงก์ตรงไปที่ไฟล์ใน 3rdpartymnn
    Threads::Threads
    OpenMP::OpenMP_CXX
)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Moving $<TARGET_FILE_NAME:${PROJECT_NAME}> to ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins"
        COMMAND ${CMAKE_COMMAND} -E rename
                "${CMAKE_BINARY_DIR}/CVDev/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
                "${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Move ${PROJECT_NAME} into cvdev_plugins"
        VERBATIM)
endif()

if( NOT WIN32 )
    install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION bin/cvdev_plugins
        COMPONENT Runtime
    )
endif()