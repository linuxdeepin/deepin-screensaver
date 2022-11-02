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

DWIDGET_USE_NAMESPACE

SlideshowScreenSaver::SlideshowScreenSaver(bool subWindow, QWidget *parent)
    : QWidget(parent), m_subWindow(subWindow)
{
    qApp->loadTranslator();

    if (m_subWindow)
        setWindowFlag(Qt::WindowTransparentForInput, true);

    loadSlideshowImages();

    if (!m_imagefiles.isEmpty())
        m_pixmap.reset(new QPixmap(m_imagefiles.first()));

    m_shuffle = SlideshowConfig::instance()->isShuffle();
    m_intervalTime = SlideshowConfig::instance()->intervalTime();

    m_timer.reset(new QTimer);
    connect(m_timer.get(), &QTimer::timeout, this, &SlideshowScreenSaver::onUpdateImage);

    m_timer->setInterval(m_intervalTime);
    m_timer->start();
}

SlideshowScreenSaver::~SlideshowScreenSaver()
{
}

void SlideshowScreenSaver::onUpdateImage()
{
    if (updateCurrentImageIndex()) {
        if (0 > m_currentIndex || m_currentIndex > m_imagefiles.size() - 1)
            return;

        auto indexPath = m_imagefiles.at(m_currentIndex);
        QFileInfo f(indexPath);
        if (f.exists()) {
            m_pixmap.reset(new QPixmap(m_imagefiles.at(m_currentIndex)));
            update();
            return;
        } else {
            loadSlideshowImages();
            onUpdateImage();
        }

    } else {
        m_pixmap.reset(nullptr);
        m_timer->stop();
    }

    update();
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

bool SlideshowScreenSaver::updateCurrentImageIndex()
{
    if (m_imagefiles.isEmpty())
        return false;

    if (m_shuffle) {
        m_currentIndex = int(QRandomGenerator::global()->generate() % uint(m_imagefiles.count()));
    } else {
        if (m_currentIndex < m_imagefiles.count() - 1)
            m_currentIndex++;
        else
            m_currentIndex = 0;
    }

    return true;
}

bool SlideshowScreenSaver::loadSlideshowImages()
{
    m_imagefiles.clear();
    QString path = SlideshowConfig::instance()->slideshowPath();

    QFileInfo fileInfo(path);
    if (fileInfo.exists() && fileInfo.isDir()) {
        QDir dir(path);
        QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
        QFileInfoList infoList = dir.entryInfoList(filters, QDir::Name);
        if (infoList.isEmpty()) {
            qWarning() << "Warning:no image file in " << path;
            return false;
        }

        static const QStringList validSuffix { QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("bmp"), QStringLiteral("png") };
        for (auto info : infoList) {
            if (validSuffix.contains(info.suffix()))
                m_imagefiles.append(info.absoluteFilePath());   // 记录图片列表
        }
    }

    return false;
}
