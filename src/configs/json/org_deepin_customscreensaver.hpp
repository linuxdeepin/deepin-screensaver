// SPDX-FileCopyrightText: 2024 - 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ORG_DEEPIN_CUSTOMSCREENSAVER_H
#define ORG_DEEPIN_CUSTOMSCREENSAVER_H

#include <QThread>
#include <QVariant>
#include <QDebug>
#include <QAtomicPointer>
#include <QAtomicInteger>
#include <DConfig>

class org_deepin_customscreensaver : public QObject {
    Q_OBJECT

    Q_PROPERTY(double intervalTime READ intervalTime WRITE setIntervalTime NOTIFY intervalTimeChanged)
    Q_PROPERTY(double playMode READ playMode WRITE setPlayMode NOTIFY playModeChanged)
    Q_PROPERTY(QString slideshowPath READ slideshowPath WRITE setSlideshowPath NOTIFY slideshowPathChanged)
public:
    explicit org_deepin_customscreensaver(QThread *thread, const QString &appId, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(appId, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_customscreensaver(QThread *thread, DTK_CORE_NAMESPACE::DConfigBackend *backend, const QString &appId, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(backend, appId, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_customscreensaver(QThread *thread, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_customscreensaver(QThread *thread, DTK_CORE_NAMESPACE::DConfigBackend *backend, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(backend, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    ~org_deepin_customscreensaver() {
        if (m_config.loadRelaxed()) {
            m_config.loadRelaxed()->deleteLater();
        }
    }

    double intervalTime() const {
        return p_intervalTime;
    }
    void setIntervalTime(const double &value) {
        auto oldValue = p_intervalTime;
        p_intervalTime = value;
        markPropertySet(0);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("intervalTime"), value);
            });
        }
        if (p_intervalTime != oldValue) {
            Q_EMIT intervalTimeChanged();
        }
    }
    double playMode() const {
        return p_playMode;
    }
    void setPlayMode(const double &value) {
        auto oldValue = p_playMode;
        p_playMode = value;
        markPropertySet(1);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("playMode"), value);
            });
        }
        if (p_playMode != oldValue) {
            Q_EMIT playModeChanged();
        }
    }
    QString slideshowPath() const {
        return p_slideshowPath;
    }
    void setSlideshowPath(const QString &value) {
        auto oldValue = p_slideshowPath;
        p_slideshowPath = value;
        markPropertySet(2);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("slideshowPath"), value);
            });
        }
        if (p_slideshowPath != oldValue) {
            Q_EMIT slideshowPathChanged();
        }
    }
Q_SIGNALS:
    void intervalTimeChanged();
    void playModeChanged();
    void slideshowPathChanged();
private:
    void initialize(DTK_CORE_NAMESPACE::DConfig *config) {
        Q_ASSERT(!m_config.loadRelaxed());
        m_config.storeRelaxed(config);
        if (testPropertySet(0)) {
            config->setValue(QStringLiteral("intervalTime"), QVariant::fromValue(p_intervalTime));
        } else {
            updateValue(QStringLiteral("intervalTime"), QVariant::fromValue(p_intervalTime));
        }
        if (testPropertySet(1)) {
            config->setValue(QStringLiteral("playMode"), QVariant::fromValue(p_playMode));
        } else {
            updateValue(QStringLiteral("playMode"), QVariant::fromValue(p_playMode));
        }
        if (testPropertySet(2)) {
            config->setValue(QStringLiteral("slideshowPath"), QVariant::fromValue(p_slideshowPath));
        } else {
            updateValue(QStringLiteral("slideshowPath"), QVariant::fromValue(p_slideshowPath));
        }

        connect(config, &DTK_CORE_NAMESPACE::DConfig::valueChanged, this, [this](const QString &key) {
            updateValue(key);
        }, Qt::DirectConnection);
    }
    void updateValue(const QString &key, const QVariant &fallback = QVariant()) {
        Q_ASSERT(QThread::currentThread() == m_config.loadRelaxed()->thread());
        const QVariant &value = m_config.loadRelaxed()->value(key, fallback);
        if (key == QStringLiteral("intervalTime")) {
            auto newValue = qvariant_cast<double>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_intervalTime != newValue) {
                    p_intervalTime = newValue;
                    Q_EMIT intervalTimeChanged();
                }
            });
            return;
        }
        if (key == QStringLiteral("playMode")) {
            auto newValue = qvariant_cast<double>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_playMode != newValue) {
                    p_playMode = newValue;
                    Q_EMIT playModeChanged();
                }
            });
            return;
        }
        if (key == QStringLiteral("slideshowPath")) {
            auto newValue = qvariant_cast<QString>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_slideshowPath != newValue) {
                    p_slideshowPath = newValue;
                    Q_EMIT slideshowPathChanged();
                }
            });
            return;
        }
    }
    inline void markPropertySet(const int index) {
        if (index < 32) {
            m_propertySetStatus0.fetchAndOrOrdered(1 << (index - 0));
            return;
        }
        Q_UNREACHABLE();
    }
    inline bool testPropertySet(const int index) const {
        if (index < 32) {
            return (m_propertySetStatus0.loadRelaxed() & (1 << (index - 0)));
        }
        Q_UNREACHABLE();
    }
    QAtomicPointer<DTK_CORE_NAMESPACE::DConfig> m_config = nullptr;
    double p_intervalTime { 0 };
    double p_playMode { -1 };
    QString p_slideshowPath { QStringLiteral("") };
    QAtomicInteger<quint32> m_propertySetStatus0 = 0;
};

#endif // ORG_DEEPIN_CUSTOMSCREENSAVER_H
