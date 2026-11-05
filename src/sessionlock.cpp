// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Bypass 阻止 Qt 创建 XDG role；首 configure 后附加 Qt 黑色 bootstrap，避免 show 的空提交。
#include "sessionlock.h"

#include <QGuiApplication>
#include <QPointer>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QtWaylandClient/QWaylandClientExtension>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandshmbackingstore_p.h>
#include <QtWaylandClient/private/qwaylandsurface_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include "qwayland-ext-session-lock-v1.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

using namespace QtWaylandClient;

class LockManager : public QWaylandClientExtensionTemplate<LockManager>,
                    public QtWayland::ext_session_lock_manager_v1
{
public:
    LockManager() : QWaylandClientExtensionTemplate(1) {}
    ~LockManager() override { if (isInitialized()) destroy(); }
};

class LockSurface : public QObject, public QtWayland::ext_session_lock_surface_v1
{
public:
    QPointer<QWindow> window;
    std::vector<std::unique_ptr<QWaylandShmBuffer>> buffers;
    std::function<void()> abort;
    LockSurface(QWindow *w, ::ext_session_lock_surface_v1 *role, QObject *parent)
        : QObject(parent), QtWayland::ext_session_lock_surface_v1(role), window(w) {}
    ~LockSurface() override
    {
        destroy();
        // Destroy the platform surface while its bootstrap storage is still alive.
        if (window) window->destroy();
    }
protected:
    void ext_session_lock_surface_v1_configure(uint32_t serial, uint32_t width, uint32_t height) override
    {
        if (!window) return;
        auto *platform = dynamic_cast<QWaylandWindow *>(window->handle());
        if (!platform || !platform->wlSurface()) return;
        buffers.erase(std::remove_if(buffers.begin(), buffers.end(),
                                    [](const auto &old) { return !old->busy(); }), buffers.end());
        const int scale = qMax(1, int(std::ceil(platform->devicePixelRatio())));
        if (!width || !height || quint64(width) * scale > 32768 || quint64(height) * scale > 32768
            || quint64(width) * height * scale * scale > 67108864 || buffers.size() >= 4) {
            QTimer::singleShot(0, this, abort);
            return;
        }
        const QSize size{int(width), int(height)};
        std::unique_ptr<QWaylandShmBuffer> buffer(new QWaylandShmBuffer(
            platform->display(), size * scale, QImage::Format_ARGB32, scale));
        if (!buffer->buffer()) { QTimer::singleShot(0, this, abort); return; }
        buffer->image()->fill(Qt::black);
        platform->resizeFromApplyConfigure(size);
        ack_configure(serial);
        auto *surface = platform->waylandSurface();
        int bufferScale = scale;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        // Qt's fractional-scale viewport handles scaling; leave buffer_scale at one.
        if (platform->display()->viewporter() && platform->display()->fractionalScaleManager()
            && QGuiApplication::highDpiScaleFactorRoundingPolicy() == Qt::HighDpiScaleFactorRoundingPolicy::PassThrough)
            bufferScale = 1;
#endif
        if (wl_proxy_get_version(reinterpret_cast<wl_proxy *>(surface->object())) >= 3)
            surface->set_buffer_scale(bufferScale);
        surface->attach(buffer->buffer(), 0, 0);
        surface->damage(0, 0, width, height);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        buffer->setBusy();
#else
        buffer->setBusy(true);
#endif
        buffers.push_back(std::move(buffer));
        // Qt5 orientation setup / Qt6.8 initWindow commits; never show before this attach.
        if (!window->isVisible()) window->show();
        else platform->commit();
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
        platform->handleExpose(QRect(QPoint(), size));
#else
        platform->sendRecursiveExposeEvent();
#endif
        window->requestUpdate();
    }
};

