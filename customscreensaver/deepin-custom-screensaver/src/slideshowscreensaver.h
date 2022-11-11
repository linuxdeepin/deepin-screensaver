// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SLIDESHOWSCREENSAVER_H
#define SLIDESHOWSCREENSAVER_H

#include <QWidget>
#include <QMap>

class SlideshowScreenSaver : public QWidget
{
    Q_OBJECT
public:
    explicit SlideshowScreenSaver(bool subWindow = false, QWidget *parent = nullptr);
    ~SlideshowScreenSaver() override;

private slots:
    void onUpdateImage();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void showDefaultBlack(QPaintEvent *event);
    void randomImageIndex();
    void loadSlideshowImages();
    void init();
    void nextValidPath();
    void filterInvalidFile(const QString &path);

private:
    QScopedPointer<QPixmap> m_pixmap;
    QScopedPointer<QTimer> m_timer;

    QString m_path;   // 图片路径
    bool m_shuffle { false };   // 随机轮播
    int m_intervalTime { 0 };   // 轮播间隔
    int m_currentIndex { 0 };
    QStringList m_imagefiles;
    bool m_subWindow { false };
    QMap<int, QString> m_playOrder;
    QString m_lastImage;
};

#endif   // SLIDESHOWSCREENSAVER_H
