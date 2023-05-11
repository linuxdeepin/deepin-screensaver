#-------------------------------------------------
#
# Project created by QtCreator 2021-05-14T10:44:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = deepin-video-screensaver
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11 link_pkgconfig ENABLE_DMR

CONFIG(ENABLE_DMR) {
    message("using dmr for video")
    PKGCONFIG += libdmr
    DEFINES += ENABLE_DMR
}else:{
   QT += multimediawidgets
}

include(../../common.pri)

PKGCONFIG += x11 xcb

SOURCES += \
        main.cpp \
    commandlinemanager.cpp \
    videoscreensaver.cpp \
    xproxywindow.cpp

HEADERS += \ 
    commandlinemanager.h \
    videoscreensaver.h \
    xproxywindow.h

target.path = $$MODULE_PATH

defaultVideo.path = $$PREFIX/share/$$TARGET/resource
defaultVideo.files = $$PWD/data/video/.readme

cover.path = $$MODULE_PATH/cover
cover.files += $$PWD/data/cover/*

INSTALLS += target cover defaultVideo


