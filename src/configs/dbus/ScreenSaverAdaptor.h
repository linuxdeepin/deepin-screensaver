// SPDX-FileCopyrightText: 2024 - 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCREENSAVERADAPTOR_H
#define SCREENSAVERADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

/*
 * Adaptor class for interface com.deepin.ScreenSaver
 */
class ScreenSaverAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.ScreenSaver")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.deepin.ScreenSaver\">\n"
"    <method name=\"Start\"/>\n"
"    <method name=\"Stop\"/>\n"
"    <method name=\"Preview\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"staysOn\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"ok\"/>\n"
"    </method>\n"
"    <method name=\"GetScreenSaverCover\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\"/>\n"
"      <arg direction=\"out\" type=\"s\" name=\"imagePath\"/>\n"
"    </method>\n"
"    <method name=\"StartCustomConfig\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"ok\"/>\n"
"    </method>\n"
"    <method name=\"ConfigurableItems\">\n"
"      <arg direction=\"out\" type=\"as\" name=\"name\"/>\n"
"    </method>\n"
"    <method name=\"IsConfigurable\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"ok\"/>\n"
"    </method>\n"
"    <property access=\"read\" type=\"b\" name=\"isRunning\"/>\n"
"    <property access=\"readwrite\" type=\"s\" name=\"currentScreenSaver\"/>\n"
"    <property access=\"read\" type=\"as\" name=\"allScreenSaver\"/>\n"
"    <method name=\"RefreshScreenSaverList\"/>\n"
"    <property access=\"readwrite\" type=\"i\" name=\"batteryScreenSaverTimeout\"/>\n"
"    <property access=\"readwrite\" type=\"i\" name=\"linePowerScreenSaverTimeout\"/>\n"
"    <property access=\"readwrite\" type=\"b\" name=\"lockScreenAtAwake\"/>\n"
"    <property access=\"readwrite\" type=\"i\" name=\"lockScreenDelay\"/>\n"
"  </interface>\n"
        "")
public:
    ScreenSaverAdaptor(QObject *parent);
    virtual ~ScreenSaverAdaptor();

public: // PROPERTIES
    Q_PROPERTY(QStringList allScreenSaver READ allScreenSaver)
    QStringList allScreenSaver() const;

    Q_PROPERTY(int batteryScreenSaverTimeout READ batteryScreenSaverTimeout WRITE setBatteryScreenSaverTimeout)
    int batteryScreenSaverTimeout() const;
    void setBatteryScreenSaverTimeout(int value);

    Q_PROPERTY(QString currentScreenSaver READ currentScreenSaver WRITE setCurrentScreenSaver)
    QString currentScreenSaver() const;
    void setCurrentScreenSaver(const QString &value);

    Q_PROPERTY(bool isRunning READ isRunning)
    bool isRunning() const;

    Q_PROPERTY(int linePowerScreenSaverTimeout READ linePowerScreenSaverTimeout WRITE setLinePowerScreenSaverTimeout)
    int linePowerScreenSaverTimeout() const;
    void setLinePowerScreenSaverTimeout(int value);

    Q_PROPERTY(bool lockScreenAtAwake READ lockScreenAtAwake WRITE setLockScreenAtAwake)
    bool lockScreenAtAwake() const;
    void setLockScreenAtAwake(bool value);

    Q_PROPERTY(int lockScreenDelay READ lockScreenDelay WRITE setLockScreenDelay)
    int lockScreenDelay() const;
    void setLockScreenDelay(int value);

public Q_SLOTS: // METHODS
    QStringList ConfigurableItems();
    QString GetScreenSaverCover(const QString &name);
    bool IsConfigurable(const QString &name);
    bool Preview(const QString &name, int staysOn);
    void RefreshScreenSaverList();
    void Start();
    bool StartCustomConfig(const QString &name);
    void Stop();
Q_SIGNALS: // SIGNALS
};

#endif