class SessionLockPrivate : public QObject, public QtWayland::ext_session_lock_v1
{
public:
    SessionLock *q;
    std::unique_ptr<LockManager> manager;
    QTimer timeout;
    QList<LockSurface *> surfaces;
    bool pending = false, secured = false, cancelling = false, syncing = false;
    wl_callback *sync = nullptr;
    ~SessionLockPrivate() override { if (sync) wl_callback_destroy(sync); }
    void synced()
    {
        wl_callback_destroy(sync);
        sync = nullptr;
        syncing = false;
        if (q) emit q->stopped();
        else deleteLater();
    }
    explicit SessionLockPrivate(SessionLock *owner) : QObject(owner), q(owner)
    {
        timeout.setSingleShot(true);
        connect(&timeout, &QTimer::timeout, this, [this] {
            if (!pending || cancelling) return;
            if (isInitialized()) {
                cancelling = true;
                clearWindows();
                if (q) emit q->failed(QStringLiteral("Timed out waiting for compositor lock confirmation"));
                return; // A late locked still requires unlock_and_destroy.
            }
            pending = false;
            QPointer<SessionLock> owner = q;
            if (owner) emit owner->failed(QStringLiteral("Session-lock protocol is unavailable"));
            if (owner) emit owner->stopped();
        });
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
        if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
            manager.reset(new LockManager);
            connect(manager.get(), &QWaylandClientExtension::activeChanged, this, [this] { acquire(); });
        }
#endif
    }
    void acquire()
    {
        if (!pending || cancelling || isInitialized() || !manager || !manager->isActive()) return;
        timeout.start(3000);
        init(manager->lock());
        emit q->ready();
    }
    void clearWindows()
    {
        const auto old = surfaces;
        surfaces.clear();
        qDeleteAll(old);
    }
    void finish(bool rejected)
    {
        clearWindows();
        if (isInitialized()) {
            if (secured) unlock_and_destroy();
            else destroy();
        }
        secured = pending = cancelling = false;
        timeout.stop();
        // Confirm that unlock was processed before allowing a cross-connection handoff.
        syncing = true;
        sync = wl_display_sync(manager->integration()->display()->wl_display());
        static const wl_callback_listener listener = {
            [](void *data, wl_callback *, uint32_t) { static_cast<SessionLockPrivate *>(data)->synced(); }
        };
        wl_callback_add_listener(sync, &listener, this);
        manager->integration()->display()->flushRequests();
        if (rejected && q) emit q->failed(QStringLiteral("Compositor refused or ended the session lock"));
    }
protected:
    void ext_session_lock_v1_locked() override
    {
        secured = true;
        if (cancelling) finish(false);
        else { pending = false; timeout.stop(); emit q->locked(); }
    }
    void ext_session_lock_v1_finished() override { finish(!cancelling); }
};

SessionLock::SessionLock(QObject *parent) : QObject(parent), d(new SessionLockPrivate(this)) {}
SessionLock::~SessionLock()
{
    d->q = nullptr;
    // Pending requests and unlock syncs outlive the facade until the compositor replies.
    if (d->isInitialized() || d->syncing) {
        d->setParent(QCoreApplication::instance());
        d->cancelling = true;
        if (d->secured) d->finish(false);
        else d->clearWindows();
    } else delete d;
}
bool SessionLock::isAvailable() const { return d->manager && d->manager->isActive(); }
bool SessionLock::isLocked() const { return d->secured; }
bool SessionLock::isBusy() const { return d->pending || d->cancelling || d->secured || d->syncing; }
void SessionLock::start()
{
    if (isLocked() || isBusy()) return;
    if (!d->manager) {
        emit failed(QStringLiteral("Session lock requires Wayland and Qt earlier than 6.9"));
        emit stopped();
        return;
    }
    d->pending = true;
    d->timeout.start(3000);
    QTimer::singleShot(0, d, [state = d] { state->acquire(); });
}
void SessionLock::stop()
{
    if (d->secured) { d->finish(false); return; }
    if (!d->pending) return;
    if (d->isInitialized()) { d->cancelling = true; d->clearWindows(); return; }
    d->pending = false;
    d->timeout.stop();
    emit stopped();
}
bool SessionLock::addWindow(QWindow *window)
{
    if (!d->isInitialized() || d->cancelling || !window || window->isVisible() || window->handle()
        || window->parent() || !window->screen()) return false;
    auto *screen = dynamic_cast<QWaylandScreen *>(window->screen()->handle());
    if (!screen || screen->isPlaceholder() || !screen->output()) return false;
    for (auto *surface : d->surfaces)
        if (surface->window && surface->window->screen() == window->screen()) return false;
    qputenv("QT_WAYLAND_USE_BYPASSWINDOWMANAGERHINT", "1");
    window->setFlags(window->flags() | Qt::BypassWindowManagerHint | Qt::FramelessWindowHint);
    window->create();
    auto *platform = dynamic_cast<QWaylandWindow *>(window->handle());
    if (!platform || !platform->wlSurface() || platform->shellSurface()) return false;
    auto *surface = new LockSurface(window, d->get_lock_surface(platform->wlSurface(), screen->output()), d);
    surface->abort = [state = d] {
        if (state->q) emit state->q->failed(QStringLiteral("Invalid configure or unavailable lock buffer"));
        if (state->syncing) return;
        if (state->secured) state->finish(false);
        else { state->cancelling = true; state->clearWindows(); }
    };
    d->surfaces.append(surface);
    connect(window, &QObject::destroyed, surface, [state = d, surface] {
        state->surfaces.removeOne(surface);
        delete surface;
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, surface,
            [state = d, surface, outputScreen = window->screen()](QScreen *removed) {
        if (removed != outputScreen) return;
        state->surfaces.removeOne(surface);
        delete surface;
    });
    platform->display()->flushRequests();
    return true;
}
