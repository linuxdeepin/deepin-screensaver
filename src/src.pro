TARGET = deepin-screensaver
QT += gui dbus quick
CONFIG += c++11

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/screensaverwindow.cpp \
    $$PWD/dbusscreensaver.cpp \
    imageprovider.cpp

HEADERS += \
    $$PWD/screensaverwindow.h \
    $$PWD/dbusscreensaver.h \
    imageprovider.h

screensaver.files = $$PWD/com.deepin.ScreenSaver.xml
screensaver.header_flags += -l DBusScreenSaver -i $$PWD/dbusscreensaver.h
screensaver.source_flags += -l DBusScreenSaver

DBUS_ADAPTORS += screensaver

screensaver_xmls.path = /usr/share/dbus-1/interfaces
screensaver_xmls.files = $$screensaver.files

screensaver_service.path = /usr/share/dbus-1/services
screensaver_service.files = $$PWD/com.deepin.ScreenSaver.service

INSTALLS += screensaver_service screensaver_xmls

isEmpty(PREFIX) {
    PREFIX = /usr
}

isEmpty(MODULE_PATH) {
    # /usr/lib/deepin-screensaver/modules
    MODULE_PATH = $$PREFIX/lib/$$TARGET/modules
}

isEmpty(RESOURCE_PATH) {
    # /usr/lib/deepin-screensaver/modules
    RESOURCE_PATH = $$PREFIX/lib/$$TARGET/resources
}

DEFINES += MODULE_PATH=\\\"$$MODULE_PATH\\\" RESOURCE_PATH=\\\"$$RESOURCE_PATH\\\"

target.path = $$PREFIX/bin

INSTALLS += target
