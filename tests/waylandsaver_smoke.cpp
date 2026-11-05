// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Standalone fake-compositor smoke test. No real desktop is locked.
// cmake -S . -B build -DBUILD_TESTING=ON -DQT_VERSION_MAJOR=6
// cmake --build build --target waylandsaver-smoke
// ctest --test-dir build -R waylandsaver-smoke --output-on-failure
#include "waylandsaver.h"
#include "ext-session-lock-v1-server-protocol.h"
#include <QGuiApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QThread>
#include <QDir>
#include <QImage>
#include <QSettings>
#include <QProcess>
#include <QTimer>
#include <QTemporaryFile>
#include <xkbcommon/xkbcommon.h>
#include <unistd.h>
#include <wayland-server.h>
#include <wayland-server-protocol.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#define CHECK(condition) do { if (!(condition)) { std::cerr << "check failed at line " << __LINE__ << ": " #condition "\n"; std::abort(); } } while (false)

struct Fake {
    enum Mode { Accept, Early, Refuse, Delayed };
    struct Surface { Fake *self; wl_resource *buffer = nullptr; wl_resource *role = nullptr; bool acked = false; int width = 64; int height = 48; int scale = 1; uint32_t serial = 1; bool attached = false; bool committed = false; bool alive = true; std::vector<wl_resource *> callbacks; };
    struct Lock { Fake *self; bool locked = false; };
    wl_display *display = nullptr;
    wl_event_loop *loop = nullptr;
    wl_resource *pendingLock = nullptr;
    wl_resource *activeLock = nullptr;
    wl_resource *pointer = nullptr;
    wl_resource *keyboard = nullptr;
    std::vector<wl_resource *> roles;
    std::vector<wl_resource *> outputResources;
    std::vector<wl_resource *> surfaces;
    std::vector<std::unique_ptr<Surface>> surfaceData;
    wl_global *extraOutput = nullptr;
    std::thread worker;
    std::atomic<bool> quit{false};
    std::atomic<int> command{0};
    std::atomic<int> commits{0}, unlocks{0}, destroys{0}, errors{0};
    std::atomic<bool> redFrame{false}, blueFrame{false};
    Mode mode;
    const char *socket = nullptr;
    int outputScale = 1;
    int autoWakeMs = 0;
    int autoMotion = 0;
    bool autoWoke = false;
    std::chrono::steady_clock::time_point lockTime;

