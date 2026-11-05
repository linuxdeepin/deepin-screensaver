// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "waylandsaver.h"
#include "sessionlock.h"

#include <QBackingStore>
#include <QDir>
#include <QFileInfo>
#include <QEnterEvent>
#include <QGuiApplication>
#include <QImageReader>
#include <QMouseEvent>
#include <QMap>
#include <QVariant>
#include <QPainter>
#include <QRandomGenerator>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QWindow>
#include <algorithm>
#if __has_include(<DConfig>) && !defined(SAVER_TEST_NO_DCONFIG)
#include <DConfig>
#define SAVER_HAS_DCONFIG 1
#endif

// 普通 Qt 图片窗口：缓冲区、绘图和输入都由 Qt 管理。
// SessionLock 仅负责把窗口设置为锁屏窗口，以及配置完成后显示窗口。
class SlideshowWindow : public QWindow
{
public:
    explicit SlideshowWindow(QScreen *screen) : QWindow(screen), backingStore(this)
    {
        setSurfaceType(QSurface::RasterSurface);
        setCursor(Qt::BlankCursor);
    }
    void setImage(const QImage &image) { currentImage = image; requestUpdate(); }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::UpdateRequest) {
            paint();
            return true;
        }
        return QWindow::event(event);
    }
    void exposeEvent(QExposeEvent *) override { paint(); }
    void resizeEvent(QResizeEvent *) override { requestUpdate(); }

private:
    void paint()
    {
        if (!isExposed() || size().isEmpty()) return;
        backingStore.resize(size());
        const QRect area(QPoint(), size());
        backingStore.beginPaint(area);
        QPainter painter(backingStore.paintDevice());
        painter.fillRect(area, Qt::black);
        if (!currentImage.isNull()) {
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            const QSize scaled = currentImage.size().scaled(size(), Qt::KeepAspectRatioByExpanding);
            painter.drawImage(QRect(QPoint((width() - scaled.width()) / 2,
                                          (height() - scaled.height()) / 2), scaled), currentImage);
        }
        painter.end();
        backingStore.endPaint();
        backingStore.flush(area);
    }
    QBackingStore backingStore;
    QImage currentImage;
};

