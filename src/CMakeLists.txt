cmake_minimum_required(VERSION 3.14)

project(deepin-screensaver LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

# 查找 Qt 模块
set(qt_required_components Core Gui Widgets DBus Quick WaylandClient)

if (QT_DESIRED_VERSION MATCHES 6)
#     list(APPEND qt_required_components GuiPrivate)
else()
    list(APPEND qt_required_components X11Extras XkbCommonSupport)
endif()

find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS ${qt_required_components})
set(SESSION_LOCK_QT_LIBS Qt${QT_DESIRED_VERSION}::WaylandClientPrivate)
if (QT_DESIRED_VERSION MATCHES 5)
    list(APPEND SESSION_LOCK_QT_LIBS Qt5::XkbCommonSupportPrivate)
endif()

# Qt 6.10 起，私有模块目标(如 Qt6::GuiPrivate)拆分为独立的 CMake 包，不再随主模块自动创建，需显式查找；
# 更早版本该目标由 Qt6GuiTargets.cmake 自动创建，无需额外处理
if (QT_DESIRED_VERSION MATCHES 6 AND Qt6_VERSION VERSION_GREATER_EQUAL 6.10)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Gui Core)

# 查找系统库
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11 xscrnsaver xext xcb)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols>=1.27)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)
set(SESSION_LOCK_XML "${WAYLAND_PROTOCOLS_DIR}/staging/ext-session-lock/ext-session-lock-v1.xml")
# Share generated protocol code between the application and fake-compositor test.
add_library(session-lock-protocol STATIC)
target_link_libraries(session-lock-protocol PUBLIC Qt${QT_DESIRED_VERSION}::Gui PkgConfig::WAYLAND_CLIENT)
target_include_directories(session-lock-protocol PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
if (QT_DESIRED_VERSION MATCHES 6)
    find_package(WaylandScanner REQUIRED)
    qt6_generate_wayland_protocol_client_sources(session-lock-protocol
        PRIVATE_CODE FILES ${SESSION_LOCK_XML})
else()
    # Qt 5.15 ships qtwaylandscanner, but no client protocol CMake helper.
    get_target_property(QT5_QMAKE Qt5::qmake IMPORTED_LOCATION)
    execute_process(COMMAND ${QT5_QMAKE} -query QT_HOST_BINS
        OUTPUT_VARIABLE QT5_HOST_BINS OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE QT5_QMAKE_RESULT)
    if (NOT QT5_QMAKE_RESULT EQUAL 0)
        message(FATAL_ERROR "Cannot query Qt5 host tools directory")
    endif()
    find_program(QT5_WAYLAND_SCANNER qtwaylandscanner HINTS ${QT5_HOST_BINS} REQUIRED)
    set(SESSION_LOCK_HEADER "${CMAKE_CURRENT_BINARY_DIR}/wayland-ext-session-lock-v1-client-protocol.h")
    set(SESSION_LOCK_CODE "${CMAKE_CURRENT_BINARY_DIR}/wayland-ext-session-lock-v1-protocol.c")
    set(SESSION_LOCK_QT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qwayland-ext-session-lock-v1.h")
    set(SESSION_LOCK_QT_CODE "${CMAKE_CURRENT_BINARY_DIR}/qwayland-ext-session-lock-v1.cpp")
    add_custom_command(OUTPUT ${SESSION_LOCK_HEADER} ${SESSION_LOCK_CODE} ${SESSION_LOCK_QT_HEADER} ${SESSION_LOCK_QT_CODE}
        COMMAND ${WAYLAND_SCANNER} client-header ${SESSION_LOCK_XML} ${SESSION_LOCK_HEADER}
        COMMAND ${WAYLAND_SCANNER} private-code ${SESSION_LOCK_XML} ${SESSION_LOCK_CODE}
        COMMAND ${QT5_WAYLAND_SCANNER} client-header ${SESSION_LOCK_XML} > ${SESSION_LOCK_QT_HEADER}
        COMMAND ${QT5_WAYLAND_SCANNER} client-code ${SESSION_LOCK_XML} > ${SESSION_LOCK_QT_CODE}
        DEPENDS ${SESSION_LOCK_XML} ${QT5_WAYLAND_SCANNER} VERBATIM)
    target_sources(session-lock-protocol PRIVATE ${SESSION_LOCK_HEADER} ${SESSION_LOCK_CODE}
        ${SESSION_LOCK_QT_HEADER} ${SESSION_LOCK_QT_CODE})
endif()

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
    waylandsaver.cpp
    sessionlock.cpp
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
    waylandsaver.h
    sessionlock.h
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
    session-lock-protocol
    ${SESSION_LOCK_QT_LIBS}
)

if (QT_DESIRED_VERSION MATCHES 6)
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::GuiPrivate)
else()
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::X11Extras)
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

if (BUILD_TESTING)
    pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)
    pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
    find_package(Threads REQUIRED)
    set(SESSION_LOCK_SERVER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-server-protocol.h")
    add_custom_command(OUTPUT ${SESSION_LOCK_SERVER_HEADER}
        COMMAND ${WAYLAND_SCANNER} server-header ${SESSION_LOCK_XML} ${SESSION_LOCK_SERVER_HEADER}
        DEPENDS ${SESSION_LOCK_XML} VERBATIM)
    add_executable(waylandsaver-smoke ../tests/waylandsaver_smoke.cpp
        waylandsaver.cpp waylandsaver.h sessionlock.cpp sessionlock.h ${SESSION_LOCK_SERVER_HEADER})
    target_include_directories(waylandsaver-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(waylandsaver-smoke PRIVATE SAVER_TEST_NO_DCONFIG)
    target_link_libraries(waylandsaver-smoke PRIVATE session-lock-protocol
        Dtk${DTK_VERSION_MAJOR}::Core ${SESSION_LOCK_QT_LIBS}
        PkgConfig::WAYLAND_SERVER PkgConfig::XKBCOMMON Threads::Threads)
    add_test(NAME waylandsaver-smoke COMMAND waylandsaver-smoke)
    set_tests_properties(waylandsaver-smoke PROPERTIES TIMEOUT 60)
endif()

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