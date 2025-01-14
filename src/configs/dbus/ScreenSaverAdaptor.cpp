// SPDX-FileCopyrightText: 2024 - 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ScreenSaverAdaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class ScreenSaverAdaptor
 */

ScreenSaverAdaptor::ScreenSaverAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

ScreenSaverAdaptor::~ScreenSaverAdaptor()
{
    // destructor
}

QStringList ScreenSaverAdaptor::allScreenSaver() const
{
    // get the value of property allScreenSaver
    return qvariant_cast< QStringList >(parent()->property("allScreenSaver"));
}

int ScreenSaverAdaptor::batteryScreenSaverTimeout() const
{
    // get the value of property batteryScreenSaverTimeout
    return qvariant_cast< int >(parent()->property("batteryScreenSaverTimeout"));
}

void ScreenSaverAdaptor::setBatteryScreenSaverTimeout(int value)
{
    // set the value of property batteryScreenSaverTimeout
    parent()->setProperty("batteryScreenSaverTimeout", QVariant::fromValue(value));
}

QString ScreenSaverAdaptor::currentScreenSaver() const
{
    // get the value of property currentScreenSaver
    return qvariant_cast< QString >(parent()->property("currentScreenSaver"));
}

void ScreenSaverAdaptor::setCurrentScreenSaver(const QString &value)
{
    // set the value of property currentScreenSaver
    parent()->setProperty("currentScreenSaver", QVariant::fromValue(value));
}

bool ScreenSaverAdaptor::isRunning() const
{
    // get the value of property isRunning
    return qvariant_cast< bool >(parent()->property("isRunning"));
}

int ScreenSaverAdaptor::linePowerScreenSaverTimeout() const
{
    // get the value of property linePowerScreenSaverTimeout
    return qvariant_cast< int >(parent()->property("linePowerScreenSaverTimeout"));
}

void ScreenSaverAdaptor::setLinePowerScreenSaverTimeout(int value)
{
    // set the value of property linePowerScreenSaverTimeout
    parent()->setProperty("linePowerScreenSaverTimeout", QVariant::fromValue(value));
}

bool ScreenSaverAdaptor::lockScreenAtAwake() const
{
    // get the value of property lockScreenAtAwake
    return qvariant_cast< bool >(parent()->property("lockScreenAtAwake"));
}

void ScreenSaverAdaptor::setLockScreenAtAwake(bool value)
{
    // set the value of property lockScreenAtAwake
    parent()->setProperty("lockScreenAtAwake", QVariant::fromValue(value));
}

int ScreenSaverAdaptor::lockScreenDelay() const
{
    // get the value of property lockScreenDelay
    return qvariant_cast< int >(parent()->property("lockScreenDelay"));
}

void ScreenSaverAdaptor::setLockScreenDelay(int value)
{
    // set the value of property lockScreenDelay
    parent()->setProperty("lockScreenDelay", QVariant::fromValue(value));
}

QStringList ScreenSaverAdaptor::ConfigurableItems()
{
    // handle method call com.deepin.ScreenSaver.ConfigurableItems
    QStringList name;
    QMetaObject::invokeMethod(parent(), "ConfigurableItems", Q_RETURN_ARG(QStringList, name));
    return name;
}

QString ScreenSaverAdaptor::GetScreenSaverCover(const QString &name)
{
    // handle method call com.deepin.ScreenSaver.GetScreenSaverCover
    QString imagePath;
    QMetaObject::invokeMethod(parent(), "GetScreenSaverCover", Q_RETURN_ARG(QString, imagePath), Q_ARG(QString, name));
    return imagePath;
}

bool ScreenSaverAdaptor::IsConfigurable(const QString &name)
{
    // handle method call com.deepin.ScreenSaver.IsConfigurable
    bool ok;
    QMetaObject::invokeMethod(parent(), "IsConfigurable", Q_RETURN_ARG(bool, ok), Q_ARG(QString, name));
    return ok;
}

bool ScreenSaverAdaptor::Preview(const QString &name, int staysOn)
{
    // handle method call com.deepin.ScreenSaver.Preview
    bool ok;
    QMetaObject::invokeMethod(parent(), "Preview", Q_RETURN_ARG(bool, ok), Q_ARG(QString, name), Q_ARG(int, staysOn));
    return ok;
}

void ScreenSaverAdaptor::RefreshScreenSaverList()
{
    // handle method call com.deepin.ScreenSaver.RefreshScreenSaverList
    QMetaObject::invokeMethod(parent(), "RefreshScreenSaverList");
}

void ScreenSaverAdaptor::Start()
{
    // handle method call com.deepin.ScreenSaver.Start
    QMetaObject::invokeMethod(parent(), "Start");
}

bool ScreenSaverAdaptor::StartCustomConfig(const QString &name)
{
    // handle method call com.deepin.ScreenSaver.StartCustomConfig
    bool ok;
    QMetaObject::invokeMethod(parent(), "StartCustomConfig", Q_RETURN_ARG(bool, ok), Q_ARG(QString, name));
    return ok;
}

void ScreenSaverAdaptor::Stop()
{
    // handle method call com.deepin.ScreenSaver.Stop
    QMetaObject::invokeMethod(parent(), "Stop");
}

