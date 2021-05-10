TARGET = deepin-screensaver
QT += gui dbus quick x11extras
CONFIG += c++11 link_pkgconfig
PKGCONFIG += x11 xscrnsaver xext

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/screensaverwindow.cpp \
    $$PWD/dbusscreensaver.cpp \
    imageprovider.cpp \
    screensaverview.cpp

HEADERS += \
    $$PWD/screensaverwindow.h \
    $$PWD/dbusscreensaver.h \
    imageprovider.h \
    screensaverview.h

screensaver.files = $$PWD/com.deepin.ScreenSaver.xml
screensaver.header_flags += -l DBusScreenSaver -i $$PWD/dbusscreensaver.h
screensaver.source_flags += -l DBusScreenSaver

DBUS_ADAPTORS += screensaver

include(../common.pri)

screensaver_xmls.path = $$PREFIX/share/dbus-1/interfaces
screensaver_xmls.files = $$screensaver.files

screensaver_service.path = $$PREFIX/share/dbus-1/services
screensaver_service.files = $$PWD/com.deepin.ScreenSaver.service

INSTALLS += screensaver_service screensaver_xmls

DEFINES += LIB_PATH=\\\"$$LIB_PATH\\\" MODULE_PATH=\\\"$$MODULE_PATH\\\" RESOURCE_PATH=\\\"$$RESOURCE_PATH\\\"

target.path = $$PREFIX/bin

INSTALLS += target

isEmpty(VERSION) {
    VERSION = 0.0.1
}
DEFINES += QMAKE_VERSION=\\\"$$VERSION\\\"
