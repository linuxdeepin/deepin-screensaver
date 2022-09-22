TARGET = deepin-screensaver
QT += core gui dbus quick x11extras
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 link_pkgconfig
PKGCONFIG += x11 xscrnsaver xext xcb dframeworkdbus dtkwidget

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/screensaverwindow.cpp \
    $$PWD/dbusscreensaver.cpp \
    $$PWD/imageprovider.cpp \
    $$PWD/screensaverview.cpp \
    $$PWD/truncatelineedit.cpp \
    $$PWD/screensaversettingdialog.cpp \
    $$PWD/selectpathwidget.cpp \
    $$PWD/dbuscustomconfig.cpp \
    utils.cpp

HEADERS += \
    $$PWD/screensaverwindow.h \
    $$PWD/dbusscreensaver.h \
    $$PWD/imageprovider.h \
    $$PWD/screensaverview.h \
    $$PWD/selectpathwidget.h \
    $$PWD/truncatelineedit.h \
    $$PWD/screensaversettingdialog.h \
    $$PWD/dbuscustomconfig.h \
    utils.h

TRANSLATIONS += $$PWD/translations/$${TARGET}.ts \
    $$PWD/translations/$${TARGET}_zh_CN.ts

# Automating generation .qm files from .ts files
CONFIG(release, debug|release) {
    !system($$PWD/generate_translations.sh): error("Failed to generate translation")
    !system($$PWD/update_translations.sh): error("Failed to generate translation")
}

translations.path = /usr/share/$${TARGET}/translations
translations.files = translations/*.qm

screensaver.files = $$PWD/com.deepin.ScreenSaver.xml
screensaver.header_flags += -l DBusScreenSaver -i $$PWD/dbusscreensaver.h
screensaver.source_flags += -l DBusScreenSaver

customconfig.files = $$PWD/org.deepin.screensaver.CustomConfig.xml
customconfig.header_flags += -l DBusCustomConfig -i $$PWD/dbuscustomconfig.h
customconfig.source_flags += -l DBusCustomConfig

DBUS_ADAPTORS += screensaver customconfig

include(../common.pri)

screensaver_xmls.path = $$PREFIX/share/dbus-1/interfaces
screensaver_xmls.files = $$screensaver.files

customconfig_xmls.path = $$PREFIX/share/dbus-1/interfaces
customconfig_xmls.files = $$customconfig.files

screensaver_service.path = $$PREFIX/share/dbus-1/services
screensaver_service.files = $$PWD/com.deepin.ScreenSaver.service

customconfig_service.path = $$PREFIX/share/dbus-1/services
customconfig_service.files = $$PWD/org.deepin.screensaver.CustomConfig.service

INSTALLS += screensaver_service screensaver_xmls customconfig_service customconfig_xmls translations

DEFINES += LIB_PATH=\\\"$$LIB_PATH\\\" MODULE_PATH=\\\"$$MODULE_PATH\\\" RESOURCE_PATH=\\\"$$RESOURCE_PATH\\\"

target.path = $$PREFIX/bin
target.files = $$TARGET

INSTALLS += target

isEmpty(VERSION) {
    VERSION = 0.0.1
}
DEFINES += QMAKE_VERSION=\\\"$$VERSION\\\"
