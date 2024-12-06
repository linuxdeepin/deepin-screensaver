// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SLIDESHOWSCREENSAVER_H
#define SLIDESHOWSCREENSAVER_H

#include <QAbstractNativeEventFilter>
#include <QWidget>
#include <QMap>
#include <QWebEngineView>

class SlideshowScreenSaver : public QWebEngineView, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit SlideshowScreenSaver(bool subWindow = false, QWidget *parent = nullptr);
    ~SlideshowScreenSaver() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
    void init();

protected:
    void closeEvent(QCloseEvent *event) override;

    QSize mapFromHandle(const QSize &handleSize);

private:
    void loadSlideshowImages();
    void updateViewSize(const QSize &updatesize);

private:
    bool m_subWindow { false };
    QString m_url;
};

#endif   // SLIDESHOWSCREENSAVER_H
