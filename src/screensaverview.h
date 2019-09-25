/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
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
#ifndef SCREENSAVERVIEW_H
#define SCREENSAVERVIEW_H

#include <QQuickView>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

class ScreenSaverView : public QQuickView
{
    Q_OBJECT

public:
    explicit ScreenSaverView(QWindow *parent = nullptr);
    ~ScreenSaverView();

    bool start(const QString &filePath);
    void stop();

signals:
    void inputEvent(QEvent::Type type);

private:
    bool event(QEvent *event) override;

    QProcess *m_process = nullptr;

    friend class ScreenSaverWindow;
};

#endif // SCREENSAVERVIEW_H
