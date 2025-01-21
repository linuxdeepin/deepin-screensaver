cmake_minimum_required(VERSION 3.14)

project(deepin-screensaver-preview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt 模块
set(qt_required_components Core Gui Widgets)

find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Core Gui Widgets)

# 源文件
set(SOURCES
    main.cpp
    previewwindow.cpp
)

# 头文件
set(HEADERS
    previewwindow.h
)

# UI 文件
set(FORMS
    previewwindow.ui
)

# 生成 UI 头文件
qt_wrap_ui(${PROJECT_NAME}_UI_HEADERS ${FORMS})

# 添加生成的 UI 头文件所在的目录到头文件搜索路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${FORMS} ${${PROJECT_NAME}_UI_HEADERS})

# 链接库
set(LINK_LIBS
    Qt${QT_DESIRED_VERSION}::Core
    Qt${QT_DESIRED_VERSION}::Gui
    Qt${QT_DESIRED_VERSION}::Widgets
)

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

# 安装目标
install(TARGETS ${PROJECT_NAME} DESTINATION bin)