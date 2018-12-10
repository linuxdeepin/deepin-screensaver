/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
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
#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QWidget>
#include <QProcess>

namespace Ui {
class PreviewWindow;
}

class PreviewWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewWindow(const QString &screenSaverPath, QWidget *parent = 0);
    ~PreviewWindow();

private slots:
    void on_pushButton_clicked();

private:
    Ui::PreviewWindow *ui;
    QProcess process;
};

#endif // PREVIEWWINDOW_H