    static void destroy(wl_client *, wl_resource *resource) { wl_resource_destroy(resource); }
    static Fake *owner(wl_resource *r) { return static_cast<Fake *>(wl_resource_get_user_data(r)); }
    static void bindOutput(wl_client *client, void *data, uint32_t version, uint32_t id)
    {
        auto *self = static_cast<Fake *>(data);
        auto *r = wl_resource_create(client, &wl_output_interface, std::min(version, 2u), id);
        wl_resource_set_implementation(r, nullptr, self, [](wl_resource *r) {
            auto &all = owner(r)->outputResources;
            all.erase(std::remove(all.begin(), all.end(), r), all.end());
        });
        self->outputResources.push_back(r);
        wl_output_send_geometry(r, int(self->outputResources.size() - 1) * 64, 0, 500, 300,
                                WL_OUTPUT_SUBPIXEL_UNKNOWN, "fake", "smoke", WL_OUTPUT_TRANSFORM_NORMAL);
        wl_output_send_mode(r, WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED,
                            64 * self->outputScale, 48 * self->outputScale, 60000);
        if (wl_resource_get_version(r) >= 2) {
            wl_output_send_scale(r, self->outputScale);
            wl_output_send_done(r);
        }
    }
    static void createSurface(wl_client *client, wl_resource *compositor, uint32_t id)
    {
        auto *r = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(compositor), id);
        auto *s = new Surface{owner(compositor)};
        s->self->surfaceData.emplace_back(s);
        s->self->surfaces.push_back(r);
        static const struct wl_surface_interface implementation = {
            destroy,
            [](wl_client *, wl_resource *r, wl_resource *buffer, int32_t, int32_t) {
                auto *s = static_cast<Surface *>(wl_resource_get_user_data(r));
                s->buffer = buffer; s->attached = true;
            },
            [](wl_client *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {},
            [](wl_client *c, wl_resource *r, uint32_t id) {
                auto *callback = wl_resource_create(c, &wl_callback_interface, 1, id);
                static_cast<Surface *>(wl_resource_get_user_data(r))->callbacks.push_back(callback);
            },
            [](wl_client *, wl_resource *, wl_resource *) {},
            [](wl_client *, wl_resource *, wl_resource *) {},
            [](wl_client *, wl_resource *r) {
                auto &s = *static_cast<Surface *>(wl_resource_get_user_data(r));
                // Empty initial commits are legal; attaching a buffer before configure/ack is not.
                if (!s.attached || !s.buffer) return;
                s.attached = false;
                if (!s.role || !s.acked) { ++s.self->errors; return; }
                auto *buffer = wl_shm_buffer_get(s.buffer);
                if (!buffer || wl_shm_buffer_get_width(buffer) != s.width * s.scale
                    || wl_shm_buffer_get_height(buffer) != s.height * s.scale) {
                    ++s.self->errors;
                    return;
                }
                wl_shm_buffer_begin_access(buffer);
                const auto *pixels = static_cast<const QRgb *>(wl_shm_buffer_get_data(buffer));
                CHECK(pixels);
                const int stride = wl_shm_buffer_get_stride(buffer) / int(sizeof(QRgb));
                const QRgb center = pixels[(wl_shm_buffer_get_height(buffer) / 2) * stride + wl_shm_buffer_get_width(buffer) / 2];
                if (qRed(center) == 255 && qGreen(center) == 0 && qBlue(center) == 0)
                    s.self->redFrame = true;
                if (qRed(center) == 0 && qGreen(center) == 0 && qBlue(center) == 255)
                    s.self->blueFrame = true;
                wl_shm_buffer_end_access(buffer);
                ++s.self->commits;
                // Release immediately. A resize also exercises reuse/retirement.
                wl_buffer_send_release(s.buffer);
                for (auto *callback : s.callbacks) {
                    wl_callback_send_done(callback, 1);
                    wl_resource_destroy(callback);
                }
                s.callbacks.clear();
                s.committed = true;
                if (s.self->pendingLock && s.self->mode == Accept
                    && s.self->roles.size() >= 2
                    && std::all_of(s.self->roles.begin(), s.self->roles.end(), [](wl_resource *r) {
                        return static_cast<Surface *>(wl_resource_get_user_data(r))->committed;
                    })) s.self->sendLocked();
            },
            [](wl_client *, wl_resource *, int32_t) {}, // buffer transform
            [](wl_client *, wl_resource *r, int32_t scale) {
                static_cast<Surface *>(wl_resource_get_user_data(r))->scale = scale;
            }, // buffer scale
            [](wl_client *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {} // damage_buffer

        };
        wl_resource_set_implementation(r, &implementation, s, [](wl_resource *r) {
            auto *s = static_cast<Surface *>(wl_resource_get_user_data(r));
            auto &all = s->self->surfaces;
            all.erase(std::remove(all.begin(), all.end(), r), all.end());
            for (auto *callback : s->callbacks) wl_resource_destroy(callback);
            s->alive = false;
        });
    }
    int width = 64;
    void sendLocked()
    {
        if (!pendingLock) return;
        static_cast<Lock *>(wl_resource_get_user_data(pendingLock))->locked = true;
        ext_session_lock_v1_send_locked(pendingLock);
        activeLock = pendingLock;
        pendingLock = nullptr;
    }
    explicit Fake(Mode m, int wakeDelay = 0, int scale = 1) : mode(m), outputScale(scale), autoWakeMs(wakeDelay)
    {
        display = wl_display_create();
        CHECK(display);
        loop = wl_display_get_event_loop(display);
        CHECK(wl_display_init_shm(display) == 0);
        wl_global_create(display, &wl_compositor_interface, 4, this,
            [](wl_client *c, void *data, uint32_t version, uint32_t id) {
                auto *r = wl_resource_create(c, &wl_compositor_interface, std::min(version, 4u), id);
                static const struct wl_compositor_interface impl = {createSurface,
                    [](wl_client *c, wl_resource *, uint32_t id) {
                        auto *r = wl_resource_create(c, &wl_region_interface, 1, id);
                        static const struct wl_region_interface impl = {destroy,
                            [](wl_client *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {},
                            [](wl_client *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {}};
                        wl_resource_set_implementation(r, &impl, nullptr, nullptr);
                    }};
                wl_resource_set_implementation(r, &impl, data, nullptr);
            });
        // QPA requires a shell integration at startup, even though saver
        // windows install their own session-lock role and must never use it.
        wl_global_create(display, &wl_shell_interface, 1, this,
            [](wl_client *c, void *data, uint32_t, uint32_t id) {
                auto *r = wl_resource_create(c, &wl_shell_interface, 1, id);
                static const struct wl_shell_interface impl = {
                    [](wl_client *, wl_resource *shell, uint32_t, wl_resource *) {
                        ++owner(shell)->errors; // A default shell role is a regression.
                    }
                };
                wl_resource_set_implementation(r, &impl, data, nullptr);
            });
        wl_global_create(display, &wl_output_interface, 2, this, bindOutput);
        wl_global_create(display, &wl_output_interface, 2, this, bindOutput);
        wl_global_create(display, &wl_seat_interface, 1, this,
            [](wl_client *c, void *data, uint32_t, uint32_t id) {
                auto *r = wl_resource_create(c, &wl_seat_interface, 1, id);
                static const struct wl_seat_interface impl = {
                    [](wl_client *c, wl_resource *seat, uint32_t id) {
                        auto *r = wl_resource_create(c, &wl_pointer_interface, 1, id);
                        static const struct wl_pointer_interface impl = {
                            [](wl_client *, wl_resource *, uint32_t, wl_resource *, int32_t, int32_t) {}
                        };
                        wl_resource_set_implementation(r, &impl, owner(seat), nullptr);
                        owner(seat)->pointer = r;
                        wl_resource_set_destructor(r, [](wl_resource *r) { owner(r)->pointer = nullptr; });
                    },
                    [](wl_client *c, wl_resource *seat, uint32_t id) {
                        auto *r = wl_resource_create(c, &wl_keyboard_interface, 1, id);
                        wl_resource_set_implementation(r, nullptr, owner(seat), nullptr);
                        owner(seat)->keyboard = r;
                        wl_resource_set_destructor(r, [](wl_resource *r) { owner(r)->keyboard = nullptr; });
                        auto *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
                        CHECK(context);
                        auto *keymap = xkb_keymap_new_from_names(context, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
                        CHECK(keymap);
                        char *text = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
                        CHECK(text);
                        QTemporaryFile file;
                        CHECK(file.open());
                        CHECK(file.write(text, qint64(strlen(text) + 1)) > 0);
                        CHECK(file.flush());
                        wl_keyboard_send_keymap(r, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, file.handle(), uint32_t(file.size()));
                        free(text);
                        xkb_keymap_unref(keymap);
                        xkb_context_unref(context);
                    }, nullptr
                };
                wl_resource_set_implementation(r, &impl, data, nullptr);
                wl_seat_send_capabilities(r, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD);
            });
        wl_global_create(display, &ext_session_lock_manager_v1_interface, 1, this,
            [](wl_client *c, void *data, uint32_t, uint32_t id) {
                auto *r = wl_resource_create(c, &ext_session_lock_manager_v1_interface, 1, id);
                static const struct ext_session_lock_manager_v1_interface impl = {
                    destroy,
                    [](wl_client *c, wl_resource *manager, uint32_t id) {
                        auto *self = owner(manager);
                        auto *r = wl_resource_create(c, &ext_session_lock_v1_interface, 1, id);
                        static const struct ext_session_lock_v1_interface impl = {
                            [](wl_client *, wl_resource *r) {
                                auto *lock = static_cast<Lock *>(wl_resource_get_user_data(r));
                                if (lock->locked) ++lock->self->errors;
                                ++lock->self->destroys;
                                wl_resource_destroy(r);
                            },
                            [](wl_client *c, wl_resource *lock, uint32_t id, wl_resource *surface, wl_resource *output) {
                                auto *s = static_cast<Surface *>(wl_resource_get_user_data(surface));
                                auto *self = static_cast<Lock *>(wl_resource_get_user_data(lock))->self;
                                auto *r = wl_resource_create(c, &ext_session_lock_surface_v1_interface, 1, id);
                                CHECK(!s->role);
                                s->role = r;
                                s->acked = false;
                                wl_surface_send_enter(surface, output);
                                self->roles.push_back(r);
                                static const struct ext_session_lock_surface_v1_interface impl = {
                                    destroy,
                                    [](wl_client *, wl_resource *r, uint32_t serial) {
                                        auto *s = static_cast<Surface *>(wl_resource_get_user_data(r));
                                        if (serial != s->serial) ++s->self->errors;
                                        s->acked = serial == s->serial;
                                    }
                                };
                                wl_resource_set_implementation(r, &impl, s, [](wl_resource *r) {
                                    auto *s = static_cast<Surface *>(wl_resource_get_user_data(r));
                                    s->role = nullptr;
                                    auto &all = s->self->roles;
                                    all.erase(std::remove(all.begin(), all.end(), r), all.end());
                                });
                                ext_session_lock_surface_v1_send_configure(r, 1, 64, 48);
                            },
                            [](wl_client *, wl_resource *r) {
                                auto *lock = static_cast<Lock *>(wl_resource_get_user_data(r));
                                if (!lock->locked) ++lock->self->errors;
                                ++lock->self->unlocks;
                                wl_resource_destroy(r);
                            }
                        };
                        wl_resource_set_implementation(r, &impl, new Lock{self}, [](wl_resource *r) {
                            auto *lock = static_cast<Lock *>(wl_resource_get_user_data(r));
                            if (lock->self->pendingLock == r) lock->self->pendingLock = nullptr;
                            if (lock->self->activeLock == r) lock->self->activeLock = nullptr;
                            delete lock;
                        });
                        self->pendingLock = r;
                        self->lockTime = std::chrono::steady_clock::now();
                        self->autoWoke = false;
                        self->autoMotion = 0;
                        if (self->mode == Refuse) ext_session_lock_v1_send_finished(r);
                        else if (self->mode == Early) self->sendLocked();
                    }
                };
                wl_resource_set_implementation(r, &impl, data, nullptr);
            });
        socket = wl_display_add_socket_auto(display);
        CHECK(socket);
        qputenv("WAYLAND_DISPLAY", socket);
        worker = std::thread([this] {
            while (!quit) {
                wl_event_loop_dispatch(loop, 10);
                const int cmd = command.exchange(0);
                if (cmd == 1) {
                    width = 80;
                    for (auto *r : roles) {
                        auto *s = static_cast<Surface *>(wl_resource_get_user_data(r));
                        s->width = 80; s->serial = 2; s->acked = false;
                        ext_session_lock_surface_v1_send_configure(r, 2, 80, 48);
                    }
                } else if (cmd == 2 && pointer && !surfaces.empty())
                    wl_pointer_send_enter(pointer, 3, surfaces.front(), 0, 0);
                else if (cmd == 3 && pointer) wl_pointer_send_motion(pointer, 10, 256, 256);
                else if (cmd == 9 && pointer) wl_pointer_send_motion(pointer, 11, 512, 512);
                else if (cmd == 4) extraOutput = wl_global_create(display, &wl_output_interface, 2, this, bindOutput);
                else if (cmd == 5 && extraOutput) { wl_global_destroy(extraOutput); extraOutput = nullptr; }
                else if (cmd == 6 && activeLock) ext_session_lock_v1_send_finished(activeLock);
                else if (cmd == 7 && keyboard && !surfaces.empty()) {
                    wl_array keys; wl_array_init(&keys);
                    wl_keyboard_send_enter(keyboard, 10, surfaces.front(), &keys);
                    wl_array_release(&keys);
                } else if (cmd == 8 && keyboard)
                    wl_keyboard_send_key(keyboard, 11, 12, 30, WL_KEYBOARD_KEY_STATE_PRESSED);
                if (mode == Delayed && pendingLock && std::chrono::steady_clock::now() - lockTime > std::chrono::milliseconds(3300))
                    sendLocked();
                if (autoWakeMs && activeLock && !autoWoke && pointer && !surfaces.empty()
                    && std::chrono::steady_clock::now() - lockTime > std::chrono::milliseconds(autoWakeMs)) {
                    wl_pointer_send_enter(pointer, 20, surfaces.front(), 0, 0);
                    wl_pointer_send_motion(pointer, 21, 256, 256);
                    autoMotion = 1;
                    autoWoke = true;
                }
                if (autoMotion == 1 && pointer && activeLock
                    && std::chrono::steady_clock::now() - lockTime > std::chrono::milliseconds(autoWakeMs + 100)) {
                    wl_pointer_send_motion(pointer, 22, 512, 512);
                    autoMotion = 2;
                }
                wl_display_flush_clients(display);
            }
        });
    }
    ~Fake()
    {
        quit = true;
        worker.join();
        // Destroy role resources first: their userdata references surfaces.
        for (auto *r : std::vector<wl_resource *>(roles)) wl_resource_destroy(r);
        wl_display_destroy_clients(display);
        wl_display_destroy(display);
    }
};

template<typename Predicate> void until(Predicate predicate)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    CHECK(predicate());
}

static void spin(int milliseconds)
{
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < milliseconds) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
}

int main(int argc, char **argv)
{
    const QByteArray mode = argc > 1 ? argv[1] : "";
    const bool serve = mode == "--serve";
    const bool child = mode == "--case";
    if (!serve && !child) {
        QCoreApplication app(argc, argv);
        for (const QString &scenario : {QStringLiteral("accept"), QStringLiteral("early"), QStringLiteral("refuse"),
                                        QStringLiteral("cancel"), QStringLiteral("scale2"), QStringLiteral("unavailable")}) {
            QProcess process;
            process.setProcessChannelMode(QProcess::ForwardedChannels);
            process.start(app.applicationFilePath(), {QStringLiteral("--case"), scenario});
            CHECK(process.waitForStarted());
            CHECK(process.waitForFinished(20000));
            CHECK(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
        }
        std::cout << "waylandsaver smoke: PASS\n";
        return 0;
    }
    QTemporaryDir runtime;
    CHECK(runtime.isValid());
    if (!serve || qEnvironmentVariableIsEmpty("XDG_RUNTIME_DIR"))
        qputenv("XDG_RUNTIME_DIR", runtime.path().toUtf8());
    if (serve) {
        // No GUI connection in the server process, and no real desktop access.
        QCoreApplication app(argc, argv);
        Fake fake(Fake::Accept, argc > 2 ? std::max(1, QByteArray(argv[2]).toInt()) : 1500);
        std::cout << fake.socket << std::endl;
        return app.exec();
    }
    const QByteArray scenario = argc > 2 ? argv[2] : "accept";
    QTemporaryDir config;
    CHECK(config.isValid());
    qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
    const QString pictures = config.path() + QStringLiteral("/pictures");
    CHECK(QDir().mkpath(pictures));
    QImage frame(64, 48, QImage::Format_RGB32);
    frame.fill(Qt::red);
    CHECK(frame.save(pictures + QStringLiteral("/1.png")));
    frame.fill(Qt::blue);
    CHECK(frame.save(pictures + QStringLiteral("/2.png")));
    // Cover both historical application-name-derived INI locations.
    for (const QString &name : {QStringLiteral("deepin-custom-screensaver"), QStringLiteral("deepin-screensaver")}) {
        QSettings settings(config.path() + "/deepin/deepin-screensaver/" + name + "/" + name + ".conf", QSettings::IniFormat);
        settings.setValue(QStringLiteral("custom_screensaver.path.select_path/value"), pictures);
        settings.setValue(QStringLiteral("custom_screensaver.slideshow.time_interval/value"), 3);
        settings.setValue(QStringLiteral("custom_screensaver.slideshow.shuffle/value"), false);
        settings.sync();
        CHECK(settings.status() == QSettings::NoError);
    }
    // QPA connects during QGuiApplication construction, not during saver.start().
    Fake fake(scenario == "refuse" ? Fake::Refuse : scenario == "cancel" ? Fake::Delayed : scenario == "early" ? Fake::Early : Fake::Accept, 0, scenario == "scale2" ? 2 : 1);
    qputenv("QT_QPA_PLATFORM", scenario == "unavailable" ? "offscreen" : "wayland");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    qputenv("QT_QUICK_BACKEND", "software");
    qputenv("QT_OPENGL", "software");
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    WaylandSaver saver;
    int wakes = 0, stopped = 0, activated = 0, failures = 0, heartbeats = 0;
    QObject::connect(&saver, &WaylandSaver::wakeRequested, [&] { ++wakes; });
    QObject::connect(&saver, &WaylandSaver::stopped, [&] { ++stopped; });
    QObject::connect(&saver, &WaylandSaver::activated, [&] { ++activated; });
    QObject::connect(&saver, &WaylandSaver::failed, [&](const QString &) { ++failures; });
    QTimer heartbeat;
    heartbeat.setInterval(10);
    QObject::connect(&heartbeat, &QTimer::timeout, [&] { ++heartbeats; });
    heartbeat.start();
    QElapsedTimer request;
    request.start();
    const bool accepted = saver.start();
    CHECK(request.elapsed() < 500); // start must never wait for compositor replies.
    if (scenario == "unavailable") {
        CHECK(!accepted);
        CHECK(!saver.isRunning());
        CHECK(!saver.isBusy());
        CHECK(saver.stop());
        return 0;
    }
    CHECK(accepted);
    if (scenario == "refuse") {
        until([&] { return failures > 0 && !saver.isBusy(); });
        CHECK(activated == 0 && !saver.isRunning());
        CHECK(saver.stop());
        until([&] { return fake.destroys == 1; });
        CHECK(fake.unlocks == 0 && fake.errors == 0);
        return 0;
    }
    if (scenario == "cancel") {
        // Cancellation cannot destroy a request for which locked is in flight.
        spin(100);
        CHECK(saver.isBusy() && !saver.isRunning());
        request.restart();
        saver.stop();
        CHECK(request.elapsed() < 500);
        saver.stop(); // idempotent while cancellation waits for the server.
        until([&] { return stopped > 0 && !saver.isBusy(); });
        CHECK(activated == 0);
        CHECK(heartbeats > 20); // Event loop stayed live while waiting for late locked.
        until([&] { return fake.unlocks == 1; });
        CHECK(fake.destroys == 0 && fake.errors == 0);
        return 0;
    }
    CHECK(!saver.start()); // Busy request is rejected without creating a second lock.
    until([&] { return activated == 1 && saver.isRunning(); });
    until([&] { return fake.commits >= 2; });
    until([&] { return fake.redFrame.load(); });
    until([&] { return fake.blueFrame.load(); });
    const int beforeResize = fake.commits;
    fake.command = 1;
    until([&] { return fake.commits >= beforeResize + 2; });
    fake.command = 2;
    spin(100);
    CHECK(wakes == 0); // Pointer enter is focus, not user activity.
    // Qt5 synthesizes MouseMove on enter whereas Qt6 may deliver only Enter.
    // Real motion must wake by the second changed position on either version.
    fake.command = 3;
    spin(100);
    CHECK(wakes <= 1);
    fake.command = 9;
    until([&] { return wakes == 1; });
    const int beforeHotplug = fake.commits;
    fake.command = 4;
    until([&] { return fake.commits > beforeHotplug; });
    fake.command = 5;
    spin(100);
    request.restart();
    saver.stop();
    CHECK(request.elapsed() < 500);
    until([&] { return stopped == 1 && !saver.isBusy(); });
    until([&] { return fake.unlocks == 1; });
    CHECK(fake.errors == 0 && !saver.isRunning());
    CHECK(saver.stop());
    CHECK(saver.start());
    until([&] { return activated == 2 && saver.isRunning(); });
    fake.command = 7;
    spin(100);
    CHECK(wakes == 1); // Keyboard focus is not activity either.
    fake.command = 8;
    until([&] { return wakes == 2; });
    fake.command = 6;
    until([&] { return stopped == 2 && !saver.isBusy(); });
    until([&] { return fake.unlocks == 2; });
    CHECK(fake.errors == 0);
    CHECK(heartbeats > 20);
    return 0;
}
