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
add_subdirectory(Main)

add_subdirectory(Plugins)
add_subdirectory(CVDevCommunityPlugins)

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

# Bundle the already-built Windows application folder (build-time CVDev dir)
# into the NSIS installer, without affecting Linux DEB packaging.
if(WIN32)
    # Install the entire built CVDev directory as the Runtime component root.
    # This captures any manual edits the user made inside build/CVDev before packaging.
    install(DIRECTORY "${CMAKE_BINARY_DIR}/CVDev/" DESTINATION . COMPONENT Runtime)

    # Prepare a combined license file for NSIS License page
    set(CPACK_LICENSE_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/CPackLicense.txt")
    file(WRITE "${CPACK_LICENSE_OUTPUT}" "See-We-Dev bundled licenses\n================================\n\n")
    if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
        file(APPEND "${CPACK_LICENSE_OUTPUT}" "---- CVDev & CVDevLibrary (Apache-2.0) ----\n\n")
        file(READ "${CMAKE_SOURCE_DIR}/LICENSE" _apache_text)
        file(APPEND "${CPACK_LICENSE_OUTPUT}" "${_apache_text}\n\n")
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/NodeEditor/LICENSE.rst")
        file(APPEND "${CPACK_LICENSE_OUTPUT}" "---- QtNodes (BSD-3-Clause) ----\n\n")
        file(READ "${CMAKE_SOURCE_DIR}/NodeEditor/LICENSE.rst" _bsd_text)
        file(APPEND "${CPACK_LICENSE_OUTPUT}" "${_bsd_text}\n\n")
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/QtPropertyBrowserLibrary/LICENSES/LGPL-3.0-only.txt")
        file(APPEND "${CPACK_LICENSE_OUTPUT}" "---- QtPropertyBrowserLibrary (LGPL-3.0) ----\n\n")
        file(READ "${CMAKE_SOURCE_DIR}/QtPropertyBrowserLibrary/LICENSES/LGPL-3.0-only.txt" _lgpl_text)
        file(APPEND "${CPACK_LICENSE_OUTPUT}" "${_lgpl_text}\n\n")
    endif()
    set(CPACK_RESOURCE_FILE_LICENSE "${CPACK_LICENSE_OUTPUT}")

    # Add shortcuts for Start Menu and Desktop to the installed CVDev.exe
    # Since we install the entire built CVDev directory to the installer root,
    # the executable path is just "CVDev.exe" relative to install prefix.
    set(CPACK_NSIS_MENU_LINKS
        "CVDev.exe" "CVDev"
    )
    set(CPACK_CREATE_DESKTOP_LINKS
        "CVDev.exe"
    )

    # For NSIS on Windows, do not present the Development component.
    # Only install the Runtime content.
    set(CPACK_COMPONENTS_ALL Runtime)
endif()

# Enable both DEB (Linux) and NSIS (Windows) generators. Users can still
# select a specific generator with `cpack -G <GEN>`.
set(CPACK_GENERATOR "DEB;NSIS")
set(CPACK_DEB_COMPONENT_INSTALL ON)

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

