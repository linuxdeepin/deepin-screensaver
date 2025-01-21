cmake_minimum_required(VERSION 3.14)

project(saverpic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt 模块
find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Quick)

# 源文件
set(SOURCES
    main.cpp
)

# 资源文件
set(RESOURCES
    saverpic.qrc
)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})

# 链接库
set(LINK_LIBS
    Qt${QT_DESIRED_VERSION}::Quick
)

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

# 安装qml文件
install(FILES saverpic.qml DESTINATION /usr/lib/deepin-screensaver/modules)