class WaylandSaver::Private : public QObject
{
public:
    explicit Private(WaylandSaver *owner) : QObject(owner), q(owner), lock(this)
    {
        connect(&lock, &SessionLock::ready, this, [this] {
            acceptingWindows = true;
            for (QScreen *screen : QGuiApplication::screens()) addScreen(screen);
        });
        connect(&lock, &SessionLock::locked, this, [this] {
            slideshow.start();
            emit q->activated();
        });
        connect(&lock, &SessionLock::stopped, this, [this] {
            slideshow.stop();
            acceptingWindows = false;
            qDeleteAll(windows);
            windows.clear();
            wakeQueued = false;
            emit q->stopped();
        });
        connect(&lock, &SessionLock::failed, q, &WaylandSaver::failed);
        connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
            if (acceptingWindows) addScreen(screen);
        });
        connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen *screen) {
            delete windows.take(screen);
        });
        connect(&slideshow, &QTimer::timeout, this, [this] {
            nextImage();
            for (auto *window : windows) window->setImage(image);
        });
    }
    ~Private() override
    {
        lock.stop();
        qDeleteAll(windows);
    }

    WaylandSaver *q;
    SessionLock lock;
    QTimer slideshow;
    QMap<QScreen *, SlideshowWindow *> windows;
    QStringList images;
    QImage image;
    int imageIndex = -1;
    bool wakeQueued = false;
    bool acceptingWindows = false;

    void addScreen(QScreen *screen)
    {
        if (windows.contains(screen)) return;
        auto *window = new SlideshowWindow(screen);
        window->setImage(image);
        window->installEventFilter(this);
        windows.insert(screen, window);
        // The adapter waits for the compositor's size before showing the window.
        if (!lock.addWindow(window)) {
            emit q->failed(QStringLiteral("Cannot create a session-lock window for this screen"));
            QTimer::singleShot(0, &lock, &SessionLock::stop);
        }
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        bool wake = event->type() == QEvent::KeyPress
                 || event->type() == QEvent::MouseButtonPress
                 || event->type() == QEvent::Wheel
                 || event->type() == QEvent::TouchBegin;
        if (event->type() == QEvent::Enter) {
            auto *enter = static_cast<QEnterEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            watched->setProperty("lastPointerPosition", enter->globalPosition().toPoint());
#else
            watched->setProperty("lastPointerPosition", enter->screenPos().toPoint());
#endif
        }
        if (event->type() == QEvent::MouseMove) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const QPoint position = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
#else
            const QPoint position = static_cast<QMouseEvent *>(event)->screenPos().toPoint();
#endif
            // Focus/initial pointer position is not user activity.
            const QVariant previous = watched->property("lastPointerPosition");
            watched->setProperty("lastPointerPosition", position);
            wake = previous.isValid() && previous.toPoint() != position;
        }
        if (wake && lock.isLocked() && !wakeQueued) {
            wakeQueued = true;
            emit q->wakeRequested();
        }
        return false;
    }

    void loadImages()
    {
        QString path;
        int seconds = 0;
        int playMode = -1;
#ifdef SAVER_HAS_DCONFIG
        std::unique_ptr<Dtk::Core::DConfig> config(Dtk::Core::DConfig::create(
            QStringLiteral("org.deepin.screensaver"), QStringLiteral("org.deepin.customscreensaver")));
        if (config && config->isValid()) {
            path = config->value(QStringLiteral("slideshowPath")).toString();
            seconds = config->value(QStringLiteral("intervalTime")).toInt();
            playMode = config->value(QStringLiteral("playMode"), -1).toInt();
        }
#endif
        // Read both historical executable names, after managed configuration.
        const QString root = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                           + QStringLiteral("/deepin/deepin-screensaver/");
        for (const QString &name : {QStringLiteral("deepin-screensaver"), QStringLiteral("deepin-custom-screensaver")}) {
            QSettings legacy(root + name + '/' + name + QStringLiteral(".conf"), QSettings::IniFormat);
            if (path.isEmpty()) path = legacy.value(QStringLiteral("custom_screensaver.path.select_path/value")).toString();
            if (seconds < 1) seconds = legacy.value(QStringLiteral("custom_screensaver.slideshow.time_interval/value")).toInt();
            if (playMode < 0 && legacy.contains(QStringLiteral("custom_screensaver.slideshow.shuffle/value")))
                playMode = legacy.value(QStringLiteral("custom_screensaver.slideshow.shuffle/value")).toBool() ? 1 : 0;
        }
        slideshow.setInterval((seconds >= 3 && seconds <= 3600 ? seconds : 10) * 1000);
        images.clear();
        auto append = [this](QString source) {
            if (source.startsWith(QLatin1String("file:"))) source = QUrl(source).toLocalFile();
            if (source == QLatin1String("~") || source.startsWith(QLatin1String("~/")))
                source.replace(0, 1, QDir::homePath());
            if (QFileInfo(source).isFile()) {
                if (QImageReader(source).canRead()) images.append(source);
            } else {
                for (const QFileInfo &file : QDir(source).entryInfoList(QDir::Files | QDir::Readable, QDir::Name))
                    if (QImageReader(file.absoluteFilePath()).canRead()) images.append(file.absoluteFilePath());
            }
        };
        if (!path.isEmpty()) append(path);
        if (images.isEmpty()) append(QDir::homePath() + QStringLiteral("/Pictures/Wallpapers"));
        if (images.isEmpty()) append(QDir::homePath() + QStringLiteral("/.config/deepin/deepin-screensaver/saverpic/defaultpic.jpg"));
        if (images.isEmpty()) append(QStringLiteral("/usr/share/wallpapers/deepin"));
        if (images.isEmpty()) append(QStringLiteral("/usr/share/backgrounds"));
        if (playMode == 1) std::shuffle(images.begin(), images.end(), *QRandomGenerator::global());
        imageIndex = -1;
        nextImage();
    }
    void nextImage()
    {
        image = QImage();
        for (int attempts = 0; attempts < images.size(); ++attempts) {
            imageIndex = (imageIndex + 1) % images.size();
            if (image.load(images.at(imageIndex))) break;
        }
    }
};

WaylandSaver::WaylandSaver(QObject *parent) : QObject(parent), d(new Private(this)) {}
WaylandSaver::~WaylandSaver() = default;
bool WaylandSaver::start()
{
    if (d->lock.isBusy()) return false;
    if (!QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) return false;
    d->loadImages();
    d->lock.start();
    return true; // Accepted, not yet locked. Completion is signalled asynchronously.
}
bool WaylandSaver::stop()
{
    d->acceptingWindows = false;
    d->slideshow.stop();
    d->lock.stop();
    return !d->lock.isBusy();
}
bool WaylandSaver::isRunning() const { return d->lock.isLocked(); }
bool WaylandSaver::isBusy() const { return d->lock.isBusy(); }
