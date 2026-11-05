// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WAYLANDSAVER_H
#define WAYLANDSAVER_H

#include <QObject>
#include <memory>

// Qt slideshow windows, one per screen. SessionLock handles the platform role.
// Preview also uses session-lock. Authentication policy belongs to the caller;
// wakeRequested() does not unlock the session.
class WaylandSaver : public QObject
{
    Q_OBJECT
public:
    explicit WaylandSaver(QObject *parent = nullptr);
    ~WaylandSaver() override;

    // Requests are asynchronous: true means accepted; activated() means locked.
    bool start();
    // Pending requests remain alive until the compositor answers the cancellation.
    // True means already stopped, otherwise wait for stopped().
    bool stop();
    bool isRunning() const;
    // Includes discovery, a pending lock/cancellation, and unlock sync.
    bool isBusy() const;

signals:
    void activated();
    void wakeRequested();
    void stopped();
    void failed(const QString &reason);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif
