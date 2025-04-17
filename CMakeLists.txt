cmake_minimum_required(VERSION 3.14)
project(deepin-screensaver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置安装路径
if (NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()
set(LIB_PATH "${CMAKE_INSTALL_PREFIX}/lib/deepin-screensaver" CACHE PATH "Library installation path")
set(MODULE_PATH "${LIB_PATH}/modules" CACHE PATH "Module installation path")
set(RESOURCE_PATH "${LIB_PATH}/resources" CACHE PATH "Resource installation path")
set(QT_VERSION_MAJOR "6" CACHE STRING "Build with qt version major")

add_definitions(-DLIB_PATH="${LIB_PATH}" -DMODULE_PATH="${MODULE_PATH}" -DRESOURCE_PATH="${RESOURCE_PATH}")

# 设置模块路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# 引入翻译生成
include(translation-generate)

if (QT_VERSION_MAJOR STREQUAL "6")
    set(QT_VERSION_MAJOR "6")
    set(QT_DESIRED_VERSION "6")
    set(DTK_VERSION_MAJOR "6")
else()
    set(QT_VERSION_MAJOR "5")
    set(QT_DESIRED_VERSION "5")
    set(DTK_VERSION_MAJOR "")
endif()
message("   >>> Build with DTK: ${DTK_VERSION_MAJOR}")

# 添加子目录
add_subdirectory(tools)
add_subdirectory(src) 
add_subdirectory(xscreensaver)
add_subdirectory(customscreensaver)
