/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef DBUSSCREENSAVER_H
#define DBUSSCREENSAVER_H

#include <QObject>
#include <QProcess>
#include <QDir>
#include <QTimer>
#include <QSettings>
#include <QDBusMessage>

class ScreenSaverWindow;
class DBusScreenSaver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList allScreenSaver READ allScreenSaver NOTIFY allScreenSaverChanged)
    Q_PROPERTY(int batteryScreenSaverTimeout READ batteryScreenSaverTimeout WRITE setBatteryScreenSaverTimeout NOTIFY batteryScreenSaverTimeoutChanged)
    Q_PROPERTY(int linePowerScreenSaverTimeout READ linePowerScreenSaverTimeout WRITE setLinePowerScreenSaverTimeout NOTIFY linePowerScreenSaverTimeoutChanged)
    Q_PROPERTY(QString currentScreenSaver READ currentScreenSaver WRITE setCurrentScreenSaver NOTIFY currentScreenSaverChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)

public:
    explicit DBusScreenSaver(QObject *parent = nullptr);
    ~DBusScreenSaver();

    bool Preview(const QString &name, int staysOn = 1, bool preview = true);
    QString GetScreenSaverCover(const QString &name) const;
    void RefreshScreenSaverList();
    void Start(const QString &name = QString());
    void Stop();

    QStringList allScreenSaver() const;
    int batteryScreenSaverTimeout() const;
    int linePowerScreenSaverTimeout() const;
    QString currentScreenSaver() const;
    bool isRunning() const;

    void setBatteryScreenSaverTimeout(int batteryScreenSaverTimeout);
    void setLinePowerScreenSaverTimeout(int linePowerScreenSaverTimeout);
    void setCurrentScreenSaver(QString currentScreenSaver);

signals:
    void allScreenSaverChanged(QStringList allScreenSaver);
    void batteryScreenSaverTimeoutChanged(int batteryScreenSaverTimeout);
    void linePowerScreenSaverTimeoutChanged(int linePowerScreenSaverTimeout);
    void currentScreenSaverChanged(QString currentScreenSaver);
    void isRunningChanged(bool isRunning);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    Q_SLOT void onDBusPropertyChanged(const QString &interface, const QVariantMap &changed_properties, const QDBusMessage &message);

    void clearResourceList();

    const QList<QDir> m_resourceDirList;
    QStringList m_resourceList;

    const QList<QDir> m_moduleDirList;
    QStringList m_screenSaverList;
    QMap<QString, QDir> m_screenSaverNameToDir;
    QString m_currentScreenSaver;
    ScreenSaverWindow *m_window = nullptr;
    QProcess *m_process = nullptr;
    QTimer m_autoQuitTimer;
    QSettings m_settings;
};

#endif // DBUSSCREENSAVER_H
