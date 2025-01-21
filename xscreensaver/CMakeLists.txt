cmake_minimum_required(VERSION 3.14)

project(xscreensaver)

# 安装封面图片
install(DIRECTORY cover/ DESTINATION ${MODULE_PATH}/cover)

set(XSCREENSAVER_DATA_PATH $ENV{XSCREENSAVER_DATA_PATH})
if(NOT XSCREENSAVER_DATA_PATH)
    if (QT_DESIRED_VERSION MATCHES 6)
        set(XSCREENSAVER_DATA_PATH ${CMAKE_INSTALL_PREFIX}/libexec/xscreensaver)
    else()
        set(XSCREENSAVER_DATA_PATH ${CMAKE_INSTALL_PREFIX}/lib/xscreensaver)
    endif()
endif()

message(STATUS "XSCREENSAVER_DATA_PATH: ${XSCREENSAVER_DATA_PATH}")

# 屏保列表
set(SCREENSAVER_LIST "blaster bouboule cage discoball dymaxionmap endgame epicycle flow flurry flyingtoasters fuzzyflakes galaxy glplanet hexadrop klein lament lockward polytopes vermiculate")

# 计算相对路径
file(RELATIVE_PATH RELATIVE_PATH ${MODULE_PATH} ${XSCREENSAVER_DATA_PATH})

# 直接生成 postinst 文件到 debian/ 目录
configure_file(
    ${CMAKE_SOURCE_DIR}/debian/deepin-screensaver-xscreensaver-data.postinst.in
    ${CMAKE_SOURCE_DIR}/debian/deepin-screensaver-xscreensaver-data.postinst
    @ONLY
)

# 设置 postinst 文件的可执行权限
file(
    CHMOD ${CMAKE_SOURCE_DIR}/debian/deepin-screensaver-xscreensaver-data.postinst
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)