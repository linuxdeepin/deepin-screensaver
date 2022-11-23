// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slideshowscreensaver.h"
#include "slideshowconfig.h"

#include <DApplication>

#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QWindow>
#include <QScreen>
#include <QPainter>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <QDebug>

#define IMAGE_MAX_SIZE 30 * 1024 * 1024 // Only display image smaller than 30M

DWIDGET_USE_NAMESPACE

SlideshowScreenSaver::SlideshowScreenSaver(bool subWindow, QWidget *parent)
    : QWidget(parent), m_subWindow(subWindow)
{
    qApp->loadTranslator();

    if (m_subWindow)
        setWindowFlag(Qt::WindowTransparentForInput, true);

    m_shuffle = SlideshowConfig::instance()->isShuffle();
    m_intervalTime = SlideshowConfig::instance()->intervalTime();

    m_timer.reset(new QTimer);
    connect(m_timer.get(), &QTimer::timeout, this, &SlideshowScreenSaver::onUpdateImage);
    loadSlideshowImages();

    m_timer->setInterval(m_intervalTime);
    if (!m_playOrder.isEmpty())
        m_timer->start();
}

SlideshowScreenSaver::~SlideshowScreenSaver()
{
}

void SlideshowScreenSaver::onUpdateImage()
{
    if (!m_playOrder.isEmpty()) {
        if (m_currentIndex ==  m_playOrder.count()) {
            if (m_shuffle) {
                randomImageIndex();
            } else {
                m_currentIndex = 1;
            }
        } else {
            nextValidPath();
        }
    } else {
        loadSlideshowImages();
    }

    m_pixmap.reset(new QPixmap(m_playOrder.value(m_currentIndex)));
    update();
    return;
}

void SlideshowScreenSaver::paintEvent(QPaintEvent *event)
{
    if (m_pixmap) {
        qreal scale = devicePixelRatioF();
        QSize trueSize(this->geometry().size());
        auto pix = m_pixmap->scaled(trueSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        if (pix.width() > trueSize.width() || pix.height() > trueSize.height()) {
            pix = pix.copy(QRect(static_cast<int>((pix.width() - trueSize.width()) / 2.0),
                                 static_cast<int>((pix.height() - trueSize.height()) / 2.0),
                                 trueSize.width(),
                                 trueSize.height()));
        }
        pix.setDevicePixelRatio(scale);

        QPainter pa(this);
        pa.drawPixmap(event->rect().topLeft(), pix, QRectF(QPointF(event->rect().topLeft()) * scale, QSizeF(event->rect().size()) * scale));
    } else {
        showDefaultBlack(event);
    }

    return QWidget::paintEvent(event);
}

void SlideshowScreenSaver::showDefaultBlack(QPaintEvent *event)
{
    qreal scale = devicePixelRatioF();
    QPixmap pip(this->geometry().size());
    pip.fill(Qt::black);

    QPainter pa(&pip);
    pa.setPen(Qt::white);
    pa.drawText(pip.rect(), Qt::AlignCenter, tr("Picture not found"));   // This text may be written in the configuration in the future

    QPainter p(this);
    p.drawPixmap(event->rect().topLeft(), pip, QRectF(QPointF(event->rect().topLeft()) * scale, QSizeF(event->rect().size()) * scale));
}

void SlideshowScreenSaver::randomImageIndex()
{
    // When "sise < 2", it's like play in sequence
    if (m_playOrder.size() > 2) {
        auto vct = m_playOrder.keys().toVector();
        std::mt19937 rg(std::random_device{}());
        std::shuffle(vct.begin(), vct.end(), rg);
        QMap<int, QString> temp;
        for (int i = 0; i < vct.size(); ++i)
            temp.insert(i + 1, m_playOrder.value(vct[i]));
        m_playOrder.clear();
        m_playOrder = std::move(temp);

        // make sure the display is different twice in a row
        m_currentIndex = (m_lastImage == m_playOrder.first()) ? 2 : 1;
        m_lastImage = m_playOrder.last();
    } else {
        m_currentIndex = 1;
    }
}

void SlideshowScreenSaver::loadSlideshowImages()
{
    m_playOrder.clear();
    QString path = SlideshowConfig::instance()->slideshowPath();

    QFileInfo fileInfo(path);
    if (fileInfo.exists() && fileInfo.isDir()) {
        QDir dir(path);
        QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
        QFileInfoList infoList = dir.entryInfoList(filters, QDir::Name);
        if (infoList.isEmpty()) {
            qWarning() << "Warning:no image file in " << path;
            return;
        }

        static const QStringList validSuffix {QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("bmp"), QStringLiteral("png")};
        int idx = 1;
        for (auto info : infoList) {
            if (info.size() < IMAGE_MAX_SIZE && validSuffix.contains(info.suffix(), Qt::CaseInsensitive)) {
                m_imagefiles.append(info.absoluteFilePath());   // 记录图片列表
                m_playOrder.insert(idx, info.absoluteFilePath());
                idx++;
            }
        }
    }

    init();
}

void SlideshowScreenSaver::init()
{
    if (!m_playOrder.isEmpty()) {
        if (m_shuffle)
            randomImageIndex();
        m_pixmap.reset(new QPixmap(m_playOrder.first()));
        m_currentIndex = 1;
        m_lastImage = m_playOrder.last();
    } else {
        m_pixmap.reset(nullptr);
        m_timer->stop();
    }
}

void SlideshowScreenSaver::nextValidPath()
{
    const int num = m_playOrder.count();

    // It almost never happens
    if (++m_currentIndex > num)
        m_currentIndex = 1;

    filterInvalidFile(m_playOrder.value(m_currentIndex));
}

void SlideshowScreenSaver:: filterInvalidFile(const QString &path)
{
    QFileInfo f(path);
    if (!f.exists())
        loadSlideshowImages();
}
