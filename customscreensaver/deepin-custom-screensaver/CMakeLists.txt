cmake_minimum_required(VERSION 3.14)

project(deepin-custom-screensaver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

# 查找 Qt 模块
set(qt_required_components Core Gui Widgets Network)

if (QT_DESIRED_VERSION MATCHES 6)
    list(APPEND qt_required_components Core5Compat)
endif()

find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS ${qt_required_components})
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Gui Core)

# 查找系统库
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)

# 源文件
set(SOURCES
    src/main.cpp
    src/slideshowscreensaver.cpp
    src/commandlinemanager.cpp
    src/slideshowconfig.cpp
    src/slideshowconfigdialog.cpp
    src/config/contenttitle.cpp
    src/config/selectpathwidget.cpp
    src/config/truncatelineedit.cpp
    src/config/timeintervalwidget.cpp
    src/singleinstance.cpp
)

# 头文件
set(HEADERS
    src/slideshowscreensaver.h
    src/commandlinemanager.h
    src/slideshowconfig.h
    src/slideshowconfigdialog.h
    src/config/contenttitle.h
    src/config/selectpathwidget.h
    src/config/truncatelineedit.h
    src/config/timeintervalwidget.h
    src/singleinstance.h
)

# 资源文件
set(RESOURCES
    icon.qrc
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

# 链接库
set(LINK_LIBS
    Qt${QT_DESIRED_VERSION}::Core
    Qt${QT_DESIRED_VERSION}::Gui
    Qt${QT_DESIRED_VERSION}::Widgets
    Qt${QT_DESIRED_VERSION}::Network
    Dtk${DTK_VERSION_MAJOR}::Widget
    Dtk${DTK_VERSION_MAJOR}::Gui
    Dtk${DTK_VERSION_MAJOR}::Core
    ${X11_LIBRARIES}
)

if (QT_DESIRED_VERSION MATCHES 6)
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::Core5Compat)
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

# 安装目标
install(TARGETS ${PROJECT_NAME} DESTINATION /usr/lib/deepin-screensaver/modules)

# translation files
TRANSLATION_GENERATE(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations)
add_custom_target(${PROJECT_NAME}_qm_files DEPENDS ${QM_FILES})
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_qm_files)

# Install translations
install(FILES ${QM_FILES} DESTINATION share/${PROJECT_NAME}/translations)

# 安装桌面文件
install(FILES data/deepin-custom-screensaver.desktop DESTINATION /etc/deepin-screensaver/deepin-custom-screensaver)

# 安装图标
install(DIRECTORY src/icons/ DESTINATION /usr/lib/deepin-screensaver/modules/cover)

# 安装 DConfig 文件
install(FILES data/org.deepin.customscreensaver.json DESTINATION /usr/share/dconfig/overrides/org.deepin.screensaver)

# 安装 DConfig 配置文件，1040及以下的默认DTK环境不支持DConfig
set(DCONFIG_APPID org.deepin.screensaver)
file(GLOB DCONFIG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/*.json")
if(DEFINED DSG_DATA_DIR)
    if (DTK_VERSION_MAJOR EQUAL "6")
        dtk_add_config_meta_files(APPID ${DCONFIG_APPID} FILES ${DCONFIG_FILES})
    else()
        dconfig_meta_files(APPID ${DCONFIG_APPID} FILES ${DCONFIG_FILES})
    endif()
    message("-- DConfig is supported by DTK")
else()
    install(FILES ${DCONFIG_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/${DCONFIG_APPID})
    message("-- DConfig is not supported by DTK")
endif()