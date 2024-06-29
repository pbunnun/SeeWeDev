To compile CVDev, you need ...


MS Windows: tested with

    - Windows 10 64bits Home and Pro.
    - Visual Studio 2017 any edition.
    - Qt 5.14.2 Opensouce Version. Need only a 64bits version compiled by VS2017 and Qt Creator which is a preferred IDE.
    - OpenCV 4.3.0, 4.5.2 uncompressed and put its directory "opencv" in C:.

    - Windows 11 64bits
    - Visual Studio Community 2019
    - Qt 5.15.4 Opensource Version. Need only a 64bits version compiled by VS2019 and Qt Creator which is a preferred IDE.
    - OpenCV 4.5.5
    - vcpkg is used to compile required libraries.
        - vcpkg.exe install qt5-serialbus qt5-winextras
        - vcpkg.exe install opencv4[contrib,dnn,eigen,ffmpeg,ipp,jasper,jpeg,lapack,nonfree,opengl,openmp,png,qt,sfm,tbb,quirc,tiff,webp,cuda,cudnn] --recurse
        
        cuda and cudnn flags may be removed.

    - Windows 10 64bits
    - Visual Studio Community 2022
    - Qt 6.7.2
    - OpenCV 4.8.0
    - vcpkg is used to compile required libraries.
        - vcpkg.exe install opencv4[contrib,dnn,eigen,ffmpeg,ipp,jasper,jpeg,lapack,nonfree,opengl,png,qt,sfm,tbb,quirc,tiff,webp,cuda,cudnn] --recurse
        
        cuda and cudnn flags may be removed.


macOS: tested with

    - macOS 10.15.5, 11.3
    - Homebrew
        - Qt 5.15.0, Qt 5.15.2
        - OpenCV 4.3.0, 4.5.2
    - Before running the software inside Qt Creator, the Add build library search path to DYLD_LIBRARY_PATH and DYLD_FRAMEWORK_PATH option from the Run section in the Projects tab has to be unchecked.

Linux: tested with

    - Ubuntu 18.04, 20.04
    
    - Raspberry Pi OS (64-bit)

