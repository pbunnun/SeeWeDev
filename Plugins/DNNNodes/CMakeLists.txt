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

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

# =========================================================
# 📌 ตั้งค่า NCNN
# =========================================================
set(NCNN_INC "/home/kawtom/ncnn/build_final/install/include")
set(NCNN_INC_NCNN "/home/kawtom/ncnn/build_final/install/include/ncnn")
set(NCNN_LIB "/home/kawtom/ncnn/build_final/install/lib")

include_directories(${NCNN_INC} ${NCNN_INC_NCNN})
link_directories(${NCNN_LIB})

# =========================================================
# 📌 ตั้งค่า MNN
# =========================================================
# ชี้ไปที่โฟลเดอร์ไลบรารี MNN ที่เราสร้างไว้
set(MNN_DIR "/home/kawtom/MNN_Library")
# เพิ่มที่อยู่ Header ของ MNN
include_directories(${MNN_DIR}/include)
# (ลบ target_link_libraries ของ MNN ตรงนี้ออก แล้วย้ายไปรวมด้านล่าง)


file(GLOB CPP_FILES *.cpp)
file(GLOB HPP_FILES *.hpp)
file(GLOB UI_FILES *.ui)

# >>> สร้าง Target ชื่อ DNNNodes ตรงนี้ <<<
add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}
    ${UI_FILES}
    resources/dnn_resources.qrc
)

# บังคับ MOC ให้รู้จัก NCNN อย่างเด็ดขาด
target_include_directories(${PROJECT_NAME} PRIVATE ${NCNN_INC} ${NCNN_INC_NCNN})

set_target_properties(${PROJECT_NAME}
                      PROPERTIES
                      POSITION_INDEPENDENT_CODE ON
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/
                      )

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED -DQT_NO_KEYWORDS)

include_directories(${CVDevLibrary_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/QtPropertyBrowserLibrary/qtpropertybrowser)
include_directories(${CMAKE_SOURCE_DIR}/QtPropertyBrowserLibrary)

# =========================================================
# 📌 รวบรวมไลบรารีทั้งหมดมาลิงก์เข้ากับโปรเจกต์ที่นี่
# =========================================================
target_link_libraries(${PROJECT_NAME}
    CVDevLibrary
    ${OpenCV_LIBS}
    ncnn
    ${MNN_DIR}/lib/libMNN.so 
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