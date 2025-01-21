cmake_minimum_required(VERSION 3.14)

project(deepin-screensaver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

# 查找 Qt 模块
set(qt_required_components Core Gui Widgets DBus Quick)

if (QT_DESIRED_VERSION MATCHES 6)
#     list(APPEND qt_required_components GuiPrivate)
else()
    list(APPEND qt_required_components X11Extras)
endif()

find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS ${qt_required_components})
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Gui Core)

# 查找系统库
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11 xscrnsaver xext xcb)

# 源文件
set(SOURCES
    main.cpp
    screensaverwindow.cpp
    dbusscreensaver.cpp
    imageprovider.cpp
    screensaverview.cpp
    screensaversettingdialog.cpp
    customconfig.cpp
    utils.cpp
    singlecustomsetting.cpp
    commandlinehelper.cpp
)

# 头文件
set(HEADERS
    screensaverwindow.h
    dbusscreensaver.h
    imageprovider.h
    screensaverview.h
    screensaversettingdialog.h
    customconfig.h
    utils.h
    singlecustomsetting.h
    commandlinehelper.h
)

# 翻译文件
set(TRANSLATIONS
    translations/deepin-screensaver.ts
    translations/deepin-screensaver_zh_CN.ts
)

# 资源文件
set(RESOURCES
    images.qrc
)

add_definitions(-DAPP_VERSION="$APP_VERSION")
add_definitions(-DVERSION="$VERSION")

# D-Bus XML 文件
set(DBUS_XML_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/com.deepin.ScreenSaver.xml
)

# 生成 D-Bus 适配器
if (QT_DESIRED_VERSION MATCHES 6)
    qt_add_dbus_adaptor(${PROJECT_NAME}_ADAPTORS ${DBUS_XML_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/dbusscreensaver.h DBusScreenSaver)
else()
    qt5_add_dbus_adaptor(${PROJECT_NAME}_ADAPTORS ${DBUS_XML_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/dbusscreensaver.h DBusScreenSaver)
endif()

# 添加生成的 UI 头文件所在的目录到头文件搜索路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES} ${${PROJECT_NAME}_ADAPTORS})

# 链接库
set(LINK_LIBS
    Qt${QT_DESIRED_VERSION}::Core
    Qt${QT_DESIRED_VERSION}::Gui
    Qt${QT_DESIRED_VERSION}::Widgets
    Qt${QT_DESIRED_VERSION}::DBus
    Qt${QT_DESIRED_VERSION}::Quick
    Dtk${DTK_VERSION_MAJOR}::Widget
    Dtk${DTK_VERSION_MAJOR}::Gui
    Dtk${DTK_VERSION_MAJOR}::Core
    ${X11_LIBRARIES}
)

if (QT_DESIRED_VERSION MATCHES 6)
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::GuiPrivate)
else()
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::X11Extras)
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

# 安装目标
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# translation files
TRANSLATION_GENERATE(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations)
add_custom_target(${PROJECT_NAME}_qm_files DEPENDS ${QM_FILES})
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_qm_files)

# Install translations
install(FILES ${QM_FILES} DESTINATION share/${PROJECT_NAME}/translations)

# 安装生成的适配器 XML 文件
install(FILES ${DBUS_XML_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/interfaces)

# 安装 D-Bus 服务文件
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/com.deepin.ScreenSaver.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)

# 安装 DConfig 配置文件，1040及以下的默认DTK环境不支持DConfig
set(DCONFIG_APPID org.deepin.screensaver)
file(GLOB DCONFIG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json")
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