cmake_minimum_required(VERSION 3.10)

project(CVDev)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

if( "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
    message(SEND_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)

if( WIN32 )
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

add_subdirectory(NodeEditor)
add_subdirectory(QtPropertyBrowserLibrary)
add_subdirectory(CVDevLibrary)
add_subdirectory(CVDev)

add_subdirectory(Plugins)

# -------------------------------------------------------------------
# CPack Configuration for DEB Package Generation
# -------------------------------------------------------------------
# Set general package information.
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION "0.7b")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "See-We-Dev -- Flow-Based Visual Programming for OpenCV")
set(CPACK_PACKAGE_VENDOR "NECTEC")
set(CPACK_PACKAGE_CONTACT "pished.bunnun@nectec.or.th")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/pbunnun/SeeWeDev")

# Specify the CPack generator for Debian packages.
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_GENERATOR "DEB")

# Specify runtime dependencies. This is a crucial step.
# List all required packages, for example: "libc6 (>= 2.17), libqt5widgets5 (>= 5.15.2)"
set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS "libc6 (>= 2.35), libqt6widgets6, libqt6gui6, libqt6core6, libopencv-core406t64, libopencv-imgproc406t64")
# --- Configuration for the DEVELOPMENT package ---
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_DEPENDS "libopencv-dev, qt6-base-dev, qt6-base-dev-tools, qt6-base-private-dev, qt6-tools-dev, qt6-tools-dev-tools")

include(CPack)
# Define the installation components
# cpack_add_component(<componentName> DISPLAY_NAME <displayName> [DESCRIPTION <description>] [DEPENDS <other_components>])
cpack_add_component(Runtime 
    DISPLAY_NAME "Runtime" 
    DESCRIPTION "Application executable and libraries"
    REQUIRED
)

cpack_add_component(Development 
    DISPLAY_NAME "Development" 
    DESCRIPTION "Header files for development" 
    DEPENDS Runtime
)